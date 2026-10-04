#include "transfer.h"
#include "io.h"
#include "checksum.h"
#include <array>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

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

static void handleClient(SSL* ssl) {
    unsigned char num[8];
    if (!sslReadAll(ssl, num, 8)) return;
    uint64_t nameLen = getU64(num);
    if (nameLen == 0 || nameLen > 255) {
        std::cerr << "[SERVER] Bad file name length\n";
        return;
    }
    std::string name(nameLen, '\0');
    if (!sslReadAll(ssl, name.data(), nameLen)) return;
    if (!sslReadAll(ssl, num, 8)) return;
    uint64_t size = getU64(num);
    std::array<unsigned char, 32> expected;
    if (!sslReadAll(ssl, expected.data(), 32)) return;

    std::string safe = std::filesystem::path(name).filename().string();
    if (safe.empty() || safe == "." || safe == "..") {
        std::cerr << "[SERVER] Bad file name\n";
        return;
    }
    std::filesystem::create_directories("received");
    std::string outPath = "received/" + safe;
    std::string partPath = outPath + ".part";

    uint64_t offset = 0;
    std::error_code ec;
    if (std::filesystem::exists(partPath, ec)) {
        uint64_t have = std::filesystem::file_size(partPath, ec);
        if (!ec && have <= size) offset = have;
    }
    putU64(offset, num);
    if (!sslWriteAll(ssl, num, 8)) return;

    std::ofstream out(partPath, std::ios::binary | (offset > 0 ? std::ios::app : std::ios::trunc));
    if (!out) {
        std::cerr << "[SERVER] Cannot create " << partPath << "\n";
        return;
    }
    std::cout << "[SERVER] Receiving " << safe << " (" << size
              << " bytes, resuming at " << offset << ")\n";

    std::vector<char> chunk(64 * 1024);
    uint64_t remaining = size - offset;
    while (remaining > 0) {
        size_t want = static_cast<size_t>(std::min<uint64_t>(remaining, chunk.size()));
        if (!sslReadAll(ssl, chunk.data(), want)) {
            out.close();
            std::cerr << "[SERVER] Connection lost, kept partial file with "
                      << (size - remaining) << " bytes\n";
            return;
        }
        out.write(chunk.data(), static_cast<std::streamsize>(want));
        remaining -= want;
    }
    out.close();

    std::array<unsigned char, 32> actual{};
    bool match = sha256File(partPath, actual) && actual == expected;
    std::cout << "[SERVER] SHA-256 " << (match ? "matches" : "MISMATCH")
              << ": " << toHex(actual) << "\n";
    if (match) {
        std::filesystem::rename(partPath, outPath);
        std::cout << "[SERVER] Saved " << outPath << "\n";
    } else {
        std::filesystem::remove(partPath);
    }
    sslWriteAll(ssl, match ? "OK" : "NO", 2);
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
            handleClient(ssl);
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(cfd);
    }
}
