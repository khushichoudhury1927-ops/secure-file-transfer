#include "transfer.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>

static SSL_CTX* makeContext() {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) return nullptr;
    if (SSL_CTX_use_certificate_file(ctx, "tls_key/server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "tls_key/server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }
    return ctx;
}

int runServer(int port) {
    SSL_CTX* ctx = makeContext();
    if (!ctx) {
        std::cerr << "[SERVER] Could not load certificate (run scripts/gen_cert.sh)\n";
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0 || listen(fd, 5) < 0) {
        perror("[SERVER] bind/listen");
        return 1;
    }
    std::cout << "[SERVER] Listening on port " << port << " (TLS enabled)\n";

    while (true) {
        int cfd = accept(fd, nullptr, nullptr);
        if (cfd < 0) continue;
        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, cfd);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            std::cout << "[SERVER] TLS handshake complete\n";
            char buf[1024];
            int n = SSL_read(ssl, buf, sizeof buf - 1);
            if (n > 0) {
                buf[n] = '\0';
                std::cout << "[SERVER] Received: " << buf << "\n";
                SSL_write(ssl, "OK", 2);
            }
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(cfd);
    }
}
