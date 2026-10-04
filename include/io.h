#pragma once
#include <openssl/ssl.h>
#include <cstddef>
#include <cstdint>

inline bool sslWriteAll(SSL* ssl, const void* data, size_t len) {
    const char* p = static_cast<const char*>(data);
    while (len > 0) {
        int n = SSL_write(ssl, p, static_cast<int>(len));
        if (n <= 0) return false;
        p += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

inline bool sslReadAll(SSL* ssl, void* data, size_t len) {
    char* p = static_cast<char*>(data);
    while (len > 0) {
        int n = SSL_read(ssl, p, static_cast<int>(len));
        if (n <= 0) return false;
        p += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

inline void putU64(uint64_t v, unsigned char out[8]) {
    for (int i = 7; i >= 0; --i) { out[i] = v & 0xFF; v >>= 8; }
}

inline uint64_t getU64(const unsigned char in[8]) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | in[i];
    return v;
}
