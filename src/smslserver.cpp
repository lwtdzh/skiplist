// Server code. Run to become a server.
// 7/5/2019, LiWentan.

#include "../include/smslserver.h"

namespace sk_cs {
    
SKServer::SKServer(const std::string& shared_path, 
    const std::string& ip, short port) :
    _ip(ip), _port(port), _shared_path(shared_path),
    _data(nullptr), _listen_socket(-1),
    _handle_pid(-1), _listen_pid(-1) {}
    
int SKServer::start() {
    // Initialize the shared_memory data.
    _data = new(std::nothrow) smsl::Smsl<SKString, SKString>(
        _shared_path, _cmp_skstring, _print_skstring, true);
    if (_data == nullptr) {
        toscreen << "Create smsl failed. Start the server failed.\n";
        return -1;
    }
    _data->set_quit_strategy(false);
    
    // Create listen socket.
    _listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_socket == 0) {
        toscreen << "Create socket failed. Start the server failed.\n";
        delete _data;
        _data = nullptr;
        return -2;
    }
    
    // Set listen addr and NON-BLOCK flag.
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = inet_addr(_ip.c_str());
    int flg = fcntl(_listen_socket, F_GETFL, 0);
    fcntl(_listen_socket, F_SETFL, flg | O_NONBLOCK);
    
    // Bind listen addr to listen socket.
    int ret = bind(_listen_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret < 0) {
        toscreen << "Bind listen addr to listen socket failed. "
            << "Code: " << ret << ". "
            << "Start the server failed.\n";
        delete _data;
        _data = nullptr;
        close(_listen_socket);
        return -3;
    }
    
    // Set the listen addr as listen mode.
    ret = listen(_listen_socket, 2000);
    if (ret < 0) {
        toscreen << "Set socket as listen mode failed. Start the server failed.\n";
        delete _data;
        _data = nullptr;
        close(_listen_socket);
        return -4;
    }
    
    // Create the handle thread.
    _listen_stop.set(false);
    ret = pthread_create(&_handle_pid, nullptr, _handle, this);
    if (ret != 0) {
        toscreen << "Create handle thread failed. Start the server failed.\n";
        delete _data;
        _data = nullptr;
        close(_listen_socket);
        _listen_stop.set(true);
        _handle_pid = -1;
        return -5;
    }
    
    // Create the listen thread.
    ret = pthread_create(&_listen_pid, nullptr, _listen, this);
    if (ret != 0) {
        toscreen << "Create listen thread failed. Start the server failed.\n";
        delete _data;
        _data = nullptr;
        close(_listen_socket);
        _listen_stop.set(true); // Set this flag to stop the created handle thread.
        _handle_pid = -1;
        _listen_pid = -1;
        return -6;
    }

    toscreen << "Start the server successfully. Listen from IP: "
        << _ip << ", PORT: " << _port << ".\n";
    return 0;
}

void SKServer::stop(bool clean_mem) {
    _listen_stop.set(true);
    toscreen << "Waitting for other threads...\n";
    while (_listen_pid != -1 && pthread_kill(_listen_pid, 0) != ESRCH) {
        usleep(3e5);
        continue;
    }
    _listen_pid = -1;
    while (_handle_pid != -1 && pthread_kill(_handle_pid, 0) != ESRCH) {
        usleep(3e5);
        continue;
    }
    _handle_pid = -1;
    toscreen << "Tasks in queue have finished. Start to execute stop task.\n";
    if (_data != nullptr) {
        _data->set_quit_strategy(clean_mem);
        delete _data;
        _data = nullptr;
        close(_listen_socket);
    }
    toscreen << "Server stopped.\n";
    return;
}

int SKServer::_cmp_skstring(const SKString& lhs, const SKString& rhs) {
    if (lhs.size != rhs.size) {
        return lhs.size - rhs.size;
    }
    uint16_t size = lhs.size;
    for (uint16_t i = 0; i < size; ++i) {
        if (lhs.data[i] != rhs.data[i]) {
            return lhs.data[i] - rhs.data[i];
        }
    }
    return 0;
}

std::string SKServer::_print_skstring(const SKString& para) {
    static char buffer[1025];
    if (para.size == 0) {
        return "{empty_content}";
    }
    memcpy(buffer, para.data, para.size - 1);
    buffer[para.size] = '\0';
    return buffer;
}

void* SKServer::_listen(void* skserver) {
    SKServer& server = *reinterpret_cast<SKServer*>(skserver);
    int empty_loops = 0; // Continuous invalid loops.
    while(!server._listen_stop.get()) {
        // Find next connection.
        int client_socket = client_socket = accept(server._listen_socket, nullptr, nullptr);
        if (client_socket < 0) {
            // No connection, continue finding next connection.
            ++empty_loops;
            if (empty_loops >= 30) {
                usleep(5e4);
                empty_loops = 0;
            }
            continue;
        }
    
        // Push the context to the handler queue.
        server._handler_queue.push(client_socket);
    }
    return nullptr;
}

void* SKServer::_handle(void* skserver) {
    // Read the parameters.
    SKServer& server = *(SKServer*)skserver;
    int client_socket = -1;
    int ret = -1;
    static Msg read_msg;
    static Msg write_msg;
    
    // Stop the loop when non-listening and the hander queue is empty.
    while ((ret = server._handler_queue.pop(client_socket)) != -1 
        || !server._listen_stop.get()) {
        if (ret == -1) {
            // No waitting request, sleep a while and continue.
            usleep(2e5);
            continue;
        }
        
        // Read the request.
        ret = read(client_socket, &read_msg, sizeof(Msg));
        if (ret != sizeof(Msg)) {
            toscreen << "Received invalid message, ignore.\n";
            static Msg message_wrong_binary(MSG_INVALID_REQ);
            write(client_socket, &message_wrong_binary, sizeof(Msg));
            close(client_socket);
            continue;
        }
        
        // Handle the request.
        write_msg.key = read_msg.key;
        write_msg.val = read_msg.val;
        if (read_msg.status == MSG_GET) {
            write_msg.status = server._data->get(read_msg.key, write_msg.val);
        } else if (read_msg.status == MSG_SET) {
            write_msg.status = server._data->set(read_msg.key, read_msg.val);
        } else if (read_msg.status == MSG_DEL) {
            write_msg.status = server._data->del(read_msg.key);
        }
        
        // Send response and close the socket.
        write(client_socket, &write_msg, sizeof(Msg));
        close(client_socket);
    }
    return nullptr;
}

} // End namespace sk_cs.