#pragma once
#include <pch/Build.h>
#include <string>

enum class TokenType
{
    TOKENTYPE_OP_MODULUS = 37,
    TOKENTYPE_BITWISE_AND = 38,
    TOKENTYPE_LPAREN = 40,
    TOKENTYPE_RPAREN = 41,
    TOKENTYPE_OP_MULTIPLY = 42,
    TOKENTYPE_OP_ADD = 43,
    TOKENTYPE_COMMA = 44,
    TOKENTYPE_OP_SUBTRACT = 45,
    TOKENTYPE_OP_ACCESS = 46,
    TOKENTYPE_OP_DIVIDE = 47,
    TOKENTYPE_DECLARATION = 58,
    TOKENTYPE_SEMICOLON = 59,
    TOKENTYPE_OP_LESS = 60,
    TOKENTYPE_OP_ASSIGN = 61,
    TOKENTYPE_OP_GREATER = 62,
    TOKENTYPE_LBRACE = 123,
    TOKENTYPE_BITWISE_OR = 124,
    TOKENTYPE_RBRACE = 125,

    TOKENTYPE_IDENTIFIER = 256,
    TOKENTYPE_NUMERIC,
    TOKENTYPE_STRING,
    TOKENTYPE_KEYWORD,
    TOKENTYPE_DECLARATION_ASSIGN,
    TOKENTYPE_CONST_DECLARATION,
    TOKENTYPE_CONST_DECLARATION_ASSIGN,
    TOKENTYPE_OP_EQUALS,
    TOKENTYPE_OP_ADD_ASSIGN,
    TOKENTYPE_OP_SUBTRACT_ASSIGN,
    TOKENTYPE_OP_MULTIPLY_ASSIGN,
    TOKENTYPE_OP_DIVIDE_ASSIGN,
    TOKENTYPE_OP_GREATER_EQUALS,
    TOKENTYPE_OP_LESS_EQUALS,
    TOKENTYPE_OP_AND,
    TOKENTYPE_OP_OR,

    TOKEN_TYPE_INVALID = 999
};

class Token
{
public:
    TokenType type = TokenType::TOKEN_TYPE_INVALID;
    std::string value = "";
    int lineNum;
    int charNum;

    static std::string TypeToString(TokenType type)
    {
        if (type == TokenType::TOKENTYPE_IDENTIFIER)
            return "identifier";
        else if (type == TokenType::TOKENTYPE_STRING)
            return "string";
        else if (type == TokenType::TOKENTYPE_NUMERIC)
            return "numeric";
        /*else if (type == TokenType::TOKENTYPE_OPERATOR)
            return "operator";
        else if (type == TokenType::TOKENTYPE_SEPERATOR)
            return "seperator";*/

        return "invalid";
    }
    static TokenType StringToType(std::string input)
    {
        if (input == "identifier")
            return TokenType::TOKENTYPE_IDENTIFIER;
        else if (input == "string")
            return TokenType::TOKENTYPE_STRING;
        else if (input == "numeric")
            return TokenType::TOKENTYPE_NUMERIC;
        /*else if (input == "operator")
            return TokenType::TOKENTYPE_OPERATOR;
        else if (input == "seperator")
            return TokenType::TOKENTYPE_SEPERATOR;*/

        return TokenType::TOKEN_TYPE_INVALID;
    }
};