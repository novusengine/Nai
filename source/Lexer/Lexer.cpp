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
        { ':', TokenType::TOKENTYPE_DECLARATION }, // Declaration
        { '=', TokenType::TOKENTYPE_OP_ASSIGN }, // Assignment
        { '*', TokenType::TOKENTYPE_OP_MULTIPLY }, // Pointer/Dereference/Multiplicative
        { '&', TokenType::TOKENTYPE_BITWISE_AND }, // Bitwise And / Get Address
        { '|', TokenType::TOKENTYPE_BITWISE_OR }, // Bitwise Or
        { '.', TokenType::TOKENTYPE_OP_ACCESS }, // Access
        { ',', TokenType::TOKENTYPE_COMMA }, // Parameter Seperator
        //{ "::", Token_Type::TOKEN_TYPE_OPERATOR }, // Const Declaration
        //{ ":=", Token_Type::TOKEN_TYPE_OPERATOR }, // Declaration Assignment
        { '(', TokenType::TOKENTYPE_LPAREN }, // Start Paramlist / Casting / Math
        { ')', TokenType::TOKENTYPE_RPAREN }, // End Paramlist / Casting / Math
        { '{', TokenType::TOKENTYPE_LBRACE }, // Start Scope
        { '}', TokenType::TOKENTYPE_RBRACE }, // End Scope
        { ';', TokenType::TOKENTYPE_SEMICOLON } // End of statement
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

bool Lexer::IsNumeric(std::string_view& str)
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
            if (++dotCount != 1 && i != 0 && i != strSize - 1)
                return false;
            break;
        case '-':
            if (++minusCount != 1 && i == 0)
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

bool Lexer::IsKeyword(std::string_view& str)
{
    return str == "fn" || str == "while" || str == "if" || str == "for";
}

TokenType Lexer::ResolveTokenType(std::string_view& input)
{
    TokenType type = TokenType::TOKENTYPE_IDENTIFIER;

    if (input[0] == '"')
    {
        input.remove_prefix(1);
        input.remove_suffix(1);
        type = TokenType::TOKENTYPE_STRING;
    }
    else if (IsKeyword(input))
    {
        type = TokenType::TOKENTYPE_KEYWORD;
    }
    else if (IsNumeric(input))
    {
        type = TokenType::TOKENTYPE_NUMERIC;
    }
    else if (input.size() == 1)
    {
        char x = input[0];
        if ((x > 36 && x < 39) || (x > 39 && x < 48) || 
            (x > 57 && x < 64) || (x == 123 || x == 125))
        {
            type = static_cast<TokenType>(input[0]);
        }
    }
    return type;
}

void Lexer::ResolveOperator(long& bufferPos, Token& token)
{
    if (token.type == TokenType::TOKENTYPE_DECLARATION)
    {
        if (buffer[bufferPos + 1] == ':')
        {
            token.value += ':';
            token.type = TokenType::TOKENTYPE_CONST_DECLARATION;
            bufferPos += 1;
        }

        if (buffer[bufferPos + 1] == '=')
        {
            token.value += '=';

            if (token.type == TokenType::TOKENTYPE_DECLARATION)
                token.type = TokenType::TOKENTYPE_DECLARATION_ASSIGN;
            else
                token.type = TokenType::TOKENTYPE_CONST_DECLARATION_ASSIGN;

            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_OP_ASSIGN)
    {
        if (buffer[bufferPos + 1] == '=')
        {
            token.value += '=';
            token.type = TokenType::TOKENTYPE_OP_EQUALS;
            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_OP_ADD)
    {
        if (buffer[bufferPos + 1] == '=')
        {
            token.value += '=';
            token.type = TokenType::TOKENTYPE_OP_ADD_ASSIGN;
            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_OP_SUBTRACT)
    {
        if (buffer[bufferPos + 1] == '=')
        {
            token.value += '=';
            token.type = TokenType::TOKENTYPE_OP_SUBTRACT_ASSIGN;
            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_OP_MULTIPLY)
    {
        if (buffer[bufferPos + 1] == '=')
        {
            token.value += '=';
            token.type = TokenType::TOKENTYPE_OP_MULTIPLY_ASSIGN;
            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_OP_DIVIDE)
    {
        if (buffer[bufferPos + 1] == '=')
        {
            token.value += '=';
            token.type = TokenType::TOKENTYPE_OP_DIVIDE_ASSIGN;
            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_OP_GREATER)
    {
        if (buffer[bufferPos + 1] == '=')
        {
            token.value += '=';
            token.type = TokenType::TOKENTYPE_OP_GREATER_EQUALS;
            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_OP_LESS)
    {
        if (buffer[bufferPos + 1] == '=')
        {
            token.value += '=';
            token.type = TokenType::TOKENTYPE_OP_LESS_EQUALS;
            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_BITWISE_AND)
    {
        if (buffer[bufferPos + 1] == '&')
        {
            token.value += '&';
            token.type = TokenType::TOKENTYPE_OP_AND;
            bufferPos += 1;
        }
    }
    else if (token.type == TokenType::TOKENTYPE_BITWISE_OR)
    {
        if (buffer[bufferPos + 1] == '|')
        {
            token.value += '|';
            token.type = TokenType::TOKENTYPE_OP_OR;
            bufferPos += 1;
        }
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
                std::string_view tokenStr(&buffer[lastOperatorIndex], tmpSize);

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

            // ResolveOperator will correct the "Type" of the Token if needed
            ResolveOperator(bufferPos, token);
            tokens.push_back(token);

            if (token.type == TokenType::TOKENTYPE_LPAREN)
                tokens[tokens.size() - 2].type = TokenType::TOKENTYPE_KEYWORD;

            lastOperatorIndex = bufferPos + 1;
        }
    }

    if (bufferPos != lastOperatorIndex)
    {
        long tmpSize = bufferPos - lastOperatorIndex;
        std::string_view tokenStr(&buffer[lastOperatorIndex], tmpSize);

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
