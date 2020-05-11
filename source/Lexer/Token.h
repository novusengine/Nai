#pragma once
#include <pch/Build.h>
#include <string>

enum class Token_Type
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_NUMERIC,
    TOKEN_TYPE_STRING,
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