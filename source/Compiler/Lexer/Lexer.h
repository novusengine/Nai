#pragma once
#include <pch/Build.h>
#include <string>
#include <vector>
#include <cassert>
#include <atomic>
#include <robin_hood.h>

#include "../ModuleInfo.h"
#include "../Token.h"

class Lexer
{
public:
    static bool Process(ModuleInfo& moduleInfo);
    static bool GatherUnits(ModuleInfo& moduleInfo);
    static bool ProcessUnits(ModuleInfo& moduleInfo);

    static void SkipSingleComment(const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum);
    static void SkipMultiComment(const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum);

    static bool TryParseNumeric(std::vector<Token>& tokens, const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum);
    static bool TryParseString(std::vector<Token>& tokens, const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum);
    static bool TryParseSpecialToken(std::vector<Token>& tokens, const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum);
    static bool TryParseIdentifier(std::vector<Token>& tokens, const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum);

    template<typename... Args>
    static void ReportError(int errorCode, std::string str, Args... args)
    {
        std::stringstream ss;
        ss << "Lexer Error " << errorCode << ": " << str << std::endl;

        printf_s(ss.str().c_str(), args...);
    }

private:
    Lexer() { }
    static robin_hood::unordered_map<std::string_view, Token::Type> _keywordStringToType;
};