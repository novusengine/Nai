#pragma once
#include <pch/Build.h>
#include <string>

enum class TokenType
{
    OP_NOT,
    OP_MODULUS,
    OP_BITWISE_AND,
    OP_MULTIPLY,
    OP_ADD,
    OP_SUBTRACT,
    OP_DIVIDE,
    OP_LESS,
    OP_ASSIGN,
    OP_GREATER,
    OP_BITWISE_OR,
    ACCESS,
    DECLARATION,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    PARAM_SEPERATOR,
    END_OF_LINE,

    IDENTIFIER = 256,
    LITERAL,
    ENUM,
    STRUCT,
    DATATYPE,
    KEYWORD,
    OPERATOR,
    NONE = 999
};

enum class TokenSubType
{
    NUMERIC = 256,
    STRING,
    KEYWORD_FUNCTION,
    KEYWORD_STRUCT,
    KEYWORD_ENUM,
    KEYWORD_BREAK,
    KEYWORD_CONTINUE,
    KEYWORD_RETURN,
    KEYWORD_WHILE,
    KEYWORD_IF,
    KEYWORD_ELSEIF,
    KEYWORD_ELSE,
    KEYWORD_FOR,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    DECLARATION_ASSIGN,
    CONST_DECLARATION,
    CONST_DECLARATION_ASSIGN,
    OP_EQUALS,
    OP_NOT_EQUALS,
    OP_GREATER_EQUALS,
    OP_LESS_EQUALS,
    OP_INCREMENT,
    OP_DECREMENT,
    OP_AND,
    OP_OR,
    OP_BITWISE_SHIFT_RIGHT,
    OP_BITWISE_SHIFT_LEFT,
    OP_ADD_ASSIGN,
    OP_SUBTRACT_ASSIGN,
    OP_MULTIPLY_ASSIGN,
    OP_DIVIDE_ASSIGN,
    OP_BITWISE_AND_ASSIGN,
    OP_BITWISE_OR_ASSIGN,
    OP_RETURN_TYPE,
    FUNCTION_DECLARATION,
    FUNCTION_CALL,
    NONE = 999
};

class Token
{
public:
    TokenType type = TokenType::NONE;
    TokenSubType subType = TokenSubType::NONE;
    
    char* value;
    int valueSize;

    int lineNum;
    int colNum;

    bool IsOperator() const
    {
        if (type == TokenType::OPERATOR)
            return true;

        if (type >= TokenType::OP_NOT && type <= TokenType::OP_BITWISE_OR)
            return true;

        return false;
    }
    bool IsExpressionOperator() const
    {
        if (type == TokenType::OPERATOR &&
            subType >= TokenSubType::OP_EQUALS &&
            subType <= TokenSubType::OP_BITWISE_SHIFT_LEFT)
            return true;

        if (type == TokenType::OP_ASSIGN && 
            subType == TokenSubType::NONE)
            return false;

        if (type >= TokenType::OP_NOT &&
            type <= TokenType::OP_BITWISE_OR)
            return true;

        return false;
    }
    bool IsAssignOperator() const
    {
        if (type == TokenType::OP_ASSIGN)
            return true;

        if (subType >= TokenSubType::OP_ADD_ASSIGN && 
            subType <= TokenSubType::OP_BITWISE_OR_ASSIGN)
            return true;
    
        return false;
    }

    static std::string TypeToString(TokenType type);
    static TokenType StringToType(std::string input);
};