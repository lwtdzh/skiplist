// You can test the skiplist under type: int.
// LiWentan at 06/26/2019.

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../include/skiplist.hpp"

using namespace std;

static int cmp(const int& lhs, const int& rhs) {
    return lhs - rhs;
}

static std::string tostr(const int& key) {
    char buffer[32];
    sprintf(buffer, "Integer: %d.", key);
    return buffer;
}

int main() {
    { // Test1: Set and get and delete.
        skiplist::SkipList<int, int> sklist(cmp, tostr, 103);
        
        int key[2000];
        int val[2000];
        for (int i = 0; i < 2000; ++i) {
            key[i] = i * 2 + 13;
            val[i] = key[i] + 1;
            sklist.set(key[i], val[i]);
        }
        
        int unmatch = 0;
        for (int i = 1999; i >=0; --i) {
            int res = 0;
            sklist.get(key[i], res);
            if (res == val[i]) {
                cout << "Matched.\n";
            } else {
                cout << "Unmatched. " << key[i] << " " << val[i] << " " << res << "\n";
                unmatch++;
            }
        }
        
        cout << "Fin get. Unmatched: " << unmatch << ".\n";
    }
    
    cout << "\n\n\n\n\n.";
    
    { // Manual.
        skiplist::SkipList<int, int> sklist(cmp, tostr, 103);
        
        char para1[1024];
        char para2[1024];
        char para3[1024];
        
        while (cin >> para1) {
            if (strcmp(para1, "set") == 0) {
                std::cin >> para2 >> para3;
                int key = std::atoi(para2);
                int val = std::atoi(para3);
                int ret = sklist.set(key, val);
                if (ret == 0) {
                    cout << "Success. Key: " << key << ", Value: " << val <<  ".\n";
                } else if (ret == 1){
                    cout << "Already existing. Key: " << key << ".\n";
                } else {
                    cout << "Failed.\n";
                }
                continue;
            }
            if (strcmp(para1, "get") == 0) {
                cin >> para2;
                int key = atoi(para2);
                int val = -65536;
                int ret = sklist.get(key, val);
                if (ret == -1) {
                    cout << "Unexisting. Key: " << key << ".\n";
                } else {
                    cout << "Found. Key: " << key << ", Value: " 
                        << val << ", Step: " << ret << ".\n";
                }
                continue;
            }
            if (strcmp(para1, "del") == 0) {
                cin >> para2;
                int key = atoi(para2);
                int ret = sklist.del(key);
                if (ret == -1) {
                    cout << "Unexisting. Key: " << key << ".\n";
                } else {
                    cout << "Success. Key: " << key << ".\n";
                }
                continue;
            }
            cout << "Unknown command: " << para1 << ", try again.\n";
        }
    }
    return 0;
}