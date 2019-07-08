#include "../include/smslclient.h"

using namespace std;

string int2str(int num) {
    char buffer[32];
    sprintf(buffer, "%d", num);
    return buffer;
}

int main() {
    
    sk_cs::SKClient sklist("127.0.0.1", 8089);
    
    char para1[1024];
    char para2[1024];
    char para3[1024];
    
    while (cin >> para1) {
        if (strcmp(para1, "set") == 0) {
            std::cin >> para2 >> para3;
            string key = (para2);
            string val = (para3);
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
            string key = (para2);
            string val = int2str(-65536);
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
            string key = (para2);
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
    
    
    return 0;
}