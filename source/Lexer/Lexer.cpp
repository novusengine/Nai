#include <pch/Build.h>
#include <iostream>

#include "Lexer.h"
#include "Utils/StringUtils.h"

void Lexer::Init(char* inBuffer, long size)
{
    buffer = inBuffer;
    bufferSize = size;
    bufferPosition = 0;

    lineNum = 1;
    charNum = 0;

    tokens.reserve(1024);
    charToTypeMap =
    {
        { ':', Token_Type::TOKEN_TYPE_OPERATOR }, // Declaration
        { '=', Token_Type::TOKEN_TYPE_OPERATOR }, // Assignment
        { '*', Token_Type::TOKEN_TYPE_OPERATOR }, // Pointer/Dereference/Multiplicative
        { '&', Token_Type::TOKEN_TYPE_OPERATOR }, // Get Memory Address
        { '.', Token_Type::TOKEN_TYPE_OPERATOR }, // Access
        { ',', Token_Type::TOKEN_TYPE_SEPERATOR }, // Parameter Seperator
        //{ "::", Token_Type::TOKEN_TYPE_OPERATOR }, // Const Declaration
        //{ ":=", Token_Type::TOKEN_TYPE_OPERATOR }, // Declaration Assignment
        //{ "fn", Token_Type::TOKEN_TYPE_IDENTIFIER }, // Signals start of function
        { '(', Token_Type::TOKEN_TYPE_SEPERATOR }, // Start Paramlist / Casting / Math
        { ')', Token_Type::TOKEN_TYPE_SEPERATOR }, // End Paramlist / Casting / Math
        { '{', Token_Type::TOKEN_TYPE_SEPERATOR }, // Start Scope
        { '}', Token_Type::TOKEN_TYPE_SEPERATOR }, // End Scope
        { ';', Token_Type::TOKEN_TYPE_SEPERATOR } // End of statement
    };
}

void Lexer::Process()
{
    ProcessTokens();
}

bool Lexer::IsDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool Lexer::IsNumeric(std::string& str)
{
    size_t strSize = str.size();
    if (strSize == 0)
        return false;

    int dotCount = 0;
    int minusCount = 0;
    int fCount = 0;
    int xCount = 0;
    bool hexPresent = false;

    for (size_t i = 0; i < strSize; i++)
    {
        char tmp = str[i];
        switch (tmp)
        {
        case '.':
            if (++dotCount != 1)
                return false;
            break;
        case '-':
            if (++minusCount != 1)
                return false;
            break;
        case 'f':
            if ((++fCount != 1 && xCount == 0) && i == strSize - 1)
                return false;
            break;
        case 'x':
        case 'X':
            // Hex 
            if (++xCount != 1 && i != 1)
                return false;
            break;
        default:
            if ((tmp >= 'a' && tmp <= 'f') || (tmp >= 'A' && tmp <= 'F'))
                hexPresent = true;
            else if (tmp < '0' || tmp > '9')
                return false;
            break;
        }
    }

    // If we detect hexidecimal characters check if hex format has been detected
    if (hexPresent && xCount == 0)
        return false;

    return true;
}

Token_Type Lexer::ResolveTokenType(std::string& input)
{
    Token_Type type = Token_Type::TOKEN_TYPE_IDENTIFIER;

    if (input[0] == '"')
    {
        input.erase(std::remove(input.begin(), input.end(), '"'), input.end());
        type = Token_Type::TOKEN_TYPE_STRING;
    }
    else if (IsNumeric(input))
    {
        type = Token_Type::TOKEN_TYPE_NUMERIC;
    }
    return type;
}

void Lexer::ResolveMultilineComment(long& bufferPos)
{
    while (bufferPos < bufferSize)
    {
        if (buffer[bufferPos] == '/' && buffer[bufferPos + 1] == '*')
        {
            // Skip the "/*"
            bufferPos += 2;
            ResolveMultilineComment(bufferPos);
        }

        if (buffer[bufferPos] == '*' && buffer[bufferPos + 1] == '/')
        {
            break;
        }

        bufferPos += 1;
    }

    bufferPos += 2;
}

long Lexer::SkipWhitespaceOrNewline(long bufferPos /* = defaultBufferPosition */)
{
    if (bufferPos == -1)
        bufferPos = bufferPosition;

    while (bufferPos < bufferSize)
    {
        char tmp = buffer[bufferPos];
        if (tmp != ' ' && tmp != '\n')
            break;

       bufferPos += 1;
    }

    return bufferPos;
}

