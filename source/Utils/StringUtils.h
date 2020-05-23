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