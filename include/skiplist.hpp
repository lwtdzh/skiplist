// Author: LiWentan.
// Date: 6/25/2019.
// SkipList in Shared Memory.

#ifndef _SKIPLIST_HPP_
#define _SKIPLIST_HPP_

#include <cstdlib>
#include <ctime>
#include <iostream>
#include "skiplist.h"

namespace skiplist {

// Functions of Level.

// Functions of Node.
template <typename KeyType, typename ValType>
Node<KeyType, ValType>::Node(int level_in, Node* backward_in,
    const KeyType& key_in, const ValType& val_in)
    : backward(backward_in), key(key_in), val(val_in) {
    levels = new(std::nothrow) Level<KeyType, ValType>[level_in]; // Allocate level size's levels.
    if (levels == nullptr) {
        toscreen << "Allocate for new levels failed.\n";
    }
}

template <typename KeyType, typename ValType>  
Node<KeyType, ValType>::Node(int level_in) {
    levels = new(std::nothrow) Level<KeyType, ValType>[level_in]; // Allocate level size's levels.
    if (levels == nullptr) {
        toscreen << "Allocate for new levels failed.\n";
    }
    backward = nullptr; // No back node.
}

template <typename KeyType, typename ValType>  
Node<KeyType, ValType>::~Node() {
    if (levels != nullptr) {
        delete[] levels;
    }
}


// Functions of SkipList.
template <typename KeyType, typename ValType>  
SkipList<KeyType, ValType>::SkipList(int (*cmp_fun)(const KeyType&, const KeyType&), 
    std::string (*key_to_str)(const KeyType&), int level_in) : 
    _tostr(key_to_str), _cmp(cmp_fun), _level_capacity(level_in) {
    _length = 0; // Has 0 nodes in total.
    _level = 1; // The head node has 1 level.
    
    // Construct the head node.
    // This node has _level_capacity levels and all levels are empty.
    _head = new(std::nothrow) Node<KeyType, ValType>(_level_capacity);
    if (_head == nullptr) {
        toscreen << "Cannot initialize skiplist, allocating head failed.\n";
    }
    
    // No tail.
    _tail = nullptr;
}

template <typename KeyType, typename ValType>  
SkipList<KeyType, ValType>::~SkipList() {
    if (_tail == nullptr && _head != nullptr) {
        // No element, only free the head.
        delete _head;
        return;
    }
    if (_tail != nullptr) {
        Node<KeyType, ValType>* cur = _tail;
        Node<KeyType, ValType>* next = nullptr;
        while (cur != nullptr) {
            next = cur->backward;
            delete cur;
            cur = next;
        }
    }
    if (_head != nullptr) {
        delete _head;
    }
}

template <typename KeyType, typename ValType>
int SkipList<KeyType, ValType>::set(const KeyType& key, const ValType& value) {
    // At the level n, it should pass node update[n] to reach the key.
    Node<KeyType, ValType>* update[_level_capacity];
    
    // To reach update[n], it need walk rank[n] steps.
    int rank[_level_capacity];
    
    // Temporary pointer.
    Node<KeyType, ValType>* x = _head;
    
    // Search from top level to the 0th level.
    for (int i = _level - 1; i >= 0; --i) {
        // Set rank[i].
        if (i == _level - 1) {
            rank[i] = 0;
        } else {
            rank[i] = rank[i + 1];
        }
        
        // Search. Until the x->levels[i].key is the last one that smaller than value.
        // Or the x is the last one in this level.
        while (x->levels[i].forward != nullptr && 
            _cmp(x->levels[i].forward->key, key) <= 0) {
            if (_cmp(x->levels[i].forward->key, key) == 0) {
                // This skiplist already has this key.
                toscreen << "Key: " << _tostr(key) << " already exists, set key failed.\n";
                return 1;
            }
            rank[i] += x->levels[i].span;
            x = x->levels[i].forward;
        }
        
        // At level i, the search path passes node x.
        update[i] = x;
    }
    
    // Randomly find a level, this time, we construt new index below this level.
    int new_node_level = _random_level();
    if (_level < new_node_level) {
        // This level is bigger than the maximum table now.
        for (int i = _level; i < new_node_level; ++i) {
            rank[i] = 0;
            update[i] = _head;
            
            // Set head->levels[i].span as _length.
            // This would be used when setting x->levels[i].span.
            _head->levels[i].span = _length;
        }
        _level = new_node_level;
    }
    
    // Generate new node for stroaging this key.
    x = new(std::nothrow) Node<KeyType, ValType>(_level_capacity, nullptr, key, value);
    if (x == nullptr) {
        toscreen << "Insert key: " << _tostr(key) << "failed since allocating memory failed.\n";
        return -1;
    }
    
    // Set the forward pointer of x.
    for (int i = 0; i < new_node_level; ++i) {
        x->levels[i].forward = update[i]->levels[i].forward;
        update[i]->levels[i].forward = x;
        x->levels[i].span = update[i]->levels[i].span - (rank[0] - rank[i]);
        update[i]->levels[i].span = rank[0] - rank[i] + 1;
    }
    
    // Node connecting to the new node and 
    // with a level in [new_node_level, _level] has a wider span.
    // Since the new node was added within the span.
    for (int i = new_node_level; i < _level; ++i) {
        ++update[i]->levels[i].span;
    }
    
    // Set the backward of the new node as the former node.
    x->backward = (update[0] == _head) ? nullptr : update[0];
    if (x->levels[0].forward != nullptr) {
        x->levels[0].forward->backward = x;
    } else {
        _tail = x;
    }
    
    ++_length;
    
    return 0;
}

template <typename KeyType, typename ValType>
int SkipList<KeyType, ValType>::get(const KeyType& key, ValType& val) {
    // Temporary pointer.
    Node<KeyType, ValType>* x = _head;
    
    // The steps from beginning to the key.
    int rank = 0;
    
    // Traverse each level.
    for (int i = _level - 1; i >= 0; --i) {
        while (x->levels[i].forward != nullptr &&
            _cmp(x->levels[i].forward->key, key) <= 0) {
            rank += x->levels[i].span; // Update the steps.
            if (_cmp(x->levels[i].forward->key, key) == 0) {
                // Founded.
                val = x->levels[i].forward->val;
                return rank;
            }
            x = x->levels[i].forward;
        }
    }
    
    // Not found.
    return -1;
}

template <typename KeyType, typename ValType>
int SkipList<KeyType, ValType>::del(const KeyType& key) {
    Node<KeyType, ValType>* update[_level_capacity]; // Record the path to the key at each level.
    Node<KeyType, ValType>* x = _head; // Temporary node.
    for (int i = _level; i >= 0; --i) {
        while (x->levels[i].forward != nullptr && 
            _cmp(x->levels[i].forward->key, key) < 0) {
            x = x->levels[i].forward;    
        }
        update[i] = x;
    }
    x = x->levels[0].forward;
    if (x == nullptr || _cmp(x->key, key) != 0) {
        // Not found.
        return -1;
    }
    // Found the key.
    // Update the former node.
    for (int i = 0; i < _level; ++i) {
        if (update[i]->levels[i].forward == x) {
            update[i]->levels[i].span += x->levels[i].span - 1;
            update[i]->levels[i].forward = x->levels[i].forward;
        } else {
            update[i]->levels[i].span -= 1;
        }
    }
    // Update the later node.
    if (x->levels[0].forward != nullptr) {
        x->levels[0].forward->backward = x->backward;
    } else {
        _tail = x->backward;
    }
    // Update the length.
    --_length;
    // Free the memory.
    delete x;
    return 0;
}

template <typename KeyType, typename ValType>
int SkipList<KeyType, ValType>::_random_level() {
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        std::srand(std::time(nullptr));
    }
    int res = 1;
    while ((std::rand() % 2 == 0) && res < _level_capacity) {
        ++res;
    }
    return res;
}

} // End namespace skiplist.

#endif // End ifndef _SKIPLIST_HPP_.
