// Test the SkipList safety edition with <string, string>.
// This is a demo showing how to use SafeSkipList.
// 6/27/2019.
// Author: LiWentan.

#include <iostream>
#include <string.h>
#include "../../include/safesl.hpp"

using namespace skiplist;
using namespace std;

namespace {

const unsigned int log_flush_interval = 1;

int cmp(const std::string& lhs, const std::string& rhs) {
    return strcmp(lhs.c_str(), rhs.c_str());
}

std::string tostr(const std::string& val) {
    return val;
}

std::string tostr(const int& val) {
    static char buffer_tostr[32];
    sprintf(buffer_tostr, "%d", val);
    return buffer_tostr;
}

void strtobin(const string& str, Binary& bin) {
    static size_t str_bytes;
    str_bytes = strlen(str.c_str()) * sizeof(char) + 1; // The last "\0" is included.
    bin.set_data(str_bytes, str.c_str(), TAG_COPY);
}

void bintostr(string& str, const Binary& bin) {
    str = reinterpret_cast<const char* const>(bin.data);
}

} // End anonoymous namespace.

int main(int argc, char **argv) {
    
    SafeSL<string, string> safesl(cmp, tostr, strtobin, strtobin, bintostr, bintostr);
    char para1[1024];
    char para2[1024];
    char para3[1024];
    int loop = 0;    
        
    cout << "Try to enter a command. To help, enter some arbitrary characters.\n\n";

    while (cin >> para1) {
        if (strcmp(para1, "set") == 0) {
            std::cin >> para2 >> para3;
            string key = para2;
            string val = para3;
            int ret = safesl.safe_set(key, val);
            if (ret == 0) {
                cout << "Success. Key: " << key << ", Value: " << val <<  ".\n";
            } else if (ret == 1){
                cout << "Already existing. Key: " << key << ".\n";
            } else {
                cout << "Failed.\n";
            }
        }
        
        else if (strcmp(para1, "get") == 0) {
            cin >> para2;
            string key = para2;
            string val = tostr(65536);
            int ret = safesl.safe_get(key, val);
            if (ret == -1) {
                cout << "Unexisting. Key: " << key << ".\n";
            } else {
                cout << "Found. Key: " << key << ", Value: " 
                    << val << ", Step: " << ret << ".\n";
            }
        }
        
        else if (strcmp(para1, "del") == 0) {
            cin >> para2;
            string key = para2;
            int ret = safesl.safe_del(key);
            if (ret == -1) {
                cout << "Unexisting. Key: " << key << ".\n";
            } else {
                cout << "Success. Key: " << key << ".\n";
            }
        }
        
        else if (strcmp(para1, "dump") == 0) {
            cin >> para2;
            string file = para2;
            int ret = safesl.dump_to_file(file);
            if (ret == 0) {
                cout << "Success.\n";
            } else {
                cout << "Failed.\n";
            }
        }
        
        else if (strcmp(para1, "restore") == 0) {
            cin >> para2;
            cin >> para3;
            string log = para2;
            string dump = para3;
            int ret;
            if (dump == "null") {
                ret = safesl.restore(log);
            } else {
                ret = safesl.restore(log, dump);
            }
            if (ret == 0) {
                cout << "Success.\n";
            } else {
                cout << "Failed.\n";
            }
        }
        
        else if (strcmp(para1, "parse") == 0) {
            cin >> para2;
            string dump = para2;
            int ret = safesl.parse_from_file(dump);
            if (ret == 0) {
                cout << "Success.\n";
            } else {
                cout << "Failed.\n";
            }
        }
        
        else {
        
            cout << "Unknown command: " << para1 << ".\n\n" << endl;
            cout << "**************\n"
                 << "Support command: \n" 
                 << "set [key] [value] \n"
                 << "del [key] \n"
                 << "get [key] \n"
                 << "restore [log_file] [dump_file(you can type \"null\")] \n"
                 << "dump [dump_file] \n"
                 << "parse [dump_file] \n"
                 << "If you just want to restore the status at the moment you make dump, use dump, \n"
                 << "If you want to restore from an accident, use parse.\n"
                 << "**************\n";
             
        }     
        
        if (log_flush_interval == 1 || log_flush_interval == 0 || (++loop) % log_flush_interval == 0) {

            safesl.land_log();

        }
        
    }
   
    
    int pause_integer;
    cin >> pause_integer;
    return 0;
}
