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
    bool IsNumeric(std::string_view& str);
    bool IsKeyword(std::string_view& str);

    const std::vector<Token> GetTokens() { return tokens; }
    TokenType ResolveTokenType(std::string_view& input);

    void ResolveOperator(long& bufferPos, Token& token);

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
    std::unordered_map<char, TokenType> charToTypeMap; // TODO: Move this to something faster (Char -> Token_Type)
};