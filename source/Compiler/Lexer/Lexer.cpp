#include <pch/Build.h>
#include <iostream>
#include <execution>

#include "Lexer.h"
#include "Utils/StringUtils.h"

const char* KEYWORD_FUNCTION = "fn";
const char* KEYWORD_STRUCT = "struct";
const char* KEYWORD_ENUM = "enum";
const char* KEYWORD_WHILE = "while";
const char* KEYWORD_IF = "if";
const char* KEYWORD_ELSEIF = "else if";
const char* KEYWORD_ELSE = "else";
const char* KEYWORD_FOR = "for";
const char* KEYWORD_FOREACH = "foreach";
const char* KEYWORD_IN = "in";
const char* KEYWORD_TRUE = "true";
const char* KEYWORD_FALSE = "false";
const char* KEYWORD_BREAK = "break";
const char* KEYWORD_CONTINUE = "continue";
const char* KEYWORD_RETURN = "return";
const char* KEYWORD_SINGLE_COMMENT = "//";
const char* KEYWORD_MULTI_COMMENT_START = "/*";
const char* KEYWORD_MULTI_COMMENT_END = "*/";

const char NEWLINE = '\n';
const char SPACE = ' ';
const char STRING_SYMBOL = '"';
const char ATTRIBUTE_SYMBOL = '#';

robin_hood::unordered_map<std::string_view, Token::Type> Lexer::_keywordStringToType =
{
    { KEYWORD_FUNCTION, Token::Type::KEYWORD_FUNCTION },
    { KEYWORD_STRUCT, Token::Type::KEYWORD_STRUCT },
    { KEYWORD_ENUM, Token::Type::KEYWORD_ENUM },
    { KEYWORD_IF, Token::Type::KEYWORD_IF },
    { KEYWORD_ELSEIF, Token::Type::KEYWORD_ELSEIF },
    { KEYWORD_ELSE, Token::Type::KEYWORD_ELSE },
    { KEYWORD_WHILE, Token::Type::KEYWORD_WHILE },
    { KEYWORD_FOR, Token::Type::KEYWORD_FOR },
    { KEYWORD_FOREACH, Token::Type::KEYWORD_FOREACH },
    { KEYWORD_IN, Token::Type::KEYWORD_IN },
    { KEYWORD_TRUE, Token::Type::KEYWORD_TRUE },
    { KEYWORD_FALSE, Token::Type::KEYWORD_FALSE },
    { KEYWORD_BREAK, Token::Type::KEYWORD_BREAK },
    { KEYWORD_CONTINUE, Token::Type::KEYWORD_CONTINUE },
    { KEYWORD_RETURN, Token::Type::KEYWORD_RETURN }
};

bool Lexer::Process(ModuleInfo& moduleInfo)
{
    ZoneScoped;

    if (!GatherUnits(moduleInfo))
        return false;

    if (!ProcessUnits(moduleInfo))
        return false;

    return true;
}

