#include "transfer.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "server")
        return runServer(std::stoi(argv[2]));
    if (argc >= 4 && std::string(argv[1]) == "client")
        return runClient(std::stoi(argv[2]), argv[3]);
    std::cerr << "Usage:\n  sft server <port>\n  sft client <port> <file>\n";
    return 1;
}
