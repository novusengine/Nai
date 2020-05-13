#pragma once
#include <pch/Build.h>
#include <string>

enum class TokenType
{
    OP_MODULUS = 37,
    BITWISE_AND = 38,
    LPAREN = 40,
    RPAREN = 41,
    OP_MULTIPLY = 42,
    OP_ADD = 43,
    COMMA = 44,
    OP_SUBTRACT = 45,
    OP_ACCESS = 46,
    OP_DIVIDE = 47,
    DECLARATION = 58,
    SEMICOLON = 59,
    OP_LESS = 60,
    OP_ASSIGN = 61,
    OP_GREATER = 62,
    LBRACE = 123,
    BITWISE_OR = 124,
    RBRACE = 125,

    IDENTIFIER = 256,
    NUMERIC,
    STRING,
    KEYWORD,
    DECLARATION_ASSIGN,
    CONST_DECLARATION,
    CONST_DECLARATION_ASSIGN,
    OP_EQUALS,
    OP_ADD_ASSIGN,
    OP_SUBTRACT_ASSIGN,
    OP_MULTIPLY_ASSIGN,
    OP_DIVIDE_ASSIGN,
    OP_GREATER_EQUALS,
    OP_LESS_EQUALS,
    OP_AND,
    OP_OR,

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
        if (type == TokenType::IDENTIFIER)
            return "identifier";
        else if (type == TokenType::STRING)
            return "string";
        else if (type == TokenType::NUMERIC)
            return "numeric";
        /*else if (type == TokenType::OPERATOR)
            return "operator";
        else if (type == TokenType::SEPERATOR)
            return "seperator";*/

        return "invalid";
    }
    static TokenType StringToType(std::string input)
    {
        if (input == "identifier")
            return TokenType::IDENTIFIER;
        else if (input == "string")
            return TokenType::STRING;
        else if (input == "numeric")
            return TokenType::NUMERIC;
        /*else if (input == "operator")
            return TokenType::OPERATOR;
        else if (input == "seperator")
            return TokenType::SEPERATOR;*/

        return TokenType::TOKEN_TYPE_INVALID;
    }
};