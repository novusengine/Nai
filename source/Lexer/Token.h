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

    static std::string TypeToString(Token_Type type)
    {
        if (type == Token_Type::TOKEN_TYPE_IDENTIFIER)
            return "identifier";
        else if (type == Token_Type::TOKEN_TYPE_STRING)
            return "string";
        else if (type == Token_Type::TOKEN_TYPE_NUMERIC)
            return "numeric";
        else if (type == Token_Type::TOKEN_TYPE_OPERATOR)
            return "operator";
        else if (type == Token_Type::TOKEN_TYPE_SEPERATOR)
            return "seperator";

        return "invalid";
    }
    static Token_Type StringToType(std::string input)
    {
        if (input == "identifier")
            return Token_Type::TOKEN_TYPE_IDENTIFIER;
        else if (input == "string")
            return Token_Type::TOKEN_TYPE_STRING;
        else if (input == "numeric")
            return Token_Type::TOKEN_TYPE_NUMERIC;
        else if (input == "operator")
            return Token_Type::TOKEN_TYPE_OPERATOR;
        else if (input == "seperator")
            return Token_Type::TOKEN_TYPE_SEPERATOR;

        return Token_Type::TOKEN_TYPE_INVALID;
    }
};