bool Lexer::GatherUnits(ModuleInfo& moduleInfo)
{
    moduleInfo.compileUnits.reserve(32);

    const long singleCommentLen = strlen(KEYWORD_SINGLE_COMMENT);
    const long multiCommentLen = strlen(KEYWORD_MULTI_COMMENT_START);
    const long functionLen = strlen(KEYWORD_FUNCTION);
    const long structLen = strlen(KEYWORD_STRUCT);
    const long enumLen = strlen(KEYWORD_ENUM);

    int lineNum = 1;
    int colNum = 1;

    for (long bufferPosition = 0; bufferPosition < moduleInfo.fileBufferSize;)
    {
        const char& character = moduleInfo.fileBuffer[bufferPosition];
        long remainingBufferSize = moduleInfo.fileBufferSize - bufferPosition;

        // Skip New Lines or White Spaces
        if (character == NEWLINE || character == SPACE)
        {
            if (character == NEWLINE)
            {
                lineNum += 1;
                colNum = 1;
            }
            else
            {
                colNum += 1;
            }

            bufferPosition++;
            continue;
        }

        // Handle Comments Here
        if (remainingBufferSize >= singleCommentLen && strncmp(&character, KEYWORD_SINGLE_COMMENT, singleCommentLen) == 0)
        {
            bufferPosition += singleCommentLen;
            colNum += singleCommentLen;

            SkipSingleComment(moduleInfo.fileBuffer, moduleInfo.fileBufferSize, bufferPosition, lineNum, colNum);
            continue;
        }
        else if (remainingBufferSize >= multiCommentLen && strncmp(&character, KEYWORD_MULTI_COMMENT_START, multiCommentLen) == 0)
        {
            bufferPosition += multiCommentLen;
            colNum += singleCommentLen;

            SkipMultiComment(moduleInfo.fileBuffer, moduleInfo.fileBufferSize, bufferPosition, lineNum, colNum);
            continue;
        }

        // Handle fns, structs and enums here
        CompileUnit::Type type = CompileUnit::Type::NONE;

        CompileUnit& unit = moduleInfo.compileUnits.emplace_back();
        unit.type = type;
        unit.startPtr = &character;
        unit.startLineNum = lineNum;
        unit.startColumnNum = colNum;
        
        if (remainingBufferSize >= 1 && character == ATTRIBUTE_SYMBOL)
        {
            bufferPosition += 1;
            colNum += 1;

            char& openBracket = moduleInfo.fileBuffer[bufferPosition];
            if (openBracket != '[')
            {
                ReportError(5, "Attributes must be encapsulated in []. (Line: %d, Col: %d)", lineNum, colNum);
            }

            bufferPosition += 1;
            bool closeBracketFound = false;

            long subBufferPosition = bufferPosition;
            for (; subBufferPosition < moduleInfo.fileBufferSize; subBufferPosition++)
            {
                const char& subCharacter = moduleInfo.fileBuffer[subBufferPosition];
                long remainingSubBufferSize = moduleInfo.fileBufferSize - subBufferPosition;

                if (subCharacter == NEWLINE)
                {
                    lineNum += 1;
                    colNum = 1;

                    continue;
                }
                else if (subCharacter == SPACE)
                {
                    colNum += 1;
                    continue;
                }

                if (subCharacter == ']')
                {
                    closeBracketFound = true;

                    unit.endPtr = &subCharacter;
                    subBufferPosition++;
                    break;
                }
                else if (remainingSubBufferSize >= functionLen && strncmp(&subCharacter, KEYWORD_FUNCTION, functionLen) == 0)
                {
                    break;
                }
                else if (remainingSubBufferSize >= structLen && strncmp(&subCharacter, KEYWORD_STRUCT, structLen) == 0)
                {
                    break;
                }
                else if (remainingSubBufferSize >= enumLen && strncmp(&subCharacter, KEYWORD_ENUM, enumLen) == 0)
                {
                    break;
                }
            }

            if (!closeBracketFound)
            {
                ReportError(6, "Failed to find closing bracket. (Line: %d, Col: %d)", lineNum, colNum);
            }

            // Skip Whitespaces / New Lines
            {
                for (; subBufferPosition < moduleInfo.fileBufferSize; subBufferPosition++)
                {
                    const char& subCharacter = moduleInfo.fileBuffer[subBufferPosition];

                    if (subCharacter == NEWLINE)
                    {
                        lineNum += 1;
                        colNum = 1;

                        continue;
                    }
                    else if (subCharacter == SPACE)
                    {
                        colNum += 1;
                        continue;
                    }

                    break;
                }
            }

            bufferPosition = subBufferPosition;
        }
        
        const char& keywordCharacter = moduleInfo.fileBuffer[bufferPosition];
        if (remainingBufferSize >= functionLen && strncmp(&keywordCharacter, KEYWORD_FUNCTION, functionLen) == 0)
        {
            unit.type = CompileUnit::Type::FUNCTION;
            bufferPosition += functionLen;
            colNum += functionLen;
        }
        else if (remainingBufferSize >= structLen && strncmp(&keywordCharacter, KEYWORD_STRUCT, structLen) == 0)
        {
            unit.type = CompileUnit::Type::STRUCT;
            bufferPosition += structLen;
            colNum += structLen;
        }
        else if (remainingBufferSize >= enumLen && strncmp(&keywordCharacter, KEYWORD_ENUM, enumLen) == 0)
        {
            unit.type = CompileUnit::Type::ENUM;
            bufferPosition += enumLen;
            colNum += enumLen;
        }
        else
        {
            ReportError(1, "NAI is a pure language. This means you cannot have global data, you may define enums, structs or functions in your global scope. (Line: %d, Col: %d)", lineNum, colNum);
            return false;
        }

        // Gather Unit
        {
            long subBufferPosition = bufferPosition;
            
            long openBraceCount = 0;
            bool openBraceFound = false;

            int firstBraceLineNum = 0;
            int firstBraceColNum = 0;

            for (; subBufferPosition < moduleInfo.fileBufferSize; subBufferPosition++)
            {
                const char& subCharacter = moduleInfo.fileBuffer[subBufferPosition];
                long remainingSubBufferSize = moduleInfo.fileBufferSize - subBufferPosition;

                if (subCharacter == NEWLINE)
                {
                    lineNum += 1;
                    colNum = 1;

                    continue;
                }
                else if (subCharacter == SPACE)
                {
                    colNum += 1;
                    continue;
                }

                if (subCharacter == '{')
                {
                    openBraceCount += 1;

                    if (openBraceFound == false)
                    {
                        firstBraceLineNum = lineNum;
                        firstBraceColNum = colNum;

                        openBraceFound = true;
                    }
                }
                else if (subCharacter == '}')
                {
                    if (openBraceFound == false)
                    {
                        ReportError(2, "Found closing brace without a matching opening brace. (Line: %d, Col: %d)", lineNum, colNum);
                        return false;
                    }

                    if (--openBraceCount == 0)
                    {
                        unit.endPtr = &subCharacter;
                        subBufferPosition++;
                        break;
                    }
                }
                else
                {
                    if (unit.type == CompileUnit::Type::FUNCTION || unit.type == CompileUnit::Type::ENUM)
                    {
                        if (remainingSubBufferSize >= functionLen && strncmp(&subCharacter, KEYWORD_FUNCTION, functionLen) == 0)
                        {
                            break;
                        }
                        else if (remainingSubBufferSize >= structLen && strncmp(&subCharacter, KEYWORD_STRUCT, structLen) == 0)
                        {
                            break;
                        }
                        else if (remainingSubBufferSize >= enumLen && strncmp(&subCharacter, KEYWORD_ENUM, enumLen) == 0)
                        {
                            break;
                        }
                    }
                    else if (unit.type == CompileUnit::Type::STRUCT)
                    {
                        if (remainingSubBufferSize >= structLen && strncmp(&subCharacter, KEYWORD_STRUCT, structLen) == 0)
                        {
                            break;
                        }
                        else if (remainingSubBufferSize >= enumLen && strncmp(&subCharacter, KEYWORD_ENUM, enumLen) == 0)
                        {
                            break;
                        }
                    }
                }

                colNum += 1;
            }

            if (openBraceFound == false)
            {
                ReportError(3, "Failed to find opening and closing braces. (Line: %d, Col: %d)", lineNum, colNum);
                return false;
            }
            else if (openBraceCount != 0)
            {
                ReportError(4, "Found opening brace without a matching closing brace. (Line: %d, Col: %d)", firstBraceLineNum, firstBraceColNum);
                return false;
            }

            bufferPosition = subBufferPosition;
        }
    }

    return true;
}

