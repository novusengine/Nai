#pragma once
#include <pch/Build.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>

#include "Token.h"

struct LexerFile
{
    LexerFile(char* inBuffer, long inSize) : buffer(inBuffer), size(inSize), tokens() { tokens.reserve(1024); }

    char* buffer = nullptr;
    long size = 0;
    long bufferPosition = 0;

    int lineNum = 1;
    int colNum = 1;

    char GetChar(long index) { assert(index < size); return buffer[index]; }

    Token& GetToken(size_t index) { assert(index >= 0 && index < tokens.size()); return tokens[index]; }
    const std::vector<Token> GetTokens() { return tokens; }
    std::vector<Token> tokens;
};

class Lexer
{
public:
    void Init();
    void Process(LexerFile& file);

    bool IsAlpha(char c);
    bool IsDigit(char c);
    bool IsNumeric(std::string_view& str);
    bool IsKeyword(std::string_view& str);

    TokenType ResolveTokenType(LexerFile& file, std::string_view& input);
    void ResolveOperator(LexerFile& file, Token& token);
    void ResolveKeyword(std::string_view& str, TokenType& type);

    void SkipComment(LexerFile& file);
    void ResolveMultilineComment(LexerFile& file);
    long SkipWhitespaceOrNewline(LexerFile& file, long bufferPos = defaultBufferPosition);
    long FindNextWhitespaceOrNewline(LexerFile& file, long bufferPos = defaultBufferPosition);

    void ExtractTokens(LexerFile& file);
    void ProcessTokens(LexerFile& file);

    template<typename... Args>
    void ReportError(int errorCode, std::string str, Args... args)
    {
        std::stringstream ss;
        ss << "Lexer Error 1" << errorCode << ": " << str;

        printf_s(ss.str().c_str(), args...);
    }

    // Unit Tests
    static std::vector<Token> UnitTest_CodeToTokens(const std::string input);
    static std::string UnitTest_TokensToCode(const std::vector<Token> _tokens);

private:
    const static int defaultBufferPosition = -1;
    int totalLineNum;

    std::unordered_map<char, TokenType> _operatorCharToTypeMap; // TODO: Move this to something faster (Char -> Token_Type)

    const char* KEYWORD_FUNCTION = "fn";
    const char* KEYWORD_STRUCT = "struct";
    const char* KEYWORD_ENUM = "enum";
    const char* KEYWORD_WHILE = "while";
    const char* KEYWORD_IF = "if";
    const char* KEYWORD_FOR = "for";
    const char* KEYWORD_TRUE = "true";
    const char* KEYWORD_FALSE = "false";
    const char* KEYWORD_BREAK = "break";
    const char* KEYWORD_CONTINUE = "continue";
    const char* KEYWORD_RETURN = "return";

    const char COMMENT_SLASH = '/';
    const char COMMENT_MULTI_SYMBOL = '*';
    const char STRING_SYMBOL = '"';
    const char NEWLINE = '\n';
    const char SPACE = ' ';

    const char OP_ASSIGN = '=';
    const char OP_NOT = '!';
    const char OP_ADD = '+';
    const char OP_SUBTRACT = '-';
    const char OP_MULTIPLY = '*';
    const char OP_DIVIDE = '/';
    const char OP_LESS = '<';
    const char OP_GREATER = '>';
    const char OP_BITWISE_AND = '&';
    const char OP_BITWISE_OR = '|';
    const char OP_ACCESS = '.';
    const char OP_DECLARATION = ':';

    const char PARAM_SEPERATOR = ',';
    const char LPAREN = '(';
    const char RPAREN = ')';
    const char LBRACE = '{';
    const char RBRACE = '}';
    const char END_OF_LINE = ';';
};