// Author: LiWentan.
// Date: 6/25/2019.
// SkipList.

#ifndef _SKIPLIST_H_
#define _SKIPLIST_H_

#include <string>

#define toscreen std::cout<<__FILE__<<", "<<__LINE__<<": "

namespace {

const int DEFAULT_LEVEL = 32;    

} // End anoyomous namespace.

namespace skiplist {

template <typename KeyType, typename ValType>
class Node;
template <typename KeyType, typename ValType>
class SkipList;

template <typename KeyType, typename ValType>
class Level {
public:
    Level() : forward(nullptr), span(0) {}
    Node<KeyType, ValType>* forward; // The next node pointer at this level.
    int span; // The distance between the node this level belonging to and the forward node.
};

template <typename KeyType, typename ValType>
class Node {
public:
    // @param level_in: The level of this node.
    // @param backward_in, key_in, val_in: The content of this node.
    Node(int level_in);
    Node(int level_in, Node* backward_in, 
        const KeyType& key_in, const ValType& val_in);
    ~Node();
    
    ValType val; // Value.
    KeyType key; // Key.
    Node* backward; // The backward pointer.
    Level<KeyType, ValType> *levels; // All levels.
};

template <typename KeyType, typename ValType>
class SkipList {
public:
    /**
     * @param level_in: All nodes will be allocated this number of levels.
     * @param cmp_fun, this function returns -1 means left < right, 0 means left == right, 1 means others.
     */
    SkipList(int (*cmp_fun)(const KeyType&, const KeyType&), 
        std::string (*key_to_str)(const KeyType&), int level_in = DEFAULT_LEVEL);

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
        return _length;
    }
    
    virtual ~SkipList();
    
protected:
    Node<KeyType, ValType>* _head; // Head node do not storage any (key, value).
    Node<KeyType, ValType>* _tail;
    int _length; // The number of nodes. Do not contain the head node.
    int _level; // The maximum length of level.
    int _level_capacity; // The allowed maximum length of level.
    int (*_cmp)(const KeyType&, const KeyType&); // Compare function, used for sorting.
    std::string (*_tostr)(const KeyType&); // Function to show the key.
    
    // Generate random level from 1 to _level_capacity.
    // Smaller number has more possibility to appear.
    int _random_level();
};

} // End namespace skiplist.

#endif // End ifndef _SKIPLIST_H_.
