#include "client.h"
#include "checksum.h"
#include "io.h"
#include <openssl/err.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

Client::Client(int port) : port_(port), ctx_(TLS_client_method()) {
    if (ctx_ && SSL_CTX_load_verify_locations(ctx_.get(), "tls_key/server.crt", nullptr) == 1) {
        SSL_CTX_set_verify(ctx_.get(), SSL_VERIFY_PEER, nullptr);
        ready_ = true;
    }
}

int Client::sendFile(const std::string& filePath) {
    if (!ready_) {
        std::cerr << "[CLIENT] Could not load the server certificate\n";
        return 1;
    }
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in) {
        std::cerr << "[CLIENT] Cannot open file: " << filePath << "\n";
        return 1;
    }
    uint64_t size = static_cast<uint64_t>(in.tellg());
    in.seekg(0);
    std::string name = std::filesystem::path(filePath).filename().string();

    Checksum::Hash hash;
    if (!Checksum::sha256File(filePath, hash)) {
        std::cerr << "[CLIENT] Could not hash file\n";
        return 1;
    }
    std::cout << "[CLIENT] SHA-256: " << Checksum::toHex(hash) << "\n";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("[CLIENT] socket");
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
        perror("[CLIENT] connect");
        ::close(fd);
        return 1;
    }

    SslConnection conn(ctx_.get(), fd);  // owns fd from here on
    SSL* ssl = conn.ssl();
    if (!ssl) {
        std::cerr << "[CLIENT] Could not create TLS connection\n";
        return 1;
    }
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
    ok = ok && sslWriteAll(ssl, num, 8) && sslWriteAll(ssl, hash.data(), 32);

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
    int nextMark = 25;
    auto start = std::chrono::steady_clock::now();
    while (ok && in) {
        in.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        std::streamsize got = in.gcount();
        if (got <= 0) break;
        ok = sslWriteAll(ssl, chunk.data(), static_cast<size_t>(got));
        sent += static_cast<uint64_t>(got);
        if (stopAfter > 0 && sent >= stopAfter) {
            std::cout << "[CLIENT] Simulated interruption after " << sent << " bytes\n";
            return 2;
        }
        if (size > 0) {
            int pct = static_cast<int>((offset + sent) * 100 / size);
            while (pct >= nextMark && nextMark <= 100) {
                std::cout << "[CLIENT] Progress " << nextMark << "%\n";
                nextMark += 25;
            }
        }
    }
    if (!ok) {
        std::cerr << "[CLIENT] Send failed after " << sent << " bytes\n";
        return 1;
    }
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    double mb = static_cast<double>(sent) / (1024.0 * 1024.0);
    std::cout << "[CLIENT] Sent " << sent << " bytes in 64 KB chunks ("
              << std::fixed << std::setprecision(2) << (secs > 0 ? mb / secs : 0.0) << " MB/s)\n";

    char reply[8];
    int n = SSL_read(ssl, reply, sizeof reply - 1);
    bool good = false;
    if (n > 0) {
        reply[n] = '\0';
        std::cout << "[CLIENT] Server replied: " << reply << "\n";
        good = std::string(reply) == "OK";
    }
    return good ? 0 : 1;
}
