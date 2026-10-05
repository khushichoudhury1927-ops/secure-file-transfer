#pragma once
#include "raii.h"
#include <string>

class Client {
public:
    explicit Client(int port);
    int sendFile(const std::string& filePath);

private:
    int port_;
    SslContext ctx_;
    bool ready_ = false;
};
