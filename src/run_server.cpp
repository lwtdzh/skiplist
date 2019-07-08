// This is the main cpp of SKServer.
// 7/5/2019, LiWentan.

#include "../include/smslserver.h"

int main(int argc, char** argv) {
    
    sk_cs::SKServer svr;
    svr.start();

    char command[200];
    while (std::cin >> command) {
        if (strcmp(command, "close") == 0) {
            svr.stop();
            break;
        }
        if (strcmp(command, "clear") == 0) {
            svr.stop(true);
            break;
        }
        std::cout << "Type \"close\" to stop the server.\n";
        std::cout << "Type \"clear\" to stop the server and clear the data.\n";
    }

    return 0;
}
