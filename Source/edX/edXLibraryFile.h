#pragma once

#include <filesystem>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>


namespace ProjectLibrarys
{
struct Library
{
    std::string library_name;
    std::string library_path;
    std::string library_version;
    std::string library_author;
    std::string libraryGit;
    double librarySize;
};

struct LibraryObject
{
    std::string id;
    std::string uniqueId; // Added uniqueId field for uniqueId in hexadecimal format
    std::string assetType;

    std::string properties; // Other properties as a string for simplicity
};

std::string generateRandomHexValue()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    for (int i = 0; i < 8; ++i) // Generate 8 hexadecimal digits
    {
        ss << std::hex << dis(gen);
    }

    return ss.str();
}
} // namespace ProjectLibrarys
