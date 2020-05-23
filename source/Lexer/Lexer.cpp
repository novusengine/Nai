#include <pch/Build.h>
#include <iostream>
#include <charconv>

#include "Lexer.h"
#include "Utils/StringUtils.h"

void Lexer::Init()
{
    totalLineNum = 0;

    _operatorCharToTypeMap =
    {
        { DECLARATION, TokenType::DECLARATION }, // Declaration
        { OP_ASSIGN, TokenType::OP_ASSIGN }, // Assignment
        { OP_NOT, TokenType::OP_NOT }, // Checks if the expression is false or 0, if so the expression is true
        { OP_ADD, TokenType::OP_ADD }, // Addition
        { OP_SUBTRACT, TokenType::OP_SUBTRACT }, // Subtraction
        { OP_MULTIPLY, TokenType::OP_MULTIPLY }, // Pointer/Dereference/Multiplicative
        { OP_DIVIDE, TokenType::OP_DIVIDE }, // Divide
        { OP_BITWISE_AND, TokenType::OP_BITWISE_AND }, // Bitwise And / Get Address
        { OP_BITWISE_OR, TokenType::OP_BITWISE_OR }, // Bitwise Or
        { OP_LESS, TokenType::OP_LESS }, // Less Than
        { OP_GREATER, TokenType::OP_GREATER }, // Greater Than
        { ACCESS, TokenType::ACCESS }, // Access
        { PARAM_SEPERATOR, TokenType::PARAM_SEPERATOR }, // Parameter Seperator
        { LPAREN, TokenType::LPAREN }, // Start Paramlist / Casting / Math
        { RPAREN, TokenType::RPAREN }, // End Paramlist / Casting / Math
        { LBRACE, TokenType::LBRACE }, // Start Scope
        { RBRACE, TokenType::RBRACE }, // End Scope
        { END_OF_LINE, TokenType::END_OF_LINE } // End of statement
    };
}

void Lexer::Process(LexerFile& file)
{
    ProcessTokens(file);
    totalLineNum += file.lineNum;
}

bool Lexer::IsAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
bool Lexer::IsDigit(char c)
{
    return c >= '0' && c <= '9';
}
bool Lexer::IsNumeric(std::string_view& str)
{
    size_t strSize = str.size();

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
    // @TODO: Turn into hash table
    return str == KEYWORD_FUNCTION || str == KEYWORD_STRUCT ||str == KEYWORD_ENUM || str == KEYWORD_WHILE || str == KEYWORD_IF || str == KEYWORD_FOR || str == KEYWORD_TRUE || str == KEYWORD_FALSE || str == KEYWORD_BREAK || str == KEYWORD_CONTINUE || str == KEYWORD_RETURN;
}

