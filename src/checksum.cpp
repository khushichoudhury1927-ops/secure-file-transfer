#include "checksum.h"
#include <openssl/evp.h>
#include <cstdio>
#include <fstream>
#include <vector>

bool sha256File(const std::string& path, std::array<unsigned char, 32>& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return false;
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    std::vector<char> buf(64 * 1024);
    while (in) {
        in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        std::streamsize n = in.gcount();
        if (n > 0) EVP_DigestUpdate(ctx, buf.data(), static_cast<size_t>(n));
    }
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, out.data(), &len);
    EVP_MD_CTX_free(ctx);
    return len == 32;
}

std::string toHex(const std::array<unsigned char, 32>& h) {
    std::string s;
    char b[3];
    for (unsigned char c : h) {
        std::snprintf(b, sizeof b, "%02x", c);
        s += b;
    }
    return s;
}