bool Lexer::ProcessUnits(ModuleInfo& moduleInfo)
{
    std::for_each(std::execution::par, moduleInfo.compileUnits.begin(), moduleInfo.compileUnits.end(),
        [](CompileUnit& unit)
        {
            unit.tokens.reserve(16384);

            const long singleCommentLen = strlen(KEYWORD_SINGLE_COMMENT);
            const long multiCommentLen = strlen(KEYWORD_MULTI_COMMENT_START);

            int lineNum = unit.startLineNum;
            int colNum = unit.startColumnNum;

            long bufferStartPosition = 0;
            long bufferSize = (unit.endPtr - unit.startPtr) + 1;

            for (long bufferPosition = bufferStartPosition; bufferPosition < bufferSize;)
            {
                const char& character = unit.startPtr[bufferPosition];
                long remainingBytes = bufferSize - bufferPosition;

                // Skip New Lines or White Spaces
                if (character == NEWLINE || character == SPACE)
                {
                    if (character == NEWLINE)
                    {
                        lineNum += 1;
                        colNum = 1;
                    }
                    else
                    {
                        colNum += 1;
                    }

                    bufferPosition += 1;
                    continue;
                }

                // Handle Comments Here
                if (remainingBytes >= singleCommentLen && strncmp(&character, KEYWORD_SINGLE_COMMENT, singleCommentLen) == 0)
                {
                    bufferPosition += singleCommentLen;
                    colNum += singleCommentLen;

                    SkipSingleComment(unit.startPtr, bufferSize, bufferPosition, lineNum, colNum);
                    continue;
                }
                else if (remainingBytes >= multiCommentLen && strncmp(&character, KEYWORD_MULTI_COMMENT_START, multiCommentLen) == 0)
                {
                    bufferPosition += multiCommentLen;
                    colNum += singleCommentLen;

                    SkipMultiComment(unit.startPtr, bufferSize, bufferPosition, lineNum, colNum);
                    continue;
                }

                if (TryParseNumeric(unit.tokens, unit.startPtr, bufferSize, bufferPosition, lineNum, colNum))
                    continue;

                if (TryParseString(unit.tokens, unit.startPtr, bufferSize, bufferPosition, lineNum, colNum))
                    continue;

                if (TryParseSpecialToken(unit.tokens, unit.startPtr, bufferSize, bufferPosition, lineNum, colNum))
                    continue;

                if (TryParseIdentifier(unit.tokens, unit.startPtr, bufferSize, bufferPosition, lineNum, colNum))
                    continue;

                // We should never reach this
                ReportError(7, "Failed to Analyse Character Sequence");
                break;
            }

            int numTokens = static_cast<int>(unit.tokens.size());
            for (int i = 0; i < numTokens; i++)
            {
                const Token& token = unit.tokens[i];
                if (token.type == Token::Type::KEYWORD_FUNCTION)
                {
                    if (i + 1 < numTokens)
                    {
                        const Token& nameToken = unit.tokens[i + 1];
                        unit.name = nameToken.stringview;
                    }
                    break;
                }
            }
        });

    return true;
}

