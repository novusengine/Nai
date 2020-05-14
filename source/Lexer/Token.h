#pragma once
#include <pch/Build.h>
#include <string>

enum class TokenType
{
    OP_NOT = 33,
    OP_MODULUS = 37,
    OP_BITWISE_AND = 38,
    LPAREN = 40,
    RPAREN = 41,
    OP_MULTIPLY = 42,
    OP_ADD = 43,
    PARAM_SEPERATOR = 44,
    OP_SUBTRACT = 45,
    OP_ACCESS = 46,
    OP_DIVIDE = 47,
    OP_DECLARATION = 58,
    END_OF_LINE = 59,
    OP_LESS = 60,
    OP_ASSIGN = 61,
    OP_GREATER = 62,
    LBRACE = 123,
    OP_BITWISE_OR = 124,
    RBRACE = 125,

    IDENTIFIER = 256,
    NUMERIC,
    STRING,
    FUNCTION_DECLARATION,
    FUNCTION_CALL,
    ENUM,
    STRUCT,
    DATATYPE,
    KEYWORD_FUNCTION,
    KEYWORD_STRUCT,
    KEYWORD_ENUM,
    KEYWORD_WHILE,
    KEYWORD_IF,
    KEYWORD_FOR,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    KEYWORD_BREAK,
    KEYWORD_CONTINUE,
    KEYWORD_RETURN,
    OP_DECLARATION_ASSIGN,
    OP_CONST_DECLARATION,
    OP_CONST_DECLARATION_ASSIGN,
    OP_EQUALS,
    OP_NOT_EQUALS,
    OP_ADD_ASSIGN,
    OP_SUBTRACT_ASSIGN,
    OP_MULTIPLY_ASSIGN,
    OP_DIVIDE_ASSIGN,
    OP_GREATER_EQUALS,
    OP_LESS_EQUALS,
    OP_INCREMENT,
    OP_DECREMENT,
    OP_AND,
    OP_OR,
    OP_BITWISE_AND_ASSIGN,
    OP_BITWISE_OR_ASSIGN,
    OP_BITWISE_SHIFT_RIGHT,
    OP_BITWISE_SHIFT_LEFT,
    OP_RETURN_TYPE,

    TOKEN_TYPE_INVALID = 999
};

class Token
{
public:
    TokenType type = TokenType::TOKEN_TYPE_INVALID;
    std::string value = "";
    int lineNum;
    int colNum;

    static std::string TypeToString(TokenType type);
    static TokenType StringToType(std::string input);
};