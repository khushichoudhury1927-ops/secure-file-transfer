#pragma once
#include "raii.h"

class Server {
public:
    explicit Server(int port);
    ~Server();
    int run();

private:
    bool loadCertificate();
    bool openSocket();

    int port_;
    int listenFd_ = -1;
    SslContext ctx_;
};
