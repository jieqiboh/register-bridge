#include <cstdlib>
#include <iostream>
#include <string>
#include "server.hpp"

int main(int argc, char* argv[]) {
    int port = 5555;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--port")
            port = std::atoi(argv[i + 1]);
    }

    Server server(port);
    server.run();
    return 0;
}
