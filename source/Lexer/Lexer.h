#pragma once
#include <pch/Build.h>
#include <string>
#include <vector>
#include <unordered_map>

#include "Token.h"

class Lexer
{
public:
    void Init(char* inBuffer, long size);
    void Process();

    bool IsDigit(char c);
    bool IsNumeric(std::string& str);

    const std::vector<Token> GetTokens() { return tokens; }
    Token_Type ResolveTokenType(std::string& input);

    // This function expects to start at a minimum of 1 position after the first "/"
    void ResolveMultilineComment(long& bufferPos);
    long SkipWhitespaceOrNewline(long bufferPos = defaultBufferPosition);

    // Only checks for the current "bufferPos"
    void SkipComment(long& bufferPos);
    long FindNextWhitespaceOrNewline(long bufferPos = defaultBufferPosition);
    void ExtractTokens(long bufferPos = defaultBufferPosition);
    void ProcessTokens();

    // Unit Tests
    static std::vector<Token> UnitTest_CodeToTokens(const std::string input);
    static std::string UnitTest_TokensToCode(const std::vector<Token> tokens);
private:
    const static int defaultBufferPosition = -1;
    char* buffer;
    long bufferSize;
    long bufferPosition;

    int lineNum;
    int charNum;

    std::vector<Token> tokens;
    std::unordered_map<char, Token_Type> charToTypeMap; // TODO: Move this to something faster (Char -> Token_Type)
};