void Lexer::ResolveTokenTypes(LexerFile& file, std::string_view& input, Token& token)
{
    if (input[0] == STRING_SYMBOL)
    {
        input.remove_suffix(1);
        input.remove_prefix(1);

        token.type = TokenType::LITERAL;
        token.subType = TokenSubType::STRING;
    }
    else if (IsKeyword(input))
    {
        token.type = TokenType::KEYWORD;
        ResolveKeyword(input, token.subType);
    }
    else if (IsNumeric(input))
    {
        token.type = TokenType::LITERAL;
        token.subType = TokenSubType::NUMERIC;
    }
    else
    {
        token.type = TokenType::IDENTIFIER;

        // Report Error, an identifier cannot start with a value thats not an alpha or underscore
        char tmp = input[0];
        if (!IsAlpha(tmp) && tmp != '_')
        {
            ReportError(3, "Found identifier with invalid starting character. The following characters are valid: a->z, A->Z and _ (Line: %d, Col: %d)\n", static_cast<int>(file.lineNum), static_cast<int>(file.colNum));
            assert(false);
        }

        if (file.tokens.size() != 0)
        {
            Token& prevToken = file.GetToken(file.tokens.size() - 1);
            if (prevToken.subType == TokenSubType::KEYWORD_FUNCTION)
            {
                token.subType = TokenSubType::FUNCTION_DECLARATION;
            }
            else if (prevToken.subType == TokenSubType::KEYWORD_STRUCT)
            {
                token.type = TokenType::STRUCT;
                token.subType = TokenSubType::NONE;
            }
            else if (prevToken.subType == TokenSubType::KEYWORD_ENUM)
            {
                token.type = TokenType::ENUM;
                token.subType = TokenSubType::NONE;
            }
            else if (prevToken.subType == TokenSubType::OP_RETURN_TYPE || (prevToken.type == TokenType::DECLARATION && prevToken.subType == TokenSubType::NONE) || prevToken.subType == TokenSubType::CONST_DECLARATION)
            {
                token.type = TokenType::DATATYPE;
                token.subType = TokenSubType::NONE;
            }
        }
    }
}
void Lexer::ResolveOperator(LexerFile& file, Token& token)
{
    long originalBufferPos = file.bufferPosition;
    long nextCharPosition = file.bufferPosition + 1;

    if (token.type == TokenType::DECLARATION)
    {
        if (file.buffer[nextCharPosition] == DECLARATION)
        {
            token.value += DECLARATION;
            token.subType = TokenSubType::CONST_DECLARATION;
            file.bufferPosition += 1;
            nextCharPosition += 1;
        }

        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;

            if (token.subType == TokenSubType::CONST_DECLARATION)
                token.subType = TokenSubType::CONST_DECLARATION_ASSIGN;
            else
                token.subType = TokenSubType::DECLARATION_ASSIGN;

            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_ASSIGN)
    {
        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_EQUALS;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_NOT)
    {
        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_NOT_EQUALS;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_ADD)
    {
        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_ADD_ASSIGN;
            file.bufferPosition += 1;
        }
        else if (file.buffer[nextCharPosition] == OP_ADD)
        {
            token.value += OP_ADD;
            token.subType = TokenSubType::OP_INCREMENT;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_SUBTRACT)
    {
        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_SUBTRACT_ASSIGN;
            file.bufferPosition += 1;
        }
        else if (file.buffer[nextCharPosition] == OP_GREATER)
        {
            token.value += OP_GREATER;
            token.subType = TokenSubType::OP_RETURN_TYPE;
            file.bufferPosition += 1;
        }
        else if (file.buffer[nextCharPosition] == OP_SUBTRACT)
        {
            token.value += OP_SUBTRACT;
            token.subType = TokenSubType::OP_DECREMENT;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_MULTIPLY)
    {
        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_MULTIPLY_ASSIGN;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_DIVIDE)
    {
        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_DIVIDE_ASSIGN;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_GREATER)
    {
        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_GREATER_EQUALS;
            file.bufferPosition += 1;
        }
        else if (file.buffer[nextCharPosition] == OP_GREATER)
        {
            token.value += OP_GREATER;
            token.subType = TokenSubType::OP_BITWISE_SHIFT_RIGHT;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_LESS)
    {
        if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_LESS_EQUALS;
            file.bufferPosition += 1;
        }
        else if (file.buffer[nextCharPosition] == OP_LESS)
        {
            token.value += OP_LESS;
            token.subType = TokenSubType::OP_BITWISE_SHIFT_LEFT;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_BITWISE_AND)
    {
        if (file.buffer[nextCharPosition] == OP_BITWISE_AND)
        {
            token.value += OP_BITWISE_AND;
            token.subType = TokenSubType::OP_AND;
            file.bufferPosition += 1;
        }
        else if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_BITWISE_AND_ASSIGN;
            file.bufferPosition += 1;
        }
    }
    else if (token.type == TokenType::OP_BITWISE_OR)
    {
        if (file.buffer[nextCharPosition] == OP_BITWISE_OR)
        {
            token.value += OP_BITWISE_OR;
            token.subType = TokenSubType::OP_OR;
            file.bufferPosition += 1;
        }
        else if (file.buffer[nextCharPosition] == OP_ASSIGN)
        {
            token.value += OP_ASSIGN;
            token.subType = TokenSubType::OP_BITWISE_OR_ASSIGN;
            file.bufferPosition += 1;
        }
    }

    long newBufferPos = file.bufferPosition - originalBufferPos;
    if (newBufferPos != file.bufferPosition)
        file.colNum += newBufferPos;
}
void Lexer::ResolveKeyword(std::string_view& str, TokenSubType& type)
{
    if (str == KEYWORD_FUNCTION)
    {
        type = TokenSubType::KEYWORD_FUNCTION;
    }
    else if (str == KEYWORD_STRUCT)
    {
        type = TokenSubType::KEYWORD_STRUCT;
    }
    else if (str == KEYWORD_ENUM)
    {
        type = TokenSubType::KEYWORD_ENUM;
    }
    else if (str == KEYWORD_WHILE)
    {
        type = TokenSubType::KEYWORD_WHILE;
    }
    else if (str == KEYWORD_IF)
    {
        type = TokenSubType::KEYWORD_IF;
    }
    else if (str == KEYWORD_FOR)
    {
        type = TokenSubType::KEYWORD_FOR;
    }
    else if (str == KEYWORD_TRUE)
    {
        type = TokenSubType::KEYWORD_TRUE;
    }
    else if (str == KEYWORD_FALSE)
    {
        type = TokenSubType::KEYWORD_FALSE;
    }
    else if (str == KEYWORD_BREAK)
    {
        type = TokenSubType::KEYWORD_BREAK;
    }
    else if (str == KEYWORD_CONTINUE)
    {
        type = TokenSubType::KEYWORD_CONTINUE;
    }
    else if (str == KEYWORD_RETURN)
    {
        type = TokenSubType::KEYWORD_RETURN;
    }
}

