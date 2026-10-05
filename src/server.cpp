#include "server.h"
#include "session.h"
#include <openssl/err.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>

Server::Server(int port) : port_(port), ctx_(TLS_server_method()) {}

Server::~Server() {
    if (listenFd_ >= 0) ::close(listenFd_);
}

bool Server::loadCertificate() {
    if (!ctx_) return false;
    if (SSL_CTX_use_certificate_file(ctx_.get(), "tls_key/server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx_.get(), "tls_key/server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

bool Server::openSocket() {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        perror("[SERVER] socket");
        return false;
    }
    int yes = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0 ||
        listen(listenFd_, 5) < 0) {
        perror("[SERVER] bind/listen");
        return false;
    }
    return true;
}

int Server::run() {
    if (!loadCertificate()) {
        std::cerr << "[SERVER] Could not load certificate (run scripts/gen_cert.sh)\n";
        return 1;
    }
    if (!openSocket()) return 1;
    std::cout << "[SERVER] Listening on port " << port_ << " (TLS enabled)" << std::endl;

    int nextId = 0;
    while (true) {
        sockaddr_in peer{};
        socklen_t len = sizeof peer;
        int cfd = accept(listenFd_, reinterpret_cast<sockaddr*>(&peer), &len);
        if (cfd < 0) continue;
        char ip[INET_ADDRSTRLEN] = "?";
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof ip);
        std::string who = std::string(ip) + ":" + std::to_string(ntohs(peer.sin_port));
        ServerSession session(ctx_.get(), cfd, ++nextId, who);
        session.run();
    }
}
