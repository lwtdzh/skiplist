// 6/26/2019.
// Author: LiWentan.
// SafeSL(SafeSkipList).
// Core dump safe, unpluging the power safe.

#ifndef _SAFESL_H_
#define _SAFESL_H_

#include "skiplist.hpp"
#include <iostream>
#include <malloc.h>
#include <stdlib.h>

namespace skiplist {

enum Tags {
    TAG_COPY,
    TAG_POINTER,
    TAG_SET,
    TAG_DEL
};

struct Binary {
private:
    Binary(const Binary&);
    Binary& operator=(const Binary&);
public:
    size_t bytes;
    void *data;
    Tags tag;
    
    Binary() : tag(TAG_POINTER), data(nullptr) {}

    // Setting data will clean the former data.
    void set_data(size_t bytes_in, const void *data_in, Tags tag_in = TAG_COPY) {
        // Free the existing data.
        if (tag == TAG_COPY && data != nullptr) {
            free(data);
            data = nullptr;
        }
        
        // Set new data.
        tag = tag_in;
        bytes = bytes_in;
        if (tag == TAG_COPY) {
            // Copy the source, safe but slow.
            data = malloc(bytes);
            if (data == nullptr) {
                toscreen << "Memory error when allocating space for Binary.\n";
            }
            if (data_in != nullptr) {
                memcpy(data, data_in, bytes);
            }
            return;
        }
        if (tag == TAG_POINTER) {
            // Only stroage the pointer to the source, quick but requiring the source persistent.
            data = const_cast<void*>(data_in);
        }
    }
    
    ~Binary() {
        if (tag == TAG_COPY && data != nullptr) {
            free(data);
        }
    }
};

template <typename KeyType, typename ValType>
class SafeSL : protected SkipList<KeyType, ValType> {
public:
    /**
     * To use this class, you must assign 6 functions:
     * 0. Compare the size of two keys.
     * 1. Convert key to string.
     * 2. Convert key to binary.
     * 3. Convert val to binary.
     * 4. Parse key from binary.
     * 5. Parse val from binary.
     **/
    SafeSL(int (*cmp_fun)(const KeyType&, const KeyType&), 
        std::string (*key_to_str)(const KeyType&), 
        void (*convert_key_to_bin)(const KeyType&, Binary& bin_data),
        void (*convert_val_to_bin)(const ValType&, Binary& bin_data),
        void (*parse_key_from_bin)(KeyType&, const Binary& bin_data),
        void (*parse_val_from_bin)(ValType&, const Binary& bin_data),
        const std::string &log_path_in = "log",
        int level_in = DEFAULT_LEVEL);
    virtual ~SafeSL();
    
    // The interface to provide safely manipulating the data in skiplist.
    int safe_get(const KeyType& key, ValType& val);
    int safe_set(const KeyType& key, const ValType& val);
    int safe_del(const KeyType& key);
    
    // Save all data to a file.
    // Call this function will cause the skiplist unused during processing.
    // Return 0 means success.
    int dump_to_file(const std::string& dump_path);
    
    // Append all data from a file.
    // Call this function will cause the skiplist unused during processing.
    // Return 0 means successful.
    int parse_from_file(const std::string& dump_path);

    // Restore from an accident, e.g., process stop, power off or etc.
    // You must give the last dumpped file and logfile.
    // If you have not dumpped data to file, you just need give the logfile.
    int restore(const std::string& log_file, const std::string& dump_file = "NOFILE");
    
    // Write the log from buffer to file.
    // If using multi-thread, it can set time-interval to automatially do this.
    void land_log();
private:
    std::string (*key2str)(const KeyType&);
    void (*key2bin)(const KeyType&, Binary& bin_data);
    void (*val2bin)(const ValType&, Binary& bin_data);
    void (*bin2key)(KeyType&, const Binary& bin_data);
    void (*bin2val)(ValType&, const Binary& bin_data);
    
    // This function need a FILE pointer, and the position is after the last_update_time.
    // This function won't close the file.
    int _parse_from_file(FILE* file);
    
    // Read the correct data from file.
    // Append the record to the file.
    // File must at position begin with the correct data.
    // It will move the file position after the correct data.
    int _read_key(FILE* file, KeyType& key); // Return 1 means the file is over. 0 means OK, 1 means error.
    int _read_val(FILE* file, ValType& val);
    int _write_key(FILE* file, const KeyType& key);
    int _write_val(FILE* file, const ValType& val);
    
    // Write or read the key and val to the file or from the file.
    int _write_record(FILE* file, const KeyType& key, const ValType& val);
    int _read_record(FILE* file, KeyType& key, ValType& val); // Return 1 means the file is over. 0 means OK, 1 means error.
    
    // Write the operation to log file.
    void _write_to_log(Tags tag, const KeyType& key, const ValType& val);
    
    // Log file pointer and path.
    FILE* _log_pointer;
    std::string _log_path;
    
};

}


#endif // End ifndef _SAFESL_H_.
