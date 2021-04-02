#pragma once
#include <pch/Build.h>
#include <sstream>
#include <string>
#include <vector>

namespace StringUtils
{
    inline std::vector<std::string> Split(std::string string, char delim = ' ')
    {
        std::vector<std::string> results;
        std::stringstream ss(string);
        std::string token;

        while (std::getline(ss, token, delim))
        {
            results.push_back(token);
        }

        return results;
    }

    inline std::vector<std::wstring> Split(std::wstring string, wchar_t delim = L' ')
    {
        std::vector<std::wstring> results;
        std::wstringstream ss(string);
        std::wstring token;

        while (std::getline(ss, token, delim))
        {
            results.push_back(token);
        }

        return results;
    }

    inline uint64_t ToUInt64(const char* value, int size) 
    {
        // TODO: Modify this to return a bool to indicate if value is within UInt64 scope
        uint64_t result = 0;

        char const* p = value;
        char const* q = p + size;
        while (p < q) {
            result = (result << 1) + (result << 3) + *(p++) - '0';
        }
        return result;
    }

    constexpr uint32_t hash_djb2(const char* str, int size)
    {
        uint32_t hash = 5381;

        for (int i = 0; i < size; i++)
        {
            int c = str[i];
            hash = ((hash << 5) + hash) ^ c;
        }

        return hash;
    }

    // FNV-1a 32bit hashing algorithm.
    constexpr unsigned int fnv1a_32(char const* s, std::size_t count)
    {
        return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
    }
}

constexpr unsigned int operator"" _h(char const* s, std::size_t count)
{
    return StringUtils::fnv1a_32(s, count);
}
constexpr unsigned int operator"" _djb2(char const* s, std::size_t count)
{
    return static_cast<unsigned int>(StringUtils::hash_djb2(s, static_cast<int>(count)));
}