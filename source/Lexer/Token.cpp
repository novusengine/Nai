#pragma once
#include <pch/Build.h>
#include "Token.h"

const char* OP_NOT = "OP_NOT";
const char* OP_MODULUS = "OP_MODULUS";
const char* OP_BITWISE_AND = "OP_BITWISE_AND";
const char* OPERATOR = "OPERATOR";
const char* LPAREN = "LPAREN";
const char* RPAREN = "RPAREN";
const char* OP_MULTIPLY = "OP_MULTIPLY";
const char* OP_ADD = "OP_ADD";
const char* PARAM_SEPERATOR = "PARAM_SEPERATOR";
const char* OP_SUBTRACT = "OP_SUBTRACT";
const char* ACCESS = "OP_ACCESS";
const char* OP_DIVIDE = "OP_DIVIDE";
const char* DECLARATION = "OP_DECLARATION";
const char* END_OF_LINE = "END_OF_LINE";
const char* OP_LESS = "OP_LESS";
const char* OP_ASSIGN = "OP_ASSIGN";
const char* OP_GREATER = "OP_GREATER";
const char* LBRACE = "LBRACE";
const char* OP_BITWISE_OR = "OP_BITWISE_OR";
const char* RBRACE = "RBRACE";
const char* IDENTIFIER = "IDENTIFIER";
const char* NUMERIC = "NUMERIC";
const char* STRING = "STRING";
const char* FUNCTION_DECLARATION = "FUNCTION_DECLARATION";
const char* FUNCTION_CALL = "FUNCTION_CALL";
const char* LITERAL = "LITERAL";
const char* ENUM = "ENUM";
const char* STRUCT = "STRUCT";
const char* DATATYPE = "DATATYPE";
const char* KEYWORD = "KEYWORD";
const char* KEYWORD_FUNCTION = "KEYWORD_FUNCTION";
const char* KEYWORD_STRUCT = "KEYWORD_STRUCT";
const char* KEYWORD_ENUM = "KEYWORD_ENUM";
const char* KEYWORD_WHILE = "KEYWORD_WHILE";
const char* KEYWORD_IF = "KEYWORD_IF";
const char* KEYWORD_ELSEIF = "KEYWORD_ELSEIF";
const char* KEYWORD_FOR = "KEYWORD_FOR";
const char* KEYWORD_TRUE = "KEYWORD_TRUE";
const char* KEYWORD_FALSE = "KEYWORD_FALSE";
const char* KEYWORD_BREAK = "KEYWORD_BREAK";
const char* KEYWORD_CONTINUE = "KEYWORD_CONTINUE";
const char* KEYWORD_RETURN = "KEYWORD_RETURN";
const char* DECLARATION_ASSIGN = "OP_DECLARATION_ASSIGN";
const char* CONST_DECLARATION = "OP_CONST_DECLARATION";
const char* CONST_DECLARATION_ASSIGN = "OP_CONST_DECLARATION_ASSIGN";
const char* OP_EQUALS = "OP_EQUALS";
const char* OP_NOT_EQUALS = "OP_NOT_EQUALS";
const char* OP_ADD_ASSIGN = "OP_ADD_ASSIGN";
const char* OP_SUBTRACT_ASSIGN = "OP_SUBTRACT_ASSIGN";
const char* OP_MULTIPLY_ASSIGN = "OP_MULTIPLY_ASSIGN";
const char* OP_DIVIDE_ASSIGN = "OP_DIVIDE_ASSIGN";
const char* OP_GREATER_EQUALS = "OP_GREATER_EQUALS";
const char* OP_LESS_EQUALS = "OP_LESS_EQUALS";
const char* OP_INCREMENT = "OP_INCREMENT";
const char* OP_DECREMENT = "OP_DECREMENT";
const char* OP_AND = "OP_AND";
const char* OP_OR = "OP_OR";
const char* OP_BITWISE_AND_ASSIGN = "OP_BITWISE_AND_ASSIGN";
const char* OP_BITWISE_OR_ASSIGN = "OP_BITWISE_OR_ASSIGN";
const char* OP_BITWISE_SHIFT_RIGHT = "OP_BITWISE_SHIFT_RIGHT";
const char* OP_BITWISE_SHIFT_LEFT = "OP_BITWISE_SHIFT_LEFT";
const char* OP_RETURN_TYPE = "OP_RETURN_TYPE";

