// SkipList implemented by shared_memory.
// It can only be used on Linux. Windows is unsupported.
// 6/28/2019, LiWentan.

#ifndef _SMSL_HPP_
#define _SMSL_HPP_

#include "smsl.h"
#include <ctime>

namespace smsl {

template <typename KeyType, typename ValType>
Smsl<KeyType, ValType>::~Smsl() {
    if (_shmid != -1 && _quit_clean) {
        toscreen << "Clean the shared_memory.\n";
        shmctl(_shmid, IPC_RMID, nullptr);
    }
}

template <typename KeyType, typename ValType>
Smsl<KeyType, ValType>::Smsl(const std::string& shm_path,
    int (*cmp_fun)(const KeyType&, const KeyType&),
    std::string (*key_to_str)(const KeyType&),
    bool resume, int level_in) :
    _cmp(cmp_fun), _key2str(key_to_str), _shmpath(shm_path), 
    _quit_clean(false), _shmid(-1), _data(nullptr) {
    // Get or create the shared_memory.
    key_t shm_key = ftok(shm_path.c_str(), 666);
    if (shm_key == (key_t)-1) {
        toscreen << "Get shm_key failed.\n";
        return;
    } else {
        toscreen << "Got shm_key: " << (int)shm_key << ".\n";
    }
    size_t node_bytes = sizeof(SmslNode<KeyType, ValType>) + sizeof(SmslLevel) * level_in;
    size_t total_bytes = sizeof(SmslData) + node_bytes * (INITIALIZE_CAPACITY + 1);
    total_bytes += sizeof(size_t) * (INITIALIZE_CAPACITY * 2 + 3);
    _shmid = shmget(shm_key, total_bytes, IPC_CREAT | 0666); // total_bytes is the minimum space needed.
    if (_shmid == -1) {
        toscreen << "Get shmid failed.\n";
        perror("shmget");
        return;
    } else {
        toscreen << "Got shmid: " << _shmid << ".\n";
    }
    _data = (SmslData*)shmat(_shmid, (void*)0, 0);
    if (_data == (void*)-1) {
        toscreen << "Get logic data address failed.\n";
        perror("shmat");
        return;
    }

    // Check if the shared memory is a created data.
    _data->checksum[9] = '\0';
    if (resume && strcmp(_data->checksum, CHECKSUM_STRING) == 0) {
        toscreen << "Successfully initializing from existing skiplist. "
                  << "Elements num: " << _data->length << ".\n";
        return;
    }

    // Format the memory.
    strncpy(_data->checksum, CHECKSUM_STRING, 9);
    _data->length = 0;
    _data->level = 1;
    _data->level_capacity = level_in;
    _data->tail = 0;
    _data->capacity = INITIALIZE_CAPACITY;
    // Initialize the head node.
    SmslNode<KeyType, ValType>* head = _get_node(0);
    head->backward = 0;
    SmslLevel* head_levels = _get_level(head, 0);
    new(head_levels) SmslLevel[level_in];
    // Initialize the space allocator info.
    for (size_t i = 0; i <= INITIALIZE_CAPACITY; ++i) {
        _get_space_status()[i] = i;
        _get_space_status_index()[i] = i;
    }
    (*_get_next_free_space()) = 1;

    toscreen << "Successfully initializing a new skiplist.\n";
}

template <typename KeyType, typename ValType>
int Smsl<KeyType, ValType>::set(const KeyType& key, const ValType& value) {
    size_t update[_data->level_capacity];
    size_t rank[_data->level_capacity];
    size_t x = 0;

    if (_data->length == _data->capacity) {
        if (_expansion() != 0) {
            toscreen << "The memory cannot storage more nodes, set failed.\n";
            return -2;
        }
    }

    // Find the path to reach the key at each level.
    for (int64_t i = _data->level - 1; i >= 0; --i) {
        if (i == _data->level - 1) {
            rank[i] = 0;
        } else {
            rank[i] = rank[i + 1];
        }

        // Search until the i th level's node has a bigger key.
        while (_get_level(x, i)->forward != 0) {
            SmslNode<KeyType, ValType>* forward_node = _get_node(_get_level(x, i)->forward);
            int cmp_res = _cmp(forward_node->key, key);
            if (cmp_res == 0) {
                // Already having the key.
                return 1;
            }
            if (cmp_res > 0) {
                // X's forward is out range, do not pass by x's forward.
                break;
            }
            rank[i] += _get_level(x, i)->span;
            x = _get_level(x, i)->forward;
        }

        // Find the node, where it should pass to reach the key at level i.
        update[i] = x;
    }

    // Generate a level behind which the new index be constructed.
    size_t new_node_level = _random_level();
    if (_data->level < new_node_level) {
        for (size_t i = _data->level; i < new_node_level; ++i) {
            rank[i] = 0;
            update[i] = 0;
            _get_level((size_t)0, i)->span = _data->length;
        }
        _data->level = new_node_level;
    }

    // Put the new node at the correct place.
    x = _allocate_new_space(key, value, 0);
    toscreen << "New element pos: " << x << std::endl;
    for (size_t i = 0; i < new_node_level; ++i) {
        _get_level(x, i)->forward = _get_level(update[i], i)->forward;
        _get_level(update[i], i)->forward = x;
        _get_level(x, i)->span = _get_level(update[i], i)->span - (rank[0] - rank[i]);
        _get_level(update[i], i)->span = rank[0] - rank[i] + 1;
    }
    for (size_t i = new_node_level; i < _data->level; ++i) {
        ++_get_level(update[i], i)->span;
    }
    _get_node(x)->backward = (update[0] == 0) ? 0 : update[0];
    if (_get_level(x, 0)->forward != 0) {
        _get_node(_get_level(x, 0)->forward)->backward = x;
    } else {
        _data->tail = x;
    }
    ++_data->length;
    return 0;
}

template <typename KeyType, typename ValType>
int Smsl<KeyType, ValType>::del(const KeyType& key) {
    size_t update[_data->level_capacity];
    size_t x = 0;

    // Find the path to reach key.
    for (int64_t i = _data->level; i >=0; --i) {
        while (_get_level(x, i)->forward != 0) {
            SmslNode<KeyType, ValType>* forward_node = _get_node(_get_level(x, i)->forward);
            if (_cmp(forward_node->key, key) < 0) {
                x = _get_level(x, i)->forward;
            } else {
                break;
            }
        }
        update[i] = x;
    }
    x = _get_level(x, 0)->forward;
    if (x == 0 || _cmp(_get_node(x)->key, key) != 0) {
        // No this key.
        return -1;
    }

    // Update the former node of the key.
    for (size_t i = 0; i < _data->level; ++i) {
        if (_get_level(update[i], i)->forward == x) {
            _get_level(update[i], i)->span += _get_level(x, i)->span - 1;
            _get_level(update[i], i)->forward = _get_level(x, i)->forward;
        } else {
            _get_level(update[i], i)->span -= 1;
        }
    }

    // Update the later node of the key.
    if (_get_level(x, 0)->forward != 0) {
        _get_node(_get_level(x, 0)->forward)->backward = _get_node(x)->backward;
    } else {
        _data->tail = x;
    }

    // Update the length.
    --_data->length;

    // Free the memory of x.
    _free_node(x);
    return 0;
}

template <typename KeyType, typename ValType>
int Smsl<KeyType, ValType>::get(const KeyType& key, ValType& val) {
    size_t x = 0;
    int rank = 0;
    for (int64_t i = _data->level - 1; i >= 0 ; --i) {
        while (_get_level(x, i)->forward != 0) {
            SmslNode<KeyType, ValType>* forward_node = _get_node(_get_level(x, i)->forward);
            int cmp_res = _cmp(forward_node->key, key);
            if (cmp_res > 0) {
                // Try next level.
                break;
            }
            rank += _get_level(x, i)->span;
            if (cmp_res == 0) {
                // Found.
                val = _get_node(_get_level(x, i)->forward)->val;
                return rank;
            }
            x = _get_level(x, i)->forward;
        }
    }
    // Not found.
    return -1;
}

template <typename KeyType, typename ValType>
size_t* Smsl<KeyType, ValType>::_get_space_status() {
    char* pos = reinterpret_cast<char*>(_data);
    pos += sizeof(SmslData);
    size_t node_size = sizeof(SmslNode<KeyType, ValType>) + sizeof(SmslLevel) * _data->level_capacity;
    pos += node_size * (_data->capacity + 1);
    return reinterpret_cast<size_t*>(pos);
}

template <typename KeyType, typename ValType>
size_t* Smsl<KeyType, ValType>::_get_next_free_space() {
    char* pos = reinterpret_cast<char*>(_get_space_status());
    pos += sizeof(size_t) * (_data->capacity + 1);
    return reinterpret_cast<size_t*>(pos);
}

template <typename KeyType, typename ValType>
size_t* Smsl<KeyType, ValType>::_get_space_status_index() {
    char* pos = reinterpret_cast<char*>(_get_next_free_space());
    return reinterpret_cast<size_t*>(pos + sizeof(size_t));
}

template <typename KeyType, typename ValType>
size_t Smsl<KeyType, ValType>::_allocate_new_space(const KeyType& key, const ValType& val, size_t backward) {
    size_t* new_space = _get_next_free_space();
    size_t* space_status = _get_space_status();
    SmslNode<KeyType, ValType>* new_node = _get_node(space_status[*new_space]);
    new_node->key = key;
    new_node->val = val;
    new_node->backward = backward;
    new(_get_level(new_node, 0)) SmslLevel[_data->level_capacity];
    ++(*new_space);
    return space_status[*new_space - 1];
}

template <typename KeyType, typename ValType>
int Smsl<KeyType, ValType>::_expansion() {
    if (_shmid == -1) {
        toscreen << "No existing shared memory. Expansion failed.\n";
        return -1;
    }
    // Calculate current shared_memory size.
    size_t level_capacity = _data->level_capacity;
    size_t cur_capacity = _data->capacity;
    size_t node_size = sizeof(SmslNode<KeyType, ValType>) + sizeof(SmslLevel) * level_capacity;
    size_t shared_size = sizeof(SmslData);
    shared_size += (cur_capacity + 1) * node_size;
    shared_size += ((cur_capacity + 1) * 2 + 1) * sizeof(size_t);
    // Allocate a temporary memory to storage the data of shared_memory.
    char* temp_mem = (char*)malloc(shared_size);
    if (temp_mem == nullptr) {
        toscreen << "Cannot allocate temporary memory. Expansion failed.\n";
        return -1;
    }
    memcpy(temp_mem, _data, shared_size);
    // Clean the existing shared_memory.
    if (shmctl(_shmid, IPC_RMID, NULL) != 0) {
        toscreen << "Free the existing shared_memory failed. Expansion failed.\n";
        free(temp_mem);
        return -1;
    }
    // Calculate the new shared_memory size.
    size_t new_size = shared_size;
    new_size += cur_capacity * node_size;
    new_size += cur_capacity * 2 * sizeof(size_t);
    // Get new shared_memory.
    key_t shm_key = ftok(_shmpath.c_str(), 666);
    if (shm_key == (key_t)-1) {
        toscreen << "Get key failed. Expansion failed.\n";
        free(temp_mem);
        return -1;
    } else {
        toscreen << "Got key: " << (int)shm_key << ".\n";
    }
    _shmid = shmget(shm_key, new_size, IPC_CREAT | 0666);
    if (_shmid == -1) {
        // If you want to handle this emergency to save the data, write the logic here.
        toscreen << "Sign up for new shared_memory failed. Expansion failed and all data was lost.\n";
        free(temp_mem);
        return -1;
    }
    _data = (SmslData*)shmat(_shmid, (void*)0, 0);
    if (_data == nullptr) {
        toscreen << "Got the pointer of new shared_memory failed. Expansion failed and all data was lost.\n";
        free(temp_mem);
        return -1;
    }
    // Restore the temporary data to new shared_memory.
    char* des_p = reinterpret_cast<char*>(_data);
    char* src_p = temp_mem;
    memcpy(des_p, src_p, sizeof(SmslData) + node_size * (cur_capacity + 1));
    des_p += sizeof(SmslData) + node_size * (cur_capacity + 1) + node_size * cur_capacity;
    src_p += sizeof(SmslData) + node_size * (cur_capacity + 1);
    memcpy(des_p, src_p, sizeof(size_t) * (cur_capacity + 1));
    for (size_t i = cur_capacity + 1; i <= cur_capacity * 2; ++i) {
        (reinterpret_cast<size_t*>(des_p))[i] = i;
    }
    des_p += sizeof(size_t) * (cur_capacity * 2 + 1);
    src_p += sizeof(size_t) * (cur_capacity + 1);
    memcpy(des_p, src_p, sizeof(size_t) * (cur_capacity + 2));
    des_p += sizeof(size_t);
    for (size_t i = cur_capacity + 1; i <= cur_capacity * 2; ++i) {
        (reinterpret_cast<size_t*>(des_p))[i] = i;
    }
    _data->capacity = cur_capacity * 2;
    // Finish.
    free(temp_mem);
    toscreen << "Successfully expand the shared_memory, new capacity: " << cur_capacity * 2 << ".\n";
    return 0;
}

template <typename KeyType, typename ValType>
void Smsl<KeyType, ValType>::_free_node(size_t node_pos) {
    size_t* next_space = _get_next_free_space();
    size_t* space_status = _get_space_status();
    size_t* space_status_index = _get_space_status_index();
    size_t node_pos_index = space_status_index[node_pos];
    std::swap(space_status[node_pos_index], space_status[*next_space - 1]);
    space_status_index[node_pos] = *next_space - 1;
    space_status_index[space_status[node_pos]] = node_pos;
    --(*next_space);
}

template <typename KeyType, typename ValType>
SmslNode<KeyType, ValType>*  Smsl<KeyType, ValType>::_get_node(size_t node_pos) {
    char* pos = reinterpret_cast<char*>(&_data->level);
    pos += sizeof(size_t);
    size_t node_size = sizeof(SmslNode<KeyType, ValType>) + sizeof(SmslLevel) * _data->level_capacity;
    pos += node_pos * node_size;
    return reinterpret_cast<SmslNode<KeyType, ValType>*>(pos);
}

template <typename KeyType, typename ValType>
SmslLevel* Smsl<KeyType, ValType>::_get_level(size_t node_pos, size_t level_num) {
    return _get_level(_get_node(node_pos), level_num);
}

template <typename KeyType, typename ValType>
SmslLevel* Smsl<KeyType, ValType>::_get_level(SmslNode<KeyType, ValType>* node, size_t level_num) {
    char* pos = reinterpret_cast<char*>(node);
    pos += sizeof(SmslNode<KeyType, ValType>);
    return &(reinterpret_cast<SmslLevel*>(pos))[level_num];
}

template <typename KeyType, typename ValType>
size_t Smsl<KeyType, ValType>::_random_level() {
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        std::srand(std::time(nullptr));
    }
    int res = 1;
    while ((std::rand() % 2 == 0) && res < _data->level_capacity) {
        ++res;
    }
    return res;
}

} // End namespace smsl.


#endif // End ifndef _SMSL_HPP_.
