#pragma once
#include "checksum.h"
#include "raii.h"
#include <cstdint>
#include <string>

class ServerSession {
public:
    ServerSession(SSL_CTX* ctx, int clientFd, int id, std::string peer);
    void run();

private:
    void log(const std::string& msg) const;
    bool receiveHeader();
    bool receiveData();
    void verifyAndFinish();

    SslConnection conn_;
    int id_;
    std::string peer_;
    std::string name_, outPath_, partPath_;
    uint64_t size_ = 0;
    uint64_t offset_ = 0;
    Checksum::Hash expected_{};
};
