#pragma once
#include <array>
#include <string>

bool sha256File(const std::string& path, std::array<unsigned char, 32>& out);
std::string toHex(const std::array<unsigned char, 32>& h);