void Lexer::SkipComment(LexerFile& file)
{
    // @TODO: Check for errors and report if any

    char tmp = file.buffer[file.bufferPosition];
    if (tmp == COMMENT_SLASH)
    {
        char nextTmp = file.buffer[file.bufferPosition + 1];

        // Single Line Comment
        if (nextTmp == COMMENT_SLASH)
        {
            file.colNum += 1;
            file.bufferPosition += 2;

            while (file.bufferPosition < file.size)
            {
                if (file.buffer[file.bufferPosition++] == NEWLINE)
                {
                    file.lineNum += 1;
                    file.colNum = 1;
                    break;
                }

                file.colNum += 1;
            }
        }
        // Multi Line Comment
        else if (nextTmp == COMMENT_MULTI_SYMBOL)
        {
            file.colNum += 1;
            file.bufferPosition += 2;
            ResolveMultilineComment(file);
        }
    }
}
void Lexer::ResolveMultilineComment(LexerFile& file)
{
    long lineNum = file.lineNum;
    long colNum = file.colNum - 2;

    while (file.bufferPosition < file.size)
    {
        char currChar = file.GetChar(file.bufferPosition);
        char nextChar = file.GetChar(file.bufferPosition + 1);

        if (currChar == NEWLINE)
        {
            file.lineNum += 1;
            file.colNum = 1;
            file.bufferPosition += 1;
        }
        else if (currChar == COMMENT_SLASH && nextChar == COMMENT_MULTI_SYMBOL)
        {
            // Skip the "/*"
            file.colNum += 2;
            file.bufferPosition += 2;
            ResolveMultilineComment(file);
        }
        else if (currChar == COMMENT_MULTI_SYMBOL && nextChar == COMMENT_SLASH)
        {
            file.colNum += 2;
            file.bufferPosition += 2;
            break;
        }
        else
        {
            file.colNum += 1;
            file.bufferPosition += 1;
        }


        // If this triggers, we have failed to find the "end" of the multi line comment
        if (file.size == file.bufferPosition + 1)
            break;
    }

    // We failed to find the end of the multi line comment
    if (file.size == file.bufferPosition + 1)
    {
        ReportError(2, "Found Multi line comment without closing symbol (Line: %d, Col: %d)\n", static_cast<int>(lineNum), static_cast<int>(colNum));
        assert(false);
    }
}
long Lexer::FindNextWhitespaceOrNewline(LexerFile& file, long bufferPos /* = defaultBufferPosition */)
{
    if (bufferPos == defaultBufferPosition)
        bufferPos = file.bufferPosition;

    long startBufferPos = bufferPos;

    bool stringError = false;
    bool stringFound = false;
    long stringColPosition = file.colNum;

    while (bufferPos < file.size)
    {
        char tmp = file.buffer[bufferPos];

        if (stringFound)
        {
            if (tmp == END_OF_LINE)
            {
                stringError = true;
                break;
            }
        }
        else
        {
            if (tmp == SPACE || tmp == NEWLINE)
                break;
        }

        if (tmp == STRING_SYMBOL)
        {
            // Here we check handle checking for strings, as such that we don't accidentally detect a space or newline within a string as a valid symbol
            if (!stringFound)
            {
                stringFound = true;
                stringColPosition += bufferPos - startBufferPos;
            }
            else
            {
                stringFound = false;
                bufferPos += 1;
                break;
            }
        }

        bufferPos += 1;
    }

    // We found a string symbol at some point, but we did not find the closing symbol
    if (stringError || (stringFound && bufferPos == file.size))
    {
        ReportError(1, "Found string without closing symbol (Line: %d , Col: %d)\n", static_cast<int>(file.lineNum), static_cast<int>(stringColPosition));
        assert(false);
    }

    return bufferPos;
}
long Lexer::SkipWhitespaceOrNewline(LexerFile& file, long bufferPos /* = defaultBufferPosition */)
{
    if (bufferPos == defaultBufferPosition)
        bufferPos = file.bufferPosition;

    while (bufferPos < file.size)
    {
        char tmp = file.buffer[bufferPos];
        if (tmp == NEWLINE)
        {
            file.lineNum += 1;
            file.colNum = 1;
        }
        else
        {
            if (tmp != SPACE)
                break;

            file.colNum += 1;
        }

        bufferPos += 1;
    }

    return bufferPos;
}

