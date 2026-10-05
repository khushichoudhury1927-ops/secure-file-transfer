#include "session.h"
#include "io.h"
#include <openssl/err.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

ServerSession::ServerSession(SSL_CTX* ctx, int fd, int id, std::string peer)
    : conn_(ctx, fd), id_(id), peer_(std::move(peer)) {}

void ServerSession::log(const std::string& msg) const {
    std::cout << "[SESSION " << id_ << "] " << msg << std::endl;
}

void ServerSession::run() {
    log("Client " + peer_ + " connected");
    if (!conn_.ssl() || SSL_accept(conn_.ssl()) <= 0) {
        ERR_print_errors_fp(stderr);
        return;
    }
    log("TLS handshake complete");
    if (!receiveHeader()) return;
    if (!receiveData()) return;
    verifyAndFinish();
}

bool ServerSession::receiveHeader() {
    SSL* ssl = conn_.ssl();
    unsigned char num[8];
    if (!sslReadAll(ssl, num, 8)) return false;
    uint64_t nameLen = getU64(num);
    if (nameLen == 0 || nameLen > 255) {
        log("Bad file name length");
        return false;
    }
    std::string name(nameLen, '\0');
    if (!sslReadAll(ssl, name.data(), nameLen)) return false;
    if (!sslReadAll(ssl, num, 8)) return false;
    size_ = getU64(num);
    if (!sslReadAll(ssl, expected_.data(), 32)) return false;

    std::string safe = std::filesystem::path(name).filename().string();
    if (safe.empty() || safe == "." || safe == "..") {
        log("Bad file name");
        return false;
    }
    std::filesystem::create_directories("received");
    name_ = safe;
    outPath_ = "received/" + safe;
    partPath_ = outPath_ + ".part";

    offset_ = 0;
    std::error_code ec;
    if (std::filesystem::exists(partPath_, ec)) {
        uint64_t have = std::filesystem::file_size(partPath_, ec);
        if (!ec && have <= size_) offset_ = have;
    }
    putU64(offset_, num);
    return sslWriteAll(ssl, num, 8);
}

bool ServerSession::receiveData() {
    std::ofstream out(partPath_, std::ios::binary | (offset_ > 0 ? std::ios::app : std::ios::trunc));
    if (!out) {
        log("Cannot create " + partPath_);
        return false;
    }
    log("Receiving " + name_ + " (" + std::to_string(size_) +
        " bytes, resuming at " + std::to_string(offset_) + ")");

    std::vector<char> chunk(64 * 1024);
    uint64_t remaining = size_ - offset_;
    uint64_t received = 0;
    int nextMark = 25;
    auto start = std::chrono::steady_clock::now();
    while (remaining > 0) {
        size_t want = static_cast<size_t>(std::min<uint64_t>(remaining, chunk.size()));
        if (!sslReadAll(conn_.ssl(), chunk.data(), want)) {
            out.close();
            log("Connection lost, kept partial file with " +
                std::to_string(offset_ + received) + " bytes");
            return false;
        }
        out.write(chunk.data(), static_cast<std::streamsize>(want));
        received += want;
        remaining -= want;
        if (size_ > 0) {
            int pct = static_cast<int>((offset_ + received) * 100 / size_);
            while (pct >= nextMark && nextMark <= 100) {
                log("Progress " + std::to_string(nextMark) + "%");
                nextMark += 25;
            }
        }
    }
    out.close();

    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    double mb = static_cast<double>(received) / (1024.0 * 1024.0);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << mb << " MB received in " << secs
       << " s (" << (secs > 0 ? mb / secs : 0.0) << " MB/s)";
    log(ss.str());
    return true;
}

void ServerSession::verifyAndFinish() {
    Checksum::Hash actual{};
    bool match = Checksum::sha256File(partPath_, actual) && actual == expected_;
    log(std::string("SHA-256 ") + (match ? "matches: " : "MISMATCH: ") + Checksum::toHex(actual));
    std::error_code ec;
    if (match) {
        std::filesystem::rename(partPath_, outPath_, ec);
        if (ec) {
            log("Could not rename: " + ec.message());
            match = false;
        } else {
            log("Saved " + outPath_);
        }
    } else {
        std::filesystem::remove(partPath_, ec);
    }
    sslWriteAll(conn_.ssl(), match ? "OK" : "NO", 2);
}
