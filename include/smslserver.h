// Server code. Run to become a server.
// 7/5/2019, LiWentan.

#ifndef _SKSERVER_H_
#define _SKSERVER_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <queue>
#include "smsl.hpp"

namespace {

const char MSG_GET = 1;
const char MSG_SET = 2;
const char MSG_DEL = 3;
const char MSG_INVALID_REQ = -10;
const char MSG_OTHER_ERR = -11;
const char MSG_EMPTY = -127;

} // End anonyomous namespace.

namespace sk_cs {

struct SKString {
    uint16_t size;
    char data[1024];
};

struct Msg {
    Msg(char in) : status(in) {}
    Msg() : status(MSG_EMPTY) {}
    
    /**
     * Inditicate the return status or operations.
     * When it is a message from client to server:
     * 1 -> GET.
     * 2 -> SET.
     * 3 -> DEL.
     * When it is a message from server to client:
     * Normal situation: Return operation function's return value.
     * -10 -> Received invalid message, check the binary format.
     * -11 -> Other errors.
     * -127 -> Empty message, check the code.
     */
    char status;
    SKString key;
    SKString val;
};

template <typename T>
class AtomData {
private:
    AtomData(const AtomData&);
    AtomData& operator=(const AtomData&);
public:
    AtomData(const T& data) : _data(data) {
        pthread_rwlock_init(&_lock, nullptr);
    }
    AtomData() {
        pthread_rwlock_init(&_lock, nullptr);
    }
    ~AtomData() {
        pthread_rwlock_destroy(&_lock);
    }
    
    /**
     * Set the element of this object.
     */
    void set(const T& data) {
        pthread_rwlock_wrlock(&_lock);
        _data = data;
        pthread_rwlock_unlock(&_lock);
    }
    
    /**
     * Get a copy of this object.
     */
    T get() {
        pthread_rwlock_rdlock(&_lock);
        T res = _data;
        pthread_rwlock_unlock(&_lock);
        return res;
    }
private:
    T _data;
    pthread_rwlock_t _lock;
};

template <typename T>
class AtomQueue {
private:
    AtomQueue(const AtomQueue&);
    AtomQueue& operator=(const AtomQueue&);
public:
    AtomQueue() {
        pthread_mutex_init(&_lock, nullptr);
    }
    ~AtomQueue() {
        pthread_mutex_destroy(&_lock);
    }
    
    /**
     * Push element to the queue.
     */
    void push(const T& data) {
        pthread_mutex_lock(&_lock);
        _data.push(data);
        pthread_mutex_unlock(&_lock);
    }
    
    /**
     * Got the copy of top element, and remove this element from the queue.
     * @return 0: success.
     * @return -1: no element in the queue.
     */
    int pop(T& data) {
        pthread_mutex_lock(&_lock);
        if (_data.size() == 0) {
            pthread_mutex_unlock(&_lock);
            return -1;
        }
        data = _data.front();
        _data.pop();
        pthread_mutex_unlock(&_lock);
        return 0;
    }
private:
    std::queue<T> _data;
    pthread_mutex_t _lock;
};

class SKServer {
public:
    /**
     * Initialize the server, specify the listen socket.
     * @param shared_path: Use this path to create or get the memory storaging data.
     */
    SKServer(const std::string& shared_path = "./mempath", 
        const std::string& ip = "127.0.0.1", 
        short port = 8089);

    /**
     * Start the server.
     * @return 0 means success.
     */
    int start();
    
    /**
     * Stop the server.
     * @param clean_mem: If true, clean the shared_memory, if false, the data can be restored next time.
     */
    void stop(bool clean_mem = false);
    
    /**
     * Dump the data to disk.
     * @return 0: success, other: failed.
     */
    int dump(const std::string& dump_path = "./memdump");
    
    /**
     * Restore the data from the dumpped file.
     * Current existing data at memory would be covered.
     * @return 0: success, other: failed.
     */
    int restore(const std::string& dump_path = "./memdump");
private:
    std::string _shared_path; // The path reflecting to the shared_memory.
    std::string _ip; // Listen ip.
    short _port; // Listen port.
    int _listen_socket; // The listen socket.
    pthread_t _listen_pid; // The pid of listen thread.
    pthread_t _handle_pid; // The pid of handle thread.
    AtomData<bool> _listen_stop; // If true, the listen thread will stop.
    AtomQueue<int> _handler_queue; // The queue storaging the client sockets.
    smsl::Smsl<SKString, SKString> *_data; // The Smsl.
private:
    static int _cmp_skstring(const SKString& lhs, const SKString& rhs);
    static std::string _print_skstring(const SKString& para);
    static void* _listen(void* skserver);
    static void* _handle(void* skserver);
};
    
} // End namespace sk_cs.



#endif // End ifdef _SKSERVER_H_.