void Lexer::SkipSingleComment(const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum)
{
    long subBufferPosition = bufferPosition;
    for (; subBufferPosition < bufferSize;)
    {
        const char& subCharacter = buffer[subBufferPosition];

        if (subCharacter == NEWLINE)
        {
            lineNum += 1;
            colNum = 1;

            subBufferPosition += 1;
            break;
        }
        else
        {
            colNum += 1;
            subBufferPosition += 1;
        }
    }

    bufferPosition = subBufferPosition;
}
void Lexer::SkipMultiComment(const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum)
{
    const long multiCommentStartLen = strlen(KEYWORD_MULTI_COMMENT_START);
    const long multiCommentEndLen = strlen(KEYWORD_MULTI_COMMENT_START);

    long subBufferPosition = bufferPosition;
    for (; subBufferPosition < bufferSize;)
    {
        const char& subCharacter = buffer[subBufferPosition];
        long remainingSubBufferSize = bufferSize - subBufferPosition;

        // Skip New Lines
        if (subCharacter == NEWLINE)
        {
            lineNum += 1;
            colNum = 1;

            subBufferPosition += 1;
        }
        else if (remainingSubBufferSize >= multiCommentStartLen && strncmp(&subCharacter, KEYWORD_MULTI_COMMENT_START, multiCommentStartLen) == 0)
        {
            subBufferPosition += multiCommentStartLen;
            colNum += multiCommentStartLen;

            SkipMultiComment(buffer, bufferSize, subBufferPosition, lineNum, colNum);
        }
        else if (remainingSubBufferSize >= multiCommentEndLen && strncmp(&subCharacter, KEYWORD_MULTI_COMMENT_END, multiCommentEndLen) == 0)
        {
            subBufferPosition += multiCommentEndLen;
            colNum += multiCommentEndLen;
            break;
        }
        else
        {
            colNum += 1;
            subBufferPosition += 1;
        }
    }

    bufferPosition = subBufferPosition;
}

