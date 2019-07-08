// 6/26/2019.
// Author: LiWentan.
// SafeSL(SafeSkipList).
// Core dump safe, unpluging the power safe.

#ifndef _SAFESL_HPP_
#define _SAFESL_HPP_

#include <stdio.h>
#include <time.h>
#include <iostream>
#include "safesl.h"

namespace {
    
char buffer[1024 * 1024 * 100]; // 100 MB.
    
} // End anoyomous namespace.

namespace skiplist {

template <typename KeyType, typename ValType>
SafeSL<KeyType, ValType>::SafeSL(int (*cmp_fun)(const KeyType&, const KeyType&), 
    std::string (*key_to_str)(const KeyType&), 
    void (*convert_key_to_bin)(const KeyType&, Binary& bin_data),
    void (*convert_val_to_bin)(const ValType&, Binary& bin_data),
    void (*parse_key_from_bin)(KeyType&, const Binary& bin_data),
    void (*parse_val_from_bin)(ValType&, const Binary& bin_data),
    const std::string &log_path_in, int level_in) :
    SkipList<KeyType, ValType>(cmp_fun, key_to_str, level_in), 
    bin2key(parse_key_from_bin), bin2val(parse_val_from_bin), 
    key2bin(convert_key_to_bin), val2bin(convert_val_to_bin),
    _log_path(log_path_in) {
    _log_pointer = fopen(_log_path.c_str(), "ab+");
    if (_log_pointer == nullptr) {
        toscreen << "Initializing SafeSL failed. Cannot open the log file.\n";
    }
}

template <typename KeyType, typename ValType>
SafeSL<KeyType, ValType>::~SafeSL() {
    if (_log_pointer != nullptr) {
        fclose(_log_pointer);
    }
}

template <typename KeyType, typename ValType>
void SafeSL<KeyType, ValType>::land_log() {
    fflush(_log_pointer);
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::safe_get(const KeyType& key, ValType& val) {
    return SkipList<KeyType, ValType>::get(key, val);
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::safe_set(const KeyType& key, const ValType& val) {
    int ret = SkipList<KeyType, ValType>::set(key, val);
    if (ret == 0) {
        _write_to_log(TAG_SET, key, val);
    }
    return ret;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::safe_del(const KeyType& key) {
    int ret = SkipList<KeyType, ValType>::del(key);
    if (ret == 0) {
        static ValType empty_val;
        _write_to_log(TAG_DEL, key, empty_val);
    }
    return ret;
}

template <typename KeyType, typename ValType>
void SafeSL<KeyType, ValType>::_write_to_log(
    Tags tag, const KeyType& key, const ValType& val) {
    static int settag = static_cast<int>(TAG_SET);
    static int deltag = static_cast<int>(TAG_DEL);
    
    // Write time.
    long cur_time = time(0);
    if (fwrite(&cur_time, sizeof(long), 1, _log_pointer) != 1) {
        toscreen << "Write time to log failed.\n";
        return;
    }
    
    if (tag == TAG_SET) {
        if (fwrite(&settag, sizeof(int), 1, _log_pointer) != 1) {
            toscreen << "Write operation type failed.\n";
            return;
        }
        if (_write_key(_log_pointer, key) != 0) {
            toscreen << "Write key failed.\n";
            return;
        }
        if (_write_val(_log_pointer, val) != 0) {
            toscreen << "Write val failed.\n";
            return;
        }
    } else if (tag == TAG_DEL) {
        if (fwrite(&deltag, sizeof(int), 1, _log_pointer) != 1) {
            toscreen << "Write operation type failed.\n";
            return;
        }
        if (_write_key(_log_pointer, key) != 0) {
            toscreen << "Write key failed.\n";
            return;
        }
    } else {
        toscreen << "Unknown operation type when writting to log: " << tag << ".\n";
    }
    return;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::restore(
    const std::string& log_file, 
    const std::string& dump_file) {
        
    int ret = 0;
    long last_dump_time = 0;
    FILE *dump = nullptr;
    
    // Open dump file, get the last_dump_time.
    // Restore the status at last_dump_time.
    if (dump_file != "NOFILE") {
        dump = fopen(dump_file.c_str(), "rb");
        if (dump == nullptr) {
            toscreen << "Dump file: " << dump_file << " cannot be opened.\n";
            fclose(dump);
            return -1;
        }
        ret = fread(buffer, sizeof(long), 1, dump);
        if (ret != 1) {
            toscreen << "Dump file: " << dump_file << " has wrong format.\n";
            fclose(dump);
            return -1;
        }
        memcpy(&last_dump_time, buffer, sizeof(long));
        _parse_from_file(dump);
    }
    
    // According to the logfile, modify the skiplist.
    FILE* log = fopen(log_file.c_str(), "rb");
    if (log == nullptr) {
        toscreen << "Logfile: " << log_file << " cannot be opened.\n";
    }
    
    // Buffers.
    long log_time;
    KeyType key_buffer;
    ValType val_buffer;
    int operation_buffer;
    long valid_log_start = -1; // Logs before this position are expired logs.
    unsigned long valid_datas = 0;
    unsigned long handled_lines = 0;
    
    // Read logs one by one.
    while (fread(&log_time, sizeof(long), 1, log) == 1) {
        ++handled_lines;
        static bool do_this_log;
        if (log_time <= last_dump_time) { // If this log is expired, do not manipulate by this log.
            do_this_log = false;
        } else {
            do_this_log = true;
        }
        
        if (do_this_log && valid_log_start == -1) {
            // The log is valid from here to the end.
            // Logs before this position are expired.
            valid_log_start = ftell(log) - sizeof(long);
        }
        
        if (do_this_log) {
            ++valid_datas;
        }
        
        // Read operation type.
        if (fread(&operation_buffer, sizeof(int), 1, log) != 1) {
            toscreen << "Read the operation type failed.\n";
            fclose(dump);
            fclose(log);
            return -1;
        }
        
        // Manpulate according to the operation type.
        if (operation_buffer == TAG_SET) {
            // Read Key.
            if (_read_key(log, key_buffer) != 0) {
                fclose(dump);
                fclose(log);
                return -1;
            }
            
            // Read Val.
            if (_read_val(log, val_buffer) != 0) {
                fclose(dump);
                fclose(log);
                return -1;
            }
            
            // Call set.
            // It should not be fail. Since only successful operation woudle be written to log.
            if (do_this_log && SkipList<KeyType, ValType>::set(key_buffer, val_buffer) != 0) {
                toscreen << "Set when restore failed. Key: " << key_buffer
                    << " Val: " << val_buffer << ".\n";
            }
        } else if (operation_buffer == TAG_DEL) {
            // Read Key.
            if (_read_key(log, key_buffer) != 0) {
                fclose(dump);
                fclose(log);
                return -1;
            }
            
            // Call del.
            if (do_this_log && SkipList<KeyType, ValType>::del(key_buffer) != 0) {
                toscreen << "Del when restore failed. Key: " << key_buffer << ".\n";
            }
        } else {
            toscreen << "Unknown log operation type: " << operation_buffer << ". Stop.\n";
            --valid_datas;
            fclose(dump);
            fclose(log);
            return -1;
        }
    }
    
    // Remove the unvalid logs from the log file.
    // Close the file.
    if (valid_log_start == -1) { // All logs are invalid, clean the log file.
        fclose(log);
        log = fopen(log_file.c_str(), "wb");
        if (log != nullptr) {
            fclose(log);
        }
    } else { // Some part of logs is valid, clean the invalid part.
        fseek(log, 0L, SEEK_END);
        long file_size = ftell(log);
        fseek(log, valid_log_start, SEEK_SET);
        if (fread(buffer, file_size - valid_log_start, 1, log) == 1) {
            fclose(log);
            log = fopen(log_file.c_str(), "wb");
            if (log != nullptr) {
                fwrite(buffer, file_size - valid_log_start, 1, log);
                fclose(log);
            } else {
                toscreen << "Write to log file failed.\n";
            }
        } else {
            toscreen << "Valid logs part read error.\n";
        }
    }        
    
    fclose(dump);
    fclose(log);
    toscreen << "Restore finished. Read: " << handled_lines << " operations."
        << " Valid operation num: " << valid_datas << ".\n";
    return 0;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::_read_key(FILE* file, KeyType& key) {
    static Binary bin_buffer; // Tag of this Binary is TAG_POINTER.
    // Read the size of key.
    if (fread(&bin_buffer.bytes, sizeof(size_t), 1, file) != 1) { // File is OVER.
        return 1;
    }
    // Read the content of key.
    if (fread(buffer, bin_buffer.bytes, 1, file) != 1) {
        toscreen << "Read the key failed.\n";
        return -1;
    }
    bin_buffer.data = buffer;
    // Convert the binary to key.
    bin2key(key, bin_buffer);
    return 0;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::_read_val(FILE* file, ValType& val) {
    static Binary bin_buffer; // Tag of this Binary is TAG_POINTER.
    // Read the size of val.
    if (fread(&bin_buffer.bytes, sizeof(size_t), 1, file) != 1) { // File is OVER.
        return 1;
    }
    // Read the content of val.
    if (fread(buffer, bin_buffer.bytes, 1, file) != 1) {
        toscreen << "Read the val failed.\n";
        return -1;
    }
    bin_buffer.data = buffer;
    // Convert the binary to key.
    bin2val(val, bin_buffer);
    return 0;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::_write_key(FILE* file, const KeyType& key) {
    static Binary bin_buffer; // Tag of this Binary is TAG_COPY.
    key2bin(key, bin_buffer);
    if (fwrite(&bin_buffer.bytes, sizeof(size_t), 1, file) != 1 ||
        fwrite(bin_buffer.data, bin_buffer.bytes, 1, file) != 1) {
        toscreen << "Write key failed.\n";
        return -1;
    }
    return 0;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::_write_val(FILE* file, const ValType& val) {
    static Binary bin_buffer; // Tag of this Binary is TAG_COPY.
    val2bin(val, bin_buffer);
    if (fwrite(&bin_buffer.bytes, sizeof(size_t), 1, file) != 1 ||
        fwrite(bin_buffer.data, bin_buffer.bytes, 1, file) != 1) {
        toscreen << "Write val failed.\n";
        return -1;
    }
    return 0;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::dump_to_file(const std::string& dump_path) {
    FILE* dump = fopen(dump_path.c_str(), "wb");
    if (dump == nullptr) {
        toscreen << "Cannot open the dump file, dump failed: " << dump_path << ".\n";
        return -1;
    }
    long cur_time = time(0);
    if (fwrite(&cur_time, sizeof(long), 1, dump) != 1) {
        toscreen << "Write current time failed. Cannot dump to file.\n";
        fclose(dump);
        return -1;
    }
    long dump_num = 0;
    for (Node<KeyType, ValType>* x = SkipList<KeyType, ValType>::_head; x != nullptr; x = x->levels[0].forward) {
        if (x == SkipList<KeyType, ValType>::_head) {
            continue;
        }
        if (_write_record(dump, x->key, x->val) != 0) {
            toscreen << "Write record failed.\n";
            fclose(dump);
            return -1;
        }
        ++dump_num;
    }
    
    if (dump_num != SkipList<KeyType, ValType>::_length) {
        toscreen << "Dump number unmatched.\n";
        fclose(dump);
        return -1;
    }
    fclose(dump);
    toscreen << "Dump to file finished. Totally dump " << dump_num << " nodes.\n";
    return 0;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::_write_record(FILE* file, const KeyType& key, const ValType& val) {
    if (_write_key(file, key) != 0) {
        return -1;
    }
    if (_write_val(file, val) != 0) {
        return -1;
    }
    return 0;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::_read_record(FILE* file, KeyType& key, ValType& val) {
    static int ret = -10;
    
    ret = _read_key(file, key);
    if (ret == -1) { // Read error.
        return -1;
    } else if (ret == 1) { // Finish read.
        return 1;
    }
    
    if (_read_val(file, val) != 0) { // Read error.
        return -1;
    }
    
    return 0; // Read OK, and the file has more content.
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::parse_from_file(const std::string& dump_path) {
    FILE* dump = fopen(dump_path.c_str(), "rb");
    if (dump == nullptr) {
        toscreen << "Cannot open the dump file: " << dump_path << ", parse from file failed.\n";
        return -1;
    }
    if (fseek(dump, sizeof(long), SEEK_SET) != 0) { // Locate to data part.
        toscreen << "Locate after the time of dump file failed.\n";
        return -1;
    }
    
    int ret = _parse_from_file(dump);
    fclose(dump);
    return ret;
}

template <typename KeyType, typename ValType>
int SafeSL<KeyType, ValType>::_parse_from_file(FILE* file) {
    if (file == nullptr) {
        toscreen << "Fun: _parse_from_file received an empty file.\n";
        return -1;
    }
    static KeyType key_buffer;
    static ValType val_buffer;
    static int ret;
    static long record_num;
    
    record_num = -1;
    for (ret = 0; ret == 0; ret = _read_record(file, key_buffer, val_buffer)) {
        if (record_num == -1) { // The first loop.
            ++record_num;
            continue;
        }
        if (SkipList<KeyType, ValType>::set(key_buffer, val_buffer) != 0) {
            toscreen << "Set data failed when parsing from file.\n";
            return -1;
        }
        ++record_num;
    }
    
    if (ret != 1) { // Ret == 1 means finishing reading the dump file.
        toscreen << "Parse from file failed.\n";
        return -1;
    }
    
    toscreen << "Parse from file finish. Total records num: " << record_num << ".\n";
    return 0;
}

} // End namespace skiplist.

#endif // End ifndef _SAFESL_HPP_.

