#include "transfer.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>

int runClient(int port, const std::string& message) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx || SSL_CTX_load_verify_locations(ctx, "tls_key/server.crt", nullptr) != 1) {
        std::cerr << "[CLIENT] Could not load the server certificate\n";
        return 1;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
        perror("[CLIENT] connect");
        return 1;
    }

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);
    SSL_set1_host(ssl, "localhost");
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    std::cout << "[CLIENT] Securely connected, certificate verified\n";

    SSL_write(ssl, message.data(), static_cast<int>(message.size()));
    char buf[64];
    int n = SSL_read(ssl, buf, sizeof buf - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::cout << "[CLIENT] Server replied: " << buf << "\n";
    }
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(fd);
    return 0;
}