bool Lexer::TryParseNumeric(std::vector<Token>& tokens, const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum)
{
    const char& character = buffer[bufferPosition];

    if (!isdigit(character))
        return false;

    // Parse Number Here
    int fCount = 0;
    int uCount = 0;
    int xCount = 0;
    int dotCount = 0;

    long subBufferPosition = bufferPosition + 1;
    for (; subBufferPosition < bufferSize; subBufferPosition++)
    {
        const char& subCharacter = buffer[subBufferPosition];

        if (isdigit(subCharacter))
            continue;

        if (subCharacter == 'f')
        {
            fCount += 1;

            if (xCount == 0)
            {
                subBufferPosition++;
                break;
            }
        }
        else if (subCharacter == 'u')
        {
            uCount += 1;
            subBufferPosition++;
            break;
        }
        else if (subCharacter == 'x' || subCharacter == 'X')
        {
            xCount += 1;
            if (xCount > 1)
                break;

            break;
        }
        else if (subCharacter == '.')
        {
            dotCount += 1;

            if (dotCount > 1)
                break;
        }
        else if ((subCharacter >= 'a' && subCharacter <= 'f') || (subCharacter >= 'A' && subCharacter <= 'F'))
        {
            if (xCount == 0)
                break;
        }
        else
        {
            break;
        }
    }

    long length = subBufferPosition - bufferPosition;
    long stringViewLength = length;

    Token& token = tokens.emplace_back();

    if (xCount != 0)
    {
        token.type = Token::Type::NUMERIC_HEX;
    }
    else if (dotCount != 0)
    {
        if (fCount == 0)
        {
            token.type = Token::Type::NUMERIC_DOUBLE;
        }
        else
        {
            token.type = Token::Type::NUMERIC_FLOAT;
            stringViewLength -= 1;
        }
    }
    else
    {
        if (uCount == 0)
        {
            token.type = Token::Type::NUMERIC_SIGNED;
        }
        else
        {
            token.type = Token::Type::NUMERIC_UNSIGNED;
            stringViewLength -= 1;
        }
    }

    token.stringview = std::string_view(&character, stringViewLength);
    token.lineNum = lineNum;
    token.colNum = colNum;

    bufferPosition = subBufferPosition;
    colNum += length;
    return true;
}
bool Lexer::TryParseString(std::vector<Token>& tokens, const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum)
{
    const char& character = buffer[bufferPosition];

    if (character != STRING_SYMBOL)
        return false;

    // Parse String Here
    bool foundEndSymbol = false;

    long subBufferPosition = bufferPosition + 1;
    for (; subBufferPosition < bufferSize; subBufferPosition++)
    {
        const char& subCharacter = buffer[subBufferPosition];

        if (subCharacter == NEWLINE)
            break;

        if (subCharacter == STRING_SYMBOL)
        {
            foundEndSymbol = true;
            break;
        }
    }

    if (foundEndSymbol == false)
        return false;

    long length = subBufferPosition - bufferPosition;
    Token& token = tokens.emplace_back();

    token.type = Token::Type::STRING;
    token.stringview = std::string_view(&character + 1, length - 1);
    token.lineNum = lineNum;
    token.colNum = colNum;

    bufferPosition = subBufferPosition + 1;
    colNum += length;
    return true;
}
bool Lexer::TryParseSpecialToken(std::vector<Token>& tokens, const char* buffer, long /*bufferSize*/, long& bufferPosition, int& lineNum, int& colNum)
{
    Token::Type type = Token::Type::NONE;
    int specialCharacterLength = 1;
    const char* character = &buffer[bufferPosition];

    switch (*character++)
    {
        case ATTRIBUTE_SYMBOL:
        {
            type = Token::Type::ATTRIBUTE;
            break;
        }

        case '!':
        {
            type = Token::Type::EXCLAMATION_MARK;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::NOT_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case '%':
        {
            type = Token::Type::PERCENT;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::MODULUS_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case '&':
        {
            type = Token::Type::AMPERSAND;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::BITWISE_AND_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }

                case '&':
                {
                    type = Token::Type::AND;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case '(':
        {
            type = Token::Type::LPAREN;
            break;
        }

        case ')':
        {
            type = Token::Type::RPAREN;
            break;
        }

        case '*':
        {
            type = Token::Type::ASTERISK;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::MULTIPLY_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case '+':
        {
            type = Token::Type::PLUS;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::PLUS_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case ',':
        {
            type = Token::Type::COMMA;
            break;
        }

        case '-':
        {
            type = Token::Type::MINUS;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::MINUS_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }

                case '>':
                {
                    type = Token::Type::RETURN_TYPE;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case '.':
        {
            type = Token::Type::PERIOD;
            break;
        }

        case '/':
        {
            type = Token::Type::SLASH;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::DIVIDE_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case ':':
        {
            type = Token::Type::DECLARATION;

            switch (*character++)
            {
                case ':':
                {
                    type = Token::Type::DECLARATION_CONST;
                    specialCharacterLength += 1;

                    break;
                }
            }

            break;
        }

        case ';':
        {
            type = Token::Type::SEMICOLON;
            break;
        }

        case '<':
        {
            type = Token::Type::LESS_THAN;

            switch (*character++)
            {
                case '=':
                {
                    type = Token::Type::LESS_THAN_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }

                case '<':
                {
                    type = Token::Type::BITSHIFT_LEFT;
                    specialCharacterLength += 1;

                    switch (*character)
                    {
                        case '=':
                        {
                            type = Token::Type::BITSHIFT_LEFT_EQUALS;
                            specialCharacterLength += 1;
                            break;
                        }
                    }

                    break;
                }
            }

            break;
        }

        case '=':
        {
            type = Token::Type::ASSIGN;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::EQUALS;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case '>':
        {
            type = Token::Type::GREATER_THAN;

            switch (*character++)
            {
                case '=':
                {
                    type = Token::Type::GREATER_THAN_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }

                case '>':
                {
                    type = Token::Type::BITSHIFT_RIGHT;
                    specialCharacterLength += 1;

                    switch (*character)
                    {
                        case '=':
                        {
                            type = Token::Type::BITSHIFT_RIGHT_EQUALS;
                            specialCharacterLength += 1;
                            break;
                        }
                    }

                    break;
                }
            }

            break;
        }

        case '?':
        {
            type = Token::Type::QUESTION_MARK;
            break;
        }

        case '[':
        {
            type = Token::Type::LBRACKET;
            break;
        }

        case '\\':
        {
            type = Token::Type::BACKSLASH;
            break;
        }

        case ']':
        {
            type = Token::Type::RBRACKET;
            break;
        }

        case '^':
        {
            type = Token::Type::CARET;

            switch (*character)
            {
                case '=':
                {
                    type = Token::Type::POW_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case '{':
        {
            type = Token::Type::LBRACE;
            break;
        }

        case '|':
        {
            type = Token::Type::PIPE;

            switch (*character)
            {
                case '|':
                {
                    type = Token::Type::OR;
                    specialCharacterLength += 1;
                    break;
                }

                case '=':
                {
                    type = Token::Type::BITWISE_OR_EQUALS;
                    specialCharacterLength += 1;
                    break;
                }
            }

            break;
        }

        case '}':
        {
            type = Token::Type::RBRACE;
            break;
        }

        case '~':
        {
            type = Token::Type::TILDE;
            break;
        }

        default:
            return false;
    }

    Token& token = tokens.emplace_back();
    token.type = type;
    token.stringview = std::string_view(&buffer[bufferPosition], specialCharacterLength);
    token.lineNum = lineNum;
    token.colNum = colNum;

    bufferPosition += specialCharacterLength;
    colNum += specialCharacterLength;

    return true;
}
bool Lexer::TryParseIdentifier(std::vector<Token>& tokens, const char* buffer, long bufferSize, long& bufferPosition, int& lineNum, int& colNum)
{
    const char& character = buffer[bufferPosition];

    if (character != '_' && !isalpha(character))
        return false;

    long subBufferPosition = bufferPosition + 1;
    for (; subBufferPosition < bufferSize; subBufferPosition++)
    {
        const char& subCharacter = buffer[subBufferPosition];

        if (subCharacter == NEWLINE || subCharacter == SPACE)
            break;

        if (!isalpha(subCharacter) && !isdigit(subCharacter))
            break;
    }

    long length = subBufferPosition - bufferPosition;

    Token& token = tokens.emplace_back();
    token.type = Token::Type::IDENTIFIER;
    token.stringview = std::string_view(&character, length);
    token.lineNum = lineNum;
    token.colNum = colNum;

    // Keyword
    auto itr = _keywordStringToType.find(token.stringview);
    if (itr != _keywordStringToType.end())
    {
        token.type = itr->second;

        // Keyword Custom Rules
        if (tokens.size() > 1)
        {
            Token& prevToken = tokens[tokens.size() - 2];

            if (prevToken.type == Token::Type::KEYWORD_ELSE && token.type == Token::Type::KEYWORD_IF)
            {
                prevToken.type = Token::Type::KEYWORD_ELSEIF;

                size_t newLength = (token.stringview.data() + length) - prevToken.stringview.data();
                prevToken.stringview = std::string_view(prevToken.stringview.data(), newLength);

                tokens.pop_back();
            }
        }
    }
    else
    {
        // Identifier Custom Rules
        if (tokens.size() > 1)
        {
            Token& prevToken = tokens[tokens.size() - 2];

            if (prevToken.type == Token::Type::DECLARATION || prevToken.type == Token::Type::DECLARATION_CONST || prevToken.type == Token::Type::RETURN_TYPE)
                token.type = Token::Type::DATATYPE;
        }
    }

    bufferPosition = subBufferPosition;
    colNum += length;
    return true;
}