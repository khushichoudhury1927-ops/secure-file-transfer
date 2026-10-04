#include "transfer.h"
#include "io.h"
#include "checksum.h"
#include <array>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

int runClient(int port, const std::string& filePath) {
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in) {
        std::cerr << "[CLIENT] Cannot open file: " << filePath << "\n";
        return 1;
    }
    uint64_t size = static_cast<uint64_t>(in.tellg());
    in.seekg(0);
    std::string name = std::filesystem::path(filePath).filename().string();

    std::array<unsigned char, 32> hash;
    if (!sha256File(filePath, hash)) {
        std::cerr << "[CLIENT] Could not hash file\n";
        return 1;
    }
    std::cout << "[CLIENT] SHA-256: " << toHex(hash) << "\n";
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

    unsigned char num[8];
    putU64(name.size(), num);
    bool ok = sslWriteAll(ssl, num, 8) && sslWriteAll(ssl, name.data(), name.size());
    putU64(size, num);
    ok = ok && sslWriteAll(ssl, num, 8);
    ok = ok && sslWriteAll(ssl, hash.data(), 32);

    uint64_t offset = 0;
    ok = ok && sslReadAll(ssl, num, 8);
    if (ok) offset = getU64(num);
    if (!ok || offset > size) {
        std::cerr << "[CLIENT] Bad reply from server\n";
        return 1;
    }
    std::cout << "[CLIENT] Resuming from offset: " << offset << " bytes\n";
    in.seekg(static_cast<std::streamoff>(offset));

    const char* stopEnv = std::getenv("SFT_STOP_AFTER");
    uint64_t stopAfter = stopEnv ? std::stoull(stopEnv) : 0;

    std::vector<char> chunk(64 * 1024);
    uint64_t sent = 0;
    while (ok && in) {
        in.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        std::streamsize got = in.gcount();
        if (got <= 0) break;
        ok = sslWriteAll(ssl, chunk.data(), static_cast<size_t>(got));
        sent += static_cast<uint64_t>(got);
        if (stopAfter > 0 && sent >= stopAfter) {
            std::cout << "[CLIENT] Simulated interruption after " << sent << " bytes\n";
            close(fd);
            return 2;
        }
    }
    if (!ok) {
        std::cerr << "[CLIENT] Send failed after " << sent << " bytes\n";
        return 1;
    }
    std::cout << "[CLIENT] Sent " << sent << " bytes in 64 KB chunks\n";

    char reply[8];
    int n = SSL_read(ssl, reply, sizeof reply - 1);
    if (n > 0) {
        reply[n] = '\0';
        std::cout << "[CLIENT] Server replied: " << reply << "\n";
    }
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(fd);
    return 0;
}
