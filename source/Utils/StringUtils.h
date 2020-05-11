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
}