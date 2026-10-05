#include "client.h"
#include "server.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "server") {
        Server server(std::stoi(argv[2]));
        return server.run();
    }
    if (argc >= 4 && std::string(argv[1]) == "client") {
        Client client(std::stoi(argv[2]));
        return client.sendFile(argv[3]);
    }
    std::cerr << "Usage:\n  sft server <port>\n  sft client <port> <file>\n";
    return 1;
}