void Lexer::ExtractTokens(LexerFile& file)
{
    long endPos = FindNextWhitespaceOrNewline(file);
    long lastOperatorIndex = file.bufferPosition;

    for (; file.bufferPosition < endPos; file.bufferPosition++)
    {
        char tmp = file.buffer[file.bufferPosition];

        auto itr = _operatorCharToTypeMap.find(tmp);
        if (itr != _operatorCharToTypeMap.end())
        {
            if (tmp == ACCESS)
            {
                char lastChar = file.buffer[file.bufferPosition - 1];
                if (IsDigit(lastChar))
                    continue;
            }
            else if (tmp == OP_SUBTRACT)
            {
                char nextChar = file.buffer[file.bufferPosition + 1];
                if (isdigit(nextChar))
                    continue;
            }

            // Handle Previous Token
            if (lastOperatorIndex != file.bufferPosition)
            {
                long tmpSize = file.bufferPosition - lastOperatorIndex;
                std::string_view tokenStr(&file.buffer[lastOperatorIndex], tmpSize);

                Token token;
                token.lineNum = file.lineNum;
                token.colNum = file.colNum;
                token.type = TokenType::NONE;
                token.subType = TokenSubType::NONE;
                token.value = tokenStr;

                ResolveTokenTypes(file, tokenStr, token);
                file.tokens.push_back(token);

                file.colNum += tmpSize;
            }

            Token token;
            token.lineNum = file.lineNum;
            token.colNum = file.colNum++;
            token.value = itr->first;
            token.type = itr->second;
            token.subType = TokenSubType::NONE;

            // ResolveOperator will correct the "Type" of the Token if needed
            ResolveOperator(file, token);

            // This will resolve the "FUNCTION_CALL" type
            if (token.type == TokenType::LPAREN)
            {
                if (file.tokens.size() != 0)
                {
                    Token& prevToken = file.GetToken(file.tokens.size() - 1);

                    if (prevToken.type == TokenType::IDENTIFIER)
                        prevToken.subType = TokenSubType::FUNCTION_CALL;
                }
            }

            file.tokens.push_back(token);
            lastOperatorIndex = file.bufferPosition + 1;
        }
    }

    if (file.bufferPosition != lastOperatorIndex)
    {
        long tmpSize = file.bufferPosition - lastOperatorIndex;
        std::string_view tokenStr(&file.buffer[lastOperatorIndex], tmpSize);

        Token token;
        token.lineNum = file.lineNum;
        token.colNum = file.colNum;
        token.type = TokenType::NONE;
        token.subType = TokenSubType::NONE;
        token.value = tokenStr;

        ResolveTokenTypes(file, tokenStr, token);
        file.tokens.push_back(token);

        file.colNum += tmpSize;
    }
}
void Lexer::ProcessTokens(LexerFile& file)
{
    long bufferPos = defaultBufferPosition;
    long lastBufferPos = 0;

    for (file.bufferPosition = 0; file.bufferPosition < file.size;)
    {
        do
        {
            file.bufferPosition = SkipWhitespaceOrNewline(file, bufferPos);
            lastBufferPos = file.bufferPosition;
            SkipComment(file);
        } while (file.bufferPosition != lastBufferPos);

        ExtractTokens(file);
    }
}

std::vector<Token> Lexer::UnitTest_CodeToTokens(const std::string input)
{
    std::vector<Token> _tokens;

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
        token.colNum = 0;
        token.type = Token::StringToType(type);
        token.value = ss.str();

        _tokens.push_back(token);
    }

    return _tokens;
}
std::string Lexer::UnitTest_TokensToCode(const std::vector<Token> _tokens)
{
    std::stringstream ss;

    for (size_t i = 0; i < _tokens.size(); i++)
    {
        Token token = _tokens[i];
        ss << "[" << Token::TypeToString(token.type) << " (" << token.value << ")] ";
    }

    return ss.str();
}