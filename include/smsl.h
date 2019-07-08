// SkipList implemented by shared_memory.
// It can only be used on Linux. Windows is unsupported.
// 6/28/2019, LiWentan.

#ifndef _SMSL_H_
#define _SMSL_H_

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#define toscreen std::cout<<__FILE__<<", "<<__LINE__<<": "

namespace {

const size_t DEFAULT_LEVEL = 32;
const size_t INITIALIZE_CAPACITY = 1;
const char* CHECKSUM_STRING = "012345678";

}; // End anoyomous namespace.

namespace smsl {

struct SmslData {
    char checksum[10]; // Check if this part is a created data.
    size_t length; // Elements numbers.
    size_t capacity; // The number of elements that the allocated memory can storage.
    size_t tail; // Position of the tail node.
    size_t level_capacity; // The allowed maximum levels.
    size_t level; // The maximum levels of all nodes.

    /**
     * Data structure:
     *     1. SmslNode[capacity + 1]:
     *           KeyType, ValType, size_t, SmslLevel[level_capacity].
     *     2. size_t space_status[capacity + 1].
     *     3. size_t next_free_space.
     *     4. size_t space_status_index[capacity + 1];
     */
    // This place storages the data.
};

struct SmslLevel {
    SmslLevel() : forward(0), span(0) {}
    size_t forward;
    size_t span;
};

template <typename KeyType, typename ValType>
struct SmslNode {
    SmslNode() : backward(0) {}
    KeyType key;
    ValType val;
    size_t backward;

    // This place storages the levels.

};

template <typename KeyType, typename ValType>
class Smsl {
public:
    /**
     * Construct function. 2 functions needed.
     * @param shm_path: Same shm_path will reflect the same data part.
     */
    Smsl(const std::string& shm_path, int (*cmp_fun)(const KeyType&, const KeyType&),
        std::string (*key_to_str)(const KeyType&), bool resume = true, int level_in = DEFAULT_LEVEL);
    virtual ~Smsl();

    /**
     * Set.
     * Return 0 success, -1 failed, 1 already existing.
     */
    int set(const KeyType& key, const ValType& value);

    /**
     * Get.
     * Return the steps between the beginning to the key.
     * If not existing, return -1.
     * If not successful, variable val won't be modified.
     */
    int get(const KeyType& key, ValType& val);

    /**
     * Delete.
     * Return 0 means success, -1 means unexisting key.
     */
    int del(const KeyType& key);

    /**
     * Return the elements numbers.
     */
    size_t size() {
        return _data->length;
    }

    /**
     * Set if cleaning the shared memory when destructing the skiplist.
     */
    void set_quit_strategy(bool flag) {
        _quit_clean = flag;
    }

private:
    SmslData* _data; // The data position.
    int (*_cmp)(const KeyType&, const KeyType&); // Compare function, used for sorting.
    std::string (*_key2str)(const KeyType&); // Function to show the key.
    std::string _shmpath; // The path of the shared_memory.
    int _shmid; // The id of the shared_memory.
    bool _quit_clean; // If true, it will free the shared memory at distruction method.

    /**
     * Functions to find the correct pointer in data.
     * If allowed, it is recommended that these functions be inline functions.
     * These functions will be called frequently, inline is better.
     */
    SmslNode<KeyType, ValType>*  _get_node(size_t node_pos);
    SmslLevel* _get_level(size_t node_pos, size_t level_num);
    SmslLevel* _get_level(SmslNode<KeyType, ValType>* node, size_t level_num);

    /**
     * Functions to allocate and deallocate a node.
     * Function: _allocate_new_space considers the space is enough.
     * It won't check whether the shared_memory space has margin.
     */
    size_t* _get_space_status();
    size_t* _get_next_free_space();
    size_t* _get_space_status_index();
    size_t _allocate_new_space(const KeyType&, const ValType&, size_t backward);
    int _expansion();
    void _free_node(size_t node_pos);

    /**
     * Tool functions.
     */
    size_t _random_level();
};

} // End namespace smsl.

#endif // End ifndef _SMSL_H_.