void Lexer::SkipComment(long& bufferPos)
{
    if (bufferPos == -1)
        bufferPos = bufferPosition;

    char tmp = buffer[bufferPos];
    if (tmp == '/')
    {
        char nextTmp = buffer[++bufferPos];

        // Single Line Comment
        if (nextTmp == '/')
        {
            bufferPos += 1;
            while (bufferPos < bufferSize)
            {
                if (buffer[bufferPos] == '\n')
                    break;

                bufferPos += 1;
            }
        }
        // Multi Line Comment
        else if (nextTmp == '*')
        {
            bufferPos += 1;
            ResolveMultilineComment(bufferPos);
        }
    }
}

long Lexer::FindNextWhitespaceOrNewline(long bufferPos /* = defaultBufferPosition */)
{
    if (bufferPos == -1)
        bufferPos = bufferPosition;

    while (bufferPos < bufferSize)
    {
        char tmp = buffer[bufferPos];
        if (tmp == ' ' || tmp == '\n')
            break;

        bufferPos += 1;
    }

    return bufferPos;
}

void Lexer::ExtractTokens(long bufferPos /* = defaultBufferPosition */)
{
    if (bufferPos == -1)
        bufferPos = bufferPosition;

    long endPos = FindNextWhitespaceOrNewline(bufferPos);
    long lastOperatorIndex = bufferPos;


    for (; bufferPos < endPos; bufferPos++)
    {
        char tmp = buffer[bufferPos];

        auto itr = charToTypeMap.find(tmp);
        if (itr != charToTypeMap.end())
        {
            if (tmp == '.')
            {
                char lastChar = buffer[bufferPos - 1];
                if (IsDigit(lastChar))
                    continue;
            }

            // Handle Previous Token
            if (lastOperatorIndex != bufferPos)
            {
                long tmpSize = bufferPos - lastOperatorIndex;
                std::string tokenStr(&buffer[lastOperatorIndex], tmpSize);

                Token token;
                token.lineNum = lineNum;
                token.charNum = charNum;
                token.type = ResolveTokenType(tokenStr);
                token.value = tokenStr;
                tokens.push_back(token);
            }

            Token token;
            token.lineNum = lineNum;
            token.charNum = charNum;
            token.value = itr->first;
            token.type = itr->second;
            tokens.push_back(token);

            lastOperatorIndex = bufferPos + 1;
        }
    }

    if (bufferPos != lastOperatorIndex)
    {
        long tmpSize = bufferPos - lastOperatorIndex;
        std::string tokenStr(&buffer[lastOperatorIndex], tmpSize);

        Token token;
        token.lineNum = lineNum;
        token.charNum = charNum;
        token.type = ResolveTokenType(tokenStr);
        token.value = tokenStr;
        tokens.push_back(token);
    }

    bufferPosition = bufferPos + 1;
}

void Lexer::ProcessTokens()
{
    for (bufferPosition = 0; bufferPosition < bufferSize;)
    {
        long bufferPos = SkipWhitespaceOrNewline(defaultBufferPosition);
        long lastBufferPos = 0;

        do
        {
            SkipComment(bufferPos);
            lastBufferPos = bufferPos;
            bufferPos = SkipWhitespaceOrNewline(bufferPos);
        } while (bufferPos != lastBufferPos);

        ExtractTokens(bufferPos);
    }
}

std::vector<Token> Lexer::UnitTest_CodeToTokens(const std::string input)
{
    std::vector<Token> tokens;

    auto splitStr = StringUtils::Split(input, ']');
    for (std::string string : splitStr)
    {
        string.erase(std::remove_if(string.begin(), string.end(), isspace), string.end());

        // We expect a "[--- (name)]" Format, if we don't immediately see a bracket we will ignore
        if (string[0] != '[')
            continue;

        std::stringstream ss;
        size_t nameIndex = 1;
        for (size_t i = 1; i < string.size(); i++)
        {
            char tmp = string[i];
            if (tmp == '(')
            {
                nameIndex = i;
                break;
            }

            ss << tmp;
        }

        // If no type was found, continue;
        if (ss.str().size() == 0)
            continue;

        std::string type = ss.str();
        ss.str("");
        ss.clear();

        for (size_t i = nameIndex; string.size(); i++)
        {
            char tmp = string[i];

            if (tmp == '(')
                continue;

            if (tmp == ')')
                break;

            ss << tmp;
        }

        // If no name was found, continue;
        if (ss.str().size() == 0)
            continue;

        Token token;
        token.lineNum = 0;
        token.charNum = 0;
        token.type = Token::StringToType(type);
        token.value = ss.str();

        tokens.push_back(token);
    }

    return tokens;
}

std::string Lexer::UnitTest_TokensToCode(const std::vector<Token> tokens)
{
    std::stringstream ss;

    for (size_t i = 0; i < tokens.size(); i++)
    {
        Token token = tokens[i];
        ss << "[" << Token::TypeToString(token.type) << " (" << token.value << ")] ";
    }

    return ss.str();
}
