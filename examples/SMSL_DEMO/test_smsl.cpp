
#include "../../include/smsl.hpp"

using namespace std;

namespace {

int cmp(const int& lhs, const int& rhs) {
    return lhs - rhs;
}

std::string tostr(const int& data) {
    char buffer[32];
    sprintf(buffer, "Integer: %d.", data);
    return buffer;
}

}

int main() {

    smsl::Smsl<int, int>* sklist = new smsl::Smsl<int, int>("./", cmp, tostr, true, 103);

    char para1[1024];
    char para2[1024];
    char para3[1024];

    while (cin >> para1) {
        if (strcmp(para1, "set") == 0) {
            std::cin >> para2 >> para3;
            int key = std::atoi(para2);
            int val = std::atoi(para3);
            int ret = sklist->set(key, val);
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
            int ret = sklist->get(key, val);
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
            int ret = sklist->del(key);
            if (ret == -1) {
                cout << "Unexisting. Key: " << key << ".\n";
            } else {
                cout << "Success. Key: " << key << ".\n";
            }
            continue;
        }
        if (strcmp(para1, "clean") == 0) {
            delete sklist;
            sklist = new smsl::Smsl<int, int>("./", cmp, tostr, false, 103);
            continue;
        }
        cout << "Unknown command: " << para1 << ", try again.\n";
    }


    delete sklist;
    return 0;
}
