#pragma once
#include <array>
#include <string>

class Checksum {
public:
    using Hash = std::array<unsigned char, 32>;
    static bool sha256File(const std::string& path, Hash& out);
    static std::string toHex(const Hash& h);
};
