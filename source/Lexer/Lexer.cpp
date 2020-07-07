#include <pch/Build.h>
#include <iostream>
#include <charconv>

#include "Lexer.h"
#include "Utils/StringUtils.h"

void Lexer::Init()
{
    ZoneScoped;

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
    ZoneScoped;

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
bool Lexer::IsNumeric(Token& token)
{
    ZoneScoped;

    size_t strSize = token.valueSize;

    int dotCount = 0;
    int minusCount = 0;
    int fCount = 0;
    int xCount = 0;
    bool hexPresent = false;

    for (size_t i = 0; i < strSize; i++)
    {
        char tmp = token.value[i];
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
bool Lexer::HandleKeyword(Token& token)
{
    ZoneScoped;

    // This is one way to optimize the amount of checks we do per call when Value is stored as a Char* inside of Token
    switch (token.valueSize)
    {
        case 2:
        {
            if (strncmp(token.value, KEYWORD_FUNCTION, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_FUNCTION;
            }
            else if (strncmp(token.value, KEYWORD_IF, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_IF;
            }
            else
            {
                return false;
            }

            return true;
        }

        case 3:
        {
            if (strncmp(token.value, KEYWORD_FOR, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_FOR;
            }
            else
            {
                return false;
            }

            return true;
        }

        case 4:
        {
            if (strncmp(token.value, KEYWORD_ENUM, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_ENUM;
            }
            else if (strncmp(token.value, KEYWORD_TRUE, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_TRUE;
            }
            else if (strncmp(token.value, KEYWORD_ELSE, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_ELSE;
            }
            else
            {
                return false;
            }

            return true;
        }

        case 5:
        {
            if (strncmp(token.value, KEYWORD_FALSE, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_FALSE;
            }
            else if (strncmp(token.value, KEYWORD_BREAK, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_BREAK;
            }
            else if (strncmp(token.value, KEYWORD_WHILE, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_WHILE;
            }
            else
            {
                return false;
            }

            return true;
        }

        case 6:
        {
            if (strncmp(token.value, KEYWORD_RETURN, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_RETURN;
            }
            else if (strncmp(token.value, KEYWORD_STRUCT, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_STRUCT;
            }
            else if (strncmp(token.value, KEYWORD_ELSEIF, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_ELSEIF;
            }
            else
            {
                return false;
            }

            return true;
        }

        case 8:
        {
            if (strncmp(token.value, KEYWORD_CONTINUE, token.valueSize) == 0)
            {
                token.subType = TokenSubType::KEYWORD_CONTINUE;
            }
            else
            {
                return false;
            }

            return true;
        }
    }

    return false;
}

void Lexer::ResolveTokenTypes(LexerFile& file, Token& token)
{
    ZoneScoped;

    if (token.value[0] == STRING_SYMBOL)
    {
        token.value += 1;
        token.valueSize -= 1;

        token.type = TokenType::LITERAL;
        token.subType = TokenSubType::STRING;
    }
    else if (HandleKeyword(token))
    {
        token.type = TokenType::KEYWORD;
    }
    else if (IsNumeric(token))
    {
        token.type = TokenType::LITERAL;
        token.subType = TokenSubType::NUMERIC;
    }
    else
    {
        ZoneScopedN("Resolve Identifier")
        token.type = TokenType::IDENTIFIER;

        // Report Error, an identifier cannot start with a value thats not an alpha or underscore
        char tmp = token.value[0];
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
    ZoneScoped;

    long originalBufferPos = file.bufferPosition;
    long nextCharPosition = file.bufferPosition + 1;

    switch (token.type)
    {
        case TokenType::DECLARATION:
        {
            if (file.buffer[nextCharPosition] == DECLARATION)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::CONST_DECLARATION;
                file.bufferPosition += 1;
                nextCharPosition += 1;
            }

            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;

                if (token.subType == TokenSubType::CONST_DECLARATION)
                    token.subType = TokenSubType::CONST_DECLARATION_ASSIGN;
                else
                    token.subType = TokenSubType::DECLARATION_ASSIGN;

                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_ASSIGN:
        {
            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_EQUALS;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_NOT:
        {
            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_NOT_EQUALS;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_ADD:
        {
            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_ADD_ASSIGN;
                file.bufferPosition += 1;
            }
            else if (file.buffer[nextCharPosition] == OP_ADD)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_INCREMENT;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_SUBTRACT:
        {
            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_SUBTRACT_ASSIGN;
                file.bufferPosition += 1;
            }
            else if (file.buffer[nextCharPosition] == OP_GREATER)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_RETURN_TYPE;
                file.bufferPosition += 1;
            }
            else if (file.buffer[nextCharPosition] == OP_SUBTRACT)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_DECREMENT;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_MULTIPLY:
        {
            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_MULTIPLY_ASSIGN;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_DIVIDE:
        {
            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_DIVIDE_ASSIGN;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_GREATER:
        {
            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_GREATER_EQUALS;
                file.bufferPosition += 1;
            }
            else if (file.buffer[nextCharPosition] == OP_GREATER)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_BITWISE_SHIFT_RIGHT;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_LESS:
        {
            if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_LESS_EQUALS;
                file.bufferPosition += 1;
            }
            else if (file.buffer[nextCharPosition] == OP_LESS)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_BITWISE_SHIFT_LEFT;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_BITWISE_AND:
        {
            if (file.buffer[nextCharPosition] == OP_BITWISE_AND)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_AND;
                file.bufferPosition += 1;
            }
            else if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_BITWISE_AND_ASSIGN;
                file.bufferPosition += 1;
            }
            break;
        }
        case TokenType::OP_BITWISE_OR:
        {
            if (file.buffer[nextCharPosition] == OP_BITWISE_OR)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_OR;
                file.bufferPosition += 1;
            }
            else if (file.buffer[nextCharPosition] == OP_ASSIGN)
            {
                token.valueSize += 1;
                token.subType = TokenSubType::OP_BITWISE_OR_ASSIGN;
                file.bufferPosition += 1;
            }
            break;
        }

        default:
            return;
    }

    long newBufferPos = file.bufferPosition - originalBufferPos;
    if (newBufferPos != file.bufferPosition)
        file.colNum += newBufferPos;
}

void Lexer::SkipComment(LexerFile& file)
{
    ZoneScoped;

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
    ZoneScoped;

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
    ZoneScoped;

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
            // Here we handle checking for strings, as such that we don't accidentally detect a space or newline within a string as a token
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
    ZoneScoped;

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
    ZoneScoped;

    long endPos = FindNextWhitespaceOrNewline(file);
    long lastOperatorIndex = file.bufferPosition;

    {
        ZoneScopedN("Iterator")

        for (; file.bufferPosition < endPos; file.bufferPosition++)
        {
            ZoneScopedN("Iteration")

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
                    if (IsDigit(nextChar))
                        continue;
                }

                // Handle Previous Token
                if (lastOperatorIndex != file.bufferPosition)
                {
                    long tmpSize = file.bufferPosition - lastOperatorIndex;

                    Token token;
                    token.lineNum = file.lineNum;
                    token.colNum = file.colNum;
                    token.type = TokenType::NONE;
                    token.subType = TokenSubType::NONE;
                    token.value = &file.buffer[lastOperatorIndex];
                    token.valueSize = tmpSize;

                    ResolveTokenTypes(file, token);
                    file.tokens.push_back(token);

                    file.colNum += tmpSize;
                }

                Token token;
                token.lineNum = file.lineNum;
                token.colNum = file.colNum++;
                token.value = &file.buffer[file.bufferPosition];
                token.valueSize = 1;
                token.type = itr->second;
                token.subType = TokenSubType::NONE;

                // ResolveOperator will correct the "Type" of the Token if needed
                ResolveOperator(file, token);

                // This will resolve the "FUNCTION_CALL" type (We could potentially move this into ResolveOperator, but it feels wrong)
                if (token.type == TokenType::LPAREN)
                {
                    if (file.tokens.size() != 0)
                    {
                        Token& prevToken = file.GetToken(file.tokens.size() - 1);

                        if (prevToken.type == TokenType::IDENTIFIER && prevToken.subType != TokenSubType::FUNCTION_DECLARATION)
                            prevToken.subType = TokenSubType::FUNCTION_CALL;
                    }
                }

                file.tokens.push_back(token);
                lastOperatorIndex = file.bufferPosition + 1;
            }
        }
    }

    if (file.bufferPosition != lastOperatorIndex)
    {
        long tmpSize = file.bufferPosition - lastOperatorIndex;

        Token token;
        token.lineNum = file.lineNum;
        token.colNum = file.colNum;
        token.type = TokenType::NONE;
        token.subType = TokenSubType::NONE;
        token.value = &file.buffer[lastOperatorIndex];
        token.valueSize = tmpSize;

        ResolveTokenTypes(file, token);
        file.tokens.push_back(token);

        file.colNum += tmpSize;
    }
}
void Lexer::ProcessTokens(LexerFile& file)
{
    ZoneScoped;

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

std::vector<Token> Lexer::UnitTest_CodeToTokens(const std::string /*input*/)
{
    ZoneScoped;

    std::vector<Token> _tokens;
    /*
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
    */
    return _tokens;
}
std::string Lexer::UnitTest_TokensToCode(const std::vector<Token> _tokens)
{
    ZoneScoped;

    std::stringstream ss;

    for (size_t i = 0; i < _tokens.size(); i++)
    {
        Token token = _tokens[i];
        ss << "[" << Token::TypeToString(token.type) << " (" << token.value << ")] ";
    }

    return ss.str();
}