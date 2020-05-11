#pragma once
#include <pch/Build.h>
#include <string>

enum class Token_Type
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_LITERAL,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SEPERATOR,

    TOKEN_TYPE_INVALID = 999
};

class Token
{
public:
    Token_Type type = Token_Type::TOKEN_TYPE_INVALID;
    std::string value = "";
    int lineNum;
    int charNum;
};