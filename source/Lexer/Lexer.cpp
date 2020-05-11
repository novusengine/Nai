#include <pch/Build.h>
#include <iostream>

#include "Lexer.h"

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
    for (bufferPosition = 0; bufferPosition < bufferSize;)
    {
        ProcessTokens();

        /*auto seperator_itr = charToTypeMap.find(c);
        if (seperator_itr != seperators.end())
        {
            Token lToken;
            lToken.type = Token_Type::TOKEN_TYPE_SEPERATOR;
            lToken.value = c;
            lToken.line = lineCount;
            tokens.push_back(lToken);
        }
        else if (c == '"')
        {
            Token lToken;
            lToken.type = Token_Type::TOKEN_TYPE_LITERAL;
            lToken.line = lineCount;

            std::string result = "";

            i += 1;
            while (i < bufferSize)
            {
                if (buffer[i] == '"')
                {
                    break;
                }

                result += buffer[i];
                i += 1;
            }

            lToken.value = result;
            tokens.push_back(lToken);
        }
        else if (isalnum(c))
        {
            Token lToken;
            lToken.type = isalpha(c) ? Token_Type::TOKEN_TYPE_IDENTIFIER : Token_Type::TOKEN_TYPE_LITERAL;
            lToken.line = lineCount;

            std::string result = "";
            while (i < bufferSize)
            {
                result += buffer[i];

                if (lToken.type == Token_Type::TOKEN_TYPE_IDENTIFIER)
                {
                    char tmp = buffer[i + 1];
                    if (!isalnum(tmp) && tmp != '_')
                        break;
                }
                else
                {
                    if (!isdigit(buffer[i + 1]))
                        break;
                }

                i += 1;
            }

            lToken.value = result;
            tokens.push_back(lToken);
        }*/
    }
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
        auto itr = charToTypeMap.find(buffer[bufferPos]);
        if (itr != charToTypeMap.end())
        {
            // Handle Previous Token
            if (lastOperatorIndex != bufferPos)
            {
                long tmpSize = bufferPos - lastOperatorIndex;
                std::string tokenStr(&buffer[lastOperatorIndex], tmpSize);

                Token token;
                token.lineNum = lineNum;
                token.charNum = charNum;
                token.value = tokenStr;
                token.type = Token_Type::TOKEN_TYPE_IDENTIFIER;
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
        token.value = tokenStr;
        token.type = Token_Type::TOKEN_TYPE_IDENTIFIER;
        tokens.push_back(token);
    }

    bufferPosition = bufferPos + 1;
}

void Lexer::ProcessTokens()
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