std::string Token::TypeToString(TokenType type)
{
    switch (type)
    {
    case TokenType::OP_NOT:
        return OP_NOT;
    case TokenType::OP_MODULUS:
        return OP_MODULUS;
    case TokenType::OP_BITWISE_AND:
        return OP_BITWISE_AND;
    case TokenType::OP_MULTIPLY:
        return OP_MULTIPLY;
    case TokenType::OP_ADD:
        return OP_ADD;
    case TokenType::OP_SUBTRACT:
        return OP_SUBTRACT;
    case TokenType::OP_LESS:
        return OP_LESS;
    case TokenType::OP_ASSIGN:
        return OP_ASSIGN;
    case TokenType::OP_GREATER:
        return OP_GREATER;
    case TokenType::OP_BITWISE_OR:
        return OP_BITWISE_OR;
    case TokenType::OP_DIVIDE:
        return OP_DIVIDE;
    case TokenType::ACCESS:
        return ACCESS;
    case TokenType::DECLARATION:
        return DECLARATION;
    /*case TokenType::OP_DECLARATION_ASSIGN:
        return OP_DECLARATION_ASSIGN;
    case TokenType::OP_CONST_DECLARATION:
        return OP_CONST_DECLARATION;
    case TokenType::OP_CONST_DECLARATION_ASSIGN:
        return OP_CONST_DECLARATION_ASSIGN;
    case TokenType::OP_EQUALS:
        return OP_EQUALS;
    case TokenType::OP_NOT_EQUALS:
        return OP_NOT_EQUALS;
    case TokenType::OP_ADD_ASSIGN:
        return OP_ADD_ASSIGN;
    case TokenType::OP_SUBTRACT_ASSIGN:
        return OP_SUBTRACT_ASSIGN;
    case TokenType::OP_MULTIPLY_ASSIGN:
        return OP_MULTIPLY_ASSIGN;
    case TokenType::OP_DIVIDE_ASSIGN:
        return OP_DIVIDE_ASSIGN;
    case TokenType::OP_GREATER_EQUALS:
        return OP_GREATER_EQUALS;
    case TokenType::OP_LESS_EQUALS:
        return OP_LESS_EQUALS;
    case TokenType::OP_INCREMENT:
        return OP_INCREMENT;
    case TokenType::OP_DECREMENT:
        return OP_DECREMENT;
    case TokenType::OP_AND:
        return OP_AND;
    case TokenType::OP_OR:
        return OP_OR;
    case TokenType::OP_BITWISE_AND_ASSIGN:
        return OP_BITWISE_AND_ASSIGN;
    case TokenType::OP_BITWISE_OR_ASSIGN:
        return OP_BITWISE_OR_ASSIGN;
    case TokenType::OP_BITWISE_SHIFT_RIGHT:
        return OP_BITWISE_SHIFT_RIGHT;
    case TokenType::OP_BITWISE_SHIFT_LEFT:
        return OP_BITWISE_SHIFT_LEFT;
    case TokenType::OP_RETURN_TYPE:
        return OP_RETURN_TYPE;*/

    case TokenType::OPERATOR:
        return OPERATOR;
    case TokenType::LPAREN:
        return LPAREN;
    case TokenType::RPAREN:
        return RPAREN;
    case TokenType::PARAM_SEPERATOR:
        return PARAM_SEPERATOR;
    case TokenType::END_OF_LINE:
        return END_OF_LINE;
    case TokenType::LBRACE:
        return LBRACE;
    case TokenType::RBRACE:
        return RBRACE;

    case TokenType::IDENTIFIER:
        return IDENTIFIER;
    /*case TokenType::NUMERIC:
        return NUMERIC;
    case TokenType::STRING:
        return STRING;
    case TokenType::FUNCTION_DECLARATION:
        return FUNCTION_DECLARATION;
    case TokenType::FUNCTION_CALL:
        return FUNCTION_CALL;*/
    case TokenType::LITERAL:
        return LITERAL;
    case TokenType::ENUM:
        return ENUM;
    case TokenType::STRUCT:
        return STRUCT;
    case TokenType::DATATYPE:
        return DATATYPE;
    case TokenType::KEYWORD:
        return KEYWORD;
    /*case TokenType::KEYWORD_FUNCTION:
        return KEYWORD_FUNCTION;
    case TokenType::KEYWORD_STRUCT:
        return KEYWORD_STRUCT;
    case TokenType::KEYWORD_ENUM:
        return KEYWORD_ENUM;
    case TokenType::KEYWORD_WHILE:
        return KEYWORD_WHILE;
    case TokenType::KEYWORD_IF:
        return KEYWORD_IF;
    case TokenType::KEYWORD_FOR:
        return KEYWORD_FOR;
    case TokenType::KEYWORD_TRUE:
        return KEYWORD_TRUE;
    case TokenType::KEYWORD_FALSE:
        return KEYWORD_FALSE;
    case TokenType::KEYWORD_CONTINUE:
        return KEYWORD_CONTINUE;
    case TokenType::KEYWORD_RETURN:
        return KEYWORD_RETURN;
    case TokenType::KEYWORD_BREAK:
        return KEYWORD_BREAK*/

    default:
        break;
    }

    return "invalid";
}
TokenType Token::StringToType(std::string input)
{
    if (input == OP_NOT)
        return TokenType::OP_NOT;
    else if (input == OP_MODULUS)
        return TokenType::OP_MODULUS;
    else if (input == OP_BITWISE_AND)
        return TokenType::OP_BITWISE_AND;
    else if (input == OP_MULTIPLY)
        return TokenType::OP_MULTIPLY;
    else if (input == OP_ADD)
        return TokenType::OP_ADD;
    else if (input == OP_SUBTRACT)
        return TokenType::OP_SUBTRACT;
    else if (input == ACCESS)
        return TokenType::ACCESS;
    else if (input == OP_DIVIDE)
        return TokenType::OP_DIVIDE;
    else if (input == DECLARATION)
        return TokenType::DECLARATION;
    else if (input == OP_LESS)
        return TokenType::OP_LESS;
    else if (input == OP_ASSIGN)
        return TokenType::OP_ASSIGN;
    else if (input == OP_GREATER)
        return TokenType::OP_GREATER;
    else if (input == OP_BITWISE_OR)
        return TokenType::OP_BITWISE_OR;
    /*else if (input == OP_DECLARATION_ASSIGN)
        return TokenType::OP_DECLARATION_ASSIGN;
    else if (input == OP_CONST_DECLARATION)
        return TokenType::OP_CONST_DECLARATION;
    else if (input == OP_CONST_DECLARATION_ASSIGN)
        return TokenType::OP_CONST_DECLARATION_ASSIGN;
    else if (input == OP_EQUALS)
        return TokenType::OP_EQUALS;
    else if (input == OP_NOT_EQUALS)
        return TokenType::OP_NOT_EQUALS;
    else if (input == OP_ADD_ASSIGN)
        return TokenType::OP_ADD_ASSIGN;
    else if (input == OP_SUBTRACT_ASSIGN)
        return TokenType::OP_SUBTRACT_ASSIGN;
    else if (input == OP_MULTIPLY_ASSIGN)
        return TokenType::OP_MULTIPLY_ASSIGN;
    else if (input == OP_DIVIDE_ASSIGN)
        return TokenType::OP_DIVIDE_ASSIGN;
    else if (input == OP_GREATER_EQUALS)
        return TokenType::OP_GREATER_EQUALS;
    else if (input == OP_LESS_EQUALS)
        return TokenType::OP_LESS_EQUALS;
    else if (input == OP_INCREMENT)
        return TokenType::OP_INCREMENT;
    else if (input == OP_DECREMENT)
        return TokenType::OP_DECREMENT;
    else if (input == OP_AND)
        return TokenType::OP_AND;
    else if (input == OP_OR)
        return TokenType::OP_OR;
    else if (input == OP_BITWISE_AND_ASSIGN)
        return TokenType::OP_BITWISE_AND_ASSIGN;
    else if (input == OP_BITWISE_OR_ASSIGN)
        return TokenType::OP_BITWISE_OR_ASSIGN;
    else if (input == OP_BITWISE_SHIFT_RIGHT)
        return TokenType::OP_BITWISE_SHIFT_RIGHT;
    else if (input == OP_BITWISE_SHIFT_LEFT)
        return TokenType::OP_BITWISE_SHIFT_LEFT;
    else if (input == OP_RETURN_TYPE)
        return TokenType::OP_RETURN_TYPE;*/

    if (input == OPERATOR)
        return TokenType::OPERATOR;
    else if (input == LPAREN)
        return TokenType::LPAREN;
    else if (input == RPAREN)
        return TokenType::RPAREN;
    else if (input == PARAM_SEPERATOR)
        return TokenType::PARAM_SEPERATOR;
    else if (input == END_OF_LINE)
        return TokenType::END_OF_LINE;
    else if (input == LBRACE)
        return TokenType::LBRACE;
    else if (input == RBRACE)
        return TokenType::RBRACE;

    else if (input == IDENTIFIER)
        return TokenType::IDENTIFIER;
    /* else if (input == NUMERIC)
         return TokenType::NUMERIC;
     else if (input == FUNCTION_DECLARATION)
         return TokenType::FUNCTION_DECLARATION;
     else if (input == FUNCTION_CALL)
         return TokenType::FUNCTION_CALL;*/
    else if (input == LITERAL)
        return TokenType::LITERAL;
    else if (input == ENUM)
        return TokenType::ENUM;
    else if (input == STRUCT)
        return TokenType::STRUCT;
    else if (input == KEYWORD)
        return TokenType::KEYWORD;
    /*else if (input == KEYWORD_FUNCTION)
        return TokenType::KEYWORD_FUNCTION;
    else if (input == KEYWORD_STRUCT)
        return TokenType::KEYWORD_STRUCT;
    else if (input == KEYWORD_ENUM)
        return TokenType::KEYWORD_ENUM;
    else if (input == KEYWORD_WHILE)
        return TokenType::KEYWORD_WHILE;
    else if (input == KEYWORD_IF)
        return TokenType::KEYWORD_IF;
    else if (input == KEYWORD_FOR)
        return TokenType::KEYWORD_FOR;
    else if (input == KEYWORD_TRUE)
        return TokenType::KEYWORD_TRUE;
    else if (input == KEYWORD_FALSE)
        return TokenType::KEYWORD_FALSE;
    else if (input == KEYWORD_BREAK)
        return TokenType::KEYWORD_BREAK;
    else if (input == KEYWORD_CONTINUE)
        return TokenType::KEYWORD_CONTINUE;
    else if (input == KEYWORD_RETURN)
        return TokenType::KEYWORD_RETURN;*/

    return TokenType::NONE;
}