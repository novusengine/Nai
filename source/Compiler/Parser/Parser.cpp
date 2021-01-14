#include <pch/Build.h>
#include <execution>
#include <stack>
#include <sstream>

#include "Parser.h"
#include "../../Memory/BlockAllocator.h"
#include <Utils/DebugHandler.h>
#include <robin_hood.h>

enum class ParseRuleSet
{
    NONE,
    ATTRIBUTE,
    ATTRIBUTE_ASSIGNMENT,
    ATTRIBUTE_OPTION,
    ATTRIBUTE_OPTION_VALUE,
    ATTRIBUTE_OPTION_NEXT,

    FUNCTION,
    FUNCTION_EXPRESSION,
    FUNCTION_EXPRESSION_IDENTIFIER,
    FUNCTION_EXPRESSION_IDENTIFIER_ACCESS,
    FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS,
    FUNCTION_EXPRESSION_SEQUENCE,
    FUNCTION_PARAMETER,
    FUNCTION_PARAMETER_NEXT,
    FUNCTION_RETURN_TYPE,
    FUNCTION_BODY,
    FUNCTION_IDENTIFIER,
    FUNCTION_IDENTIFIER_ACTION,
    FUNCTION_IDENTIFIER_MULTI_DECLARE,
    FUNCTION_IDENTIFIER_MULTI_DECLARE_ASSIGN,
    FUNCTION_IDENTIFIER_MULTI_DECLARE_SEQUENCE,
    FUNCTION_IDENTIFIER_ACCESS,
    FUNCTION_IDENTIFIER_ARRAY_ACCESS,
    FUNCTION_IF,
    FUNCTION_IF_NEXT,
    FUNCTION_WHILE,
    FUNCTION_FOR,
    FUNCTION_FOR_HEADER_IDENTIFIER,
    FUNCTION_FOR_HEADER_IDENTIFIER_ACTION,
    FUNCTION_FOR_HEADER_CONDITION,
    FUNCTION_FOR_HEADER_ACTION,
    FUNCTION_FOR_HEADER_ACTION_TYPE,
    FUNCTION_FOR_HEADER_ACTION_ACCESS,
    FUNCTION_FOREACH,
    FUNCTION_FOREACH_IDENTIFIER,
    FUNCTION_FOREACH_IDENTIFIER_ACTION,
    FUNCTION_FOREACH_IN,
    FUNCTION_FOREACH_IDENTIFIER_SEQUENCE,
    FUNCTION_FOREACH_IDENTIFIER_ACCESS,
    FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS,
    FUNCTION_CALL_PARAMETER_LIST,
    FUNCTION_CALL_PARAMETER,
    FUNCTION_CALL_PARAMETER_SEQUENCE,


    STRUCT,
    STRUCT_BODY,
    STRUCT_EXPRESSION,
    STRUCT_EXPRESSION_IDENTIFIER,
    STRUCT_EXPRESSION_SEQUENCE,
    STRUCT_IDENTIFIER,
    STRUCT_IDENTIFIER_ACTION,

    ENUM,
    ENUM_BODY,
    ENUM_EXPRESSION,
    ENUM_EXPRESSION_SEQUENCE,
    ENUM_DECLARATION,
    ENUM_IDENTIFIER_ACTION,
    ENUM_IDENTIFIER_ACTION_SEQUENCE,

    IDENTIFIER,
    STRING,
    DECLARATION,
    DATATYPE,
    DATATYPE_SEQUENCE,
    DECLARATION_ASSIGNMENT,
    DECLARATION_ASSIGNMENT_ACTION,
    DECLARATION_ASSIGNMENT_FORCE,
    ASSIGNMENT,
    ACCESS,
    PARAM_SEPERATOR,
    OPEN_PAREN,
    OPEN_BRACE,
    OPEN_BRACKET,
    CLOSE_PAREN,
    CLOSE_BRACE,
    CLOSE_BRACKET,
    END_OF_LINE
};

bool Parser::Process(ModuleInfo& moduleInfo)
{
    if (!CheckSyntax(moduleInfo))
        return false;

    if (!CreateAST(moduleInfo))
        return false;

    if (!CheckSemantics(moduleInfo))
        return false;

    return true;
}

bool Parser::CheckSyntax(ModuleInfo& moduleInfo)
{
    std::atomic<int> errorCount = 0;

    std::for_each(std::execution::par, moduleInfo.compileUnits.begin(), moduleInfo.compileUnits.end(),
        [&errorCount](CompileUnit& compileUnit)
        {
            int numTokens = static_cast<int>(compileUnit.tokens.size());
            bool localDidError = numTokens == 0;

            std::stack<ParseRuleSet> ruleSetStack;
            std::stack<Token::Type> tokenTypeStack;
            tokenTypeStack.push(Token::Type::NONE);

            std::string_view currentAttributeName = "";

            if (numTokens > 0)
            {
                if (compileUnit.type == CompileUnit::Type::FUNCTION)
                {
                    ruleSetStack.push(ParseRuleSet::FUNCTION);
                }
                else if (compileUnit.type == CompileUnit::Type::STRUCT)
                {
                    ruleSetStack.push(ParseRuleSet::STRUCT);
                }
                else if (compileUnit.type == CompileUnit::Type::ENUM)
                {
                    ruleSetStack.push(ParseRuleSet::ENUM);
                }                    

                const Token& firstToken = compileUnit.tokens[0];
                if (firstToken.type == Token::Type::ATTRIBUTE)
                {
                    ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                    ruleSetStack.push(ParseRuleSet::ATTRIBUTE_OPTION);
                    ruleSetStack.push(ParseRuleSet::OPEN_BRACKET);
                    ruleSetStack.push(ParseRuleSet::ATTRIBUTE);
                }
            }

            for (int tokenIndex = 0; tokenIndex < numTokens;)
            {
                const Token& currentToken = compileUnit.tokens[tokenIndex];
                const Token::Type& expectedTokenType = tokenTypeStack.top();

                if (currentToken.type == expectedTokenType)
                {
                    tokenIndex += 1;
                    tokenTypeStack.pop();
                    continue;
                }

                const ParseRuleSet& rule = ruleSetStack.top();

                if (ruleSetStack.size() > 1)
                    ruleSetStack.pop();

                switch (rule)
                {
                    case ParseRuleSet::ATTRIBUTE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ATTRIBUTE:
                                tokenTypeStack.push(Token::Type::ATTRIBUTE);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::ATTRIBUTE_ASSIGNMENT:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ASSIGN:
                                tokenTypeStack.push(Token::Type::ASSIGN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::ATTRIBUTE_OPTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::ATTRIBUTE_OPTION_NEXT);
                                ruleSetStack.push(ParseRuleSet::ATTRIBUTE_OPTION_VALUE);
                                ruleSetStack.push(ParseRuleSet::ATTRIBUTE_ASSIGNMENT);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);

                                currentAttributeName = currentToken.stringview;
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::ATTRIBUTE_OPTION_VALUE:
                    {
                        bool didError = true;

                        if (currentAttributeName == "ParseResult")
                        {
                            if (currentToken.type == Token::Type::KEYWORD_TRUE)
                            {
                                compileUnit.attributes.parseResult = true;
                                didError = false;
                            }
                            else if (currentToken.type == Token::Type::KEYWORD_FALSE)
                            {
                                compileUnit.attributes.parseResult = false;
                                didError = false;
                            }
                        }
                        else if (currentAttributeName == "Name")
                        {
                            if (currentToken.type == Token::Type::STRING)
                            {
                                compileUnit.attributes.name = currentToken.stringview;
                                didError = false;
                            }
                        }

                        if (didError == true)
                        {
                            localDidError = true;
                        }
                        else
                        {
                            tokenTypeStack.push(currentToken.type);
                        }

                        break;
                    }
                    case ParseRuleSet::ATTRIBUTE_OPTION_NEXT:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PARAM_SEPERATOR:
                                tokenTypeStack.push(Token::Type::PARAM_SEPERATOR);

                                ruleSetStack.push(ParseRuleSet::ATTRIBUTE_OPTION);
                                break;

                            case Token::Type::RBRACKET:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }

                    case ParseRuleSet::FUNCTION_EXPRESSION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            case Token::Type::NUMERIC_SIGNED:
                            case Token::Type::NUMERIC_UNSIGNED:
                            case Token::Type::NUMERIC_FLOAT:
                            case Token::Type::NUMERIC_DOUBLE:
                            case Token::Type::NUMERIC_HEX:
                            case Token::Type::STRING:
                            case Token::Type::KEYWORD_TRUE:
                            case Token::Type::KEYWORD_FALSE:
                            case Token::Type::KEYWORD_NULLPTR:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                                break;

                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                break;

                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS);
                                break;

                            case Token::Type::LBRACKET:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASTERISK:
                            case Token::Type::PLUS:
                            case Token::Type::MINUS:
                            case Token::Type::SLASH:
                            case Token::Type::PERCENT:
                            case Token::Type::AMPERSAND:
                            case Token::Type::BITSHIFT_LEFT:
                            case Token::Type::BITSHIFT_RIGHT:
                            case Token::Type::CARET:
                            case Token::Type::PIPE:
                            case Token::Type::EQUALS:
                            case Token::Type::LESS_THAN:
                            case Token::Type::GREATER_THAN:
                            case Token::Type::AND:
                            case Token::Type::OR:
                            case Token::Type::RPAREN:
                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                break;

                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::ACCESS);
                                break;

                            case Token::Type::LBRACKET:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASTERISK:
                            case Token::Type::PLUS:
                            case Token::Type::MINUS:
                            case Token::Type::SLASH:
                            case Token::Type::PERCENT:
                            case Token::Type::AMPERSAND:
                            case Token::Type::BITSHIFT_LEFT:
                            case Token::Type::BITSHIFT_RIGHT:
                            case Token::Type::CARET:
                            case Token::Type::PIPE:
                            case Token::Type::EQUALS:
                            case Token::Type::LESS_THAN:
                            case Token::Type::GREATER_THAN:
                            case Token::Type::AND:
                            case Token::Type::OR:
                            case Token::Type::RPAREN:
                            case Token::Type::PARAM_SEPERATOR:
                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::ACCESS);
                                break;

                            case Token::Type::LBRACKET:
                                tokenTypeStack.push(Token::Type::LBRACKET);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS);
                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASTERISK:
                            case Token::Type::PLUS:
                            case Token::Type::MINUS:
                            case Token::Type::SLASH:
                            case Token::Type::PERCENT:
                            case Token::Type::AMPERSAND:
                            case Token::Type::BITSHIFT_LEFT:
                            case Token::Type::BITSHIFT_RIGHT:
                            case Token::Type::CARET:
                            case Token::Type::PIPE:
                            case Token::Type::EQUALS:
                            case Token::Type::LESS_THAN:
                            case Token::Type::GREATER_THAN:
                            case Token::Type::AND:
                            case Token::Type::OR:
                            case Token::Type::RPAREN:
                            case Token::Type::PARAM_SEPERATOR:
                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ASTERISK:
                            case Token::Type::PLUS:
                            case Token::Type::MINUS:
                            case Token::Type::SLASH:
                            case Token::Type::PERCENT:
                            case Token::Type::AMPERSAND:
                            case Token::Type::BITSHIFT_LEFT:
                            case Token::Type::BITSHIFT_RIGHT:
                            case Token::Type::CARET:
                            case Token::Type::PIPE:
                            case Token::Type::EQUALS:
                            case Token::Type::LESS_THAN:
                            case Token::Type::GREATER_THAN:
                            case Token::Type::AND:
                            case Token::Type::OR:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;

                            case Token::Type::PERIOD:
                            case Token::Type::RPAREN:
                            case Token::Type::RBRACE:
                            case Token::Type::RBRACKET:
                            case Token::Type::PARAM_SEPERATOR:
                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }

                    case ParseRuleSet::FUNCTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_FUNCTION:
                                tokenTypeStack.push(Token::Type::KEYWORD_FUNCTION);

                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_RETURN_TYPE);
                                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_PARAMETER);
                                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);

                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::FUNCTION_PARAMETER:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_PARAMETER_NEXT);
                                ruleSetStack.push(ParseRuleSet::DECLARATION);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::FUNCTION_PARAMETER_NEXT:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PARAM_SEPERATOR:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_PARAMETER);
                                ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::FUNCTION_RETURN_TYPE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::RETURN_TYPE:
                                tokenTypeStack.push(Token::Type::RETURN_TYPE);

                                ruleSetStack.push(ParseRuleSet::DATATYPE_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::DATATYPE);
                                break;

                            case Token::Type::LBRACE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_BODY:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER);
                                break;

                            case Token::Type::KEYWORD_IF:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IF);
                                break;

                            case Token::Type::KEYWORD_WHILE:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_WHILE);
                                break;

                            case Token::Type::KEYWORD_FOR:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR);
                                break;

                            case Token::Type::KEYWORD_FOREACH:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH);
                                break;

                            case Token::Type::KEYWORD_CONTINUE:
                            case Token::Type::KEYWORD_BREAK:
                                tokenTypeStack.push(Token::Type::END_OF_LINE);
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                break;

                            case Token::Type::KEYWORD_RETURN:
                                tokenTypeStack.push(Token::Type::END_OF_LINE);
                                tokenTypeStack.push(Token::Type::KEYWORD_RETURN);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;

                            case Token::Type::RBRACE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_IDENTIFIER:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACTION);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_IDENTIFIER_ACTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::DECLARATION:
                            case Token::Type::DECLARATION_CONST:
                                ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT);
                                ruleSetStack.push(ParseRuleSet::DECLARATION);
                                break;

                            case Token::Type::PARAM_SEPERATOR:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE);
                                break;

                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                break;

                            case Token::Type::LBRACKET:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ARRAY_ACCESS);
                                break;

                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASSIGN:
                            case Token::Type::MODULUS_EQUALS:
                            case Token::Type::BITWISE_AND_EQUALS:
                            case Token::Type::MULTIPLY_EQUALS:
                            case Token::Type::PLUS_EQUALS:
                            case Token::Type::MINUS_EQUALS:
                            case Token::Type::DIVIDE_EQUALS:
                            case Token::Type::BITSHIFT_LEFT_EQUALS:
                            case Token::Type::BITSHIFT_RIGHT_EQUALS:
                            case Token::Type::POW_EQUALS:
                            case Token::Type::BITWISE_OR_EQUALS:
                                ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PARAM_SEPERATOR:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                                break;

                            case Token::Type::ASSIGN:
                                tokenTypeStack.push(Token::Type::ASSIGN);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_ASSIGN);
                                break;

                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_ASSIGN:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            case Token::Type::LBRACKET:
                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACKET);
                                break;

                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_SEQUENCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PARAM_SEPERATOR:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                                break;

                            case Token::Type::RBRACKET:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                break;

                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::ACCESS);
                                break;

                            case Token::Type::LBRACKET:
                                tokenTypeStack.push(Token::Type::LBRACKET);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ARRAY_ACCESS);
                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASSIGN:
                            case Token::Type::MODULUS_EQUALS:
                            case Token::Type::BITWISE_AND_EQUALS:
                            case Token::Type::MULTIPLY_EQUALS:
                            case Token::Type::PLUS_EQUALS:
                            case Token::Type::MINUS_EQUALS:
                            case Token::Type::DIVIDE_EQUALS:
                            case Token::Type::BITSHIFT_LEFT_EQUALS:
                            case Token::Type::BITSHIFT_RIGHT_EQUALS:
                            case Token::Type::POW_EQUALS:
                            case Token::Type::BITWISE_OR_EQUALS:
                                ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                                break;

                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_IDENTIFIER_ARRAY_ACCESS:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::ACCESS);
                                break;

                            case Token::Type::LBRACKET:
                                tokenTypeStack.push(Token::Type::LBRACKET);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ARRAY_ACCESS);
                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASSIGN:
                            case Token::Type::MODULUS_EQUALS:
                            case Token::Type::BITWISE_AND_EQUALS:
                            case Token::Type::MULTIPLY_EQUALS:
                            case Token::Type::PLUS_EQUALS:
                            case Token::Type::MINUS_EQUALS:
                            case Token::Type::DIVIDE_EQUALS:
                            case Token::Type::BITSHIFT_LEFT_EQUALS:
                            case Token::Type::BITSHIFT_RIGHT_EQUALS:
                            case Token::Type::POW_EQUALS:
                            case Token::Type::BITWISE_OR_EQUALS:
                                ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }

                    case ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST:
                    {
                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;
                    }
                    case ParseRuleSet::FUNCTION_CALL_PARAMETER:
                    {
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;
                    }
                    case ParseRuleSet::FUNCTION_CALL_PARAMETER_SEQUENCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PARAM_SEPERATOR:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER);
                                ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }

                    case ParseRuleSet::FUNCTION_IF:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_IF:
                            case Token::Type::KEYWORD_ELSEIF:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_IF_NEXT);
                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                                break;

                            case Token::Type::KEYWORD_ELSE:
                                tokenTypeStack.push(Token::Type::KEYWORD_ELSE);

                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACE);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_IF_NEXT:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_ELSEIF:
                            case Token::Type::KEYWORD_ELSE:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_IF);
                                break;

                            default:
                                break;
                        }
                        break;
                    }

                    case ParseRuleSet::FUNCTION_WHILE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_WHILE:
                                tokenTypeStack.push(Token::Type::KEYWORD_WHILE);

                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }

                    case ParseRuleSet::FUNCTION_FOR:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_FOR:
                                tokenTypeStack.push(Token::Type::KEYWORD_FOR);

                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_ACTION);
                                ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_CONDITION);
                                ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOR_HEADER_IDENTIFIER:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_IDENTIFIER_ACTION);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOR_HEADER_IDENTIFIER_ACTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::DECLARATION:
                            case Token::Type::DECLARATION_CONST:
                                ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT);
                                ruleSetStack.push(ParseRuleSet::DECLARATION);
                                break;

                            case Token::Type::ASSIGN:
                                ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOR_HEADER_CONDITION:
                    {
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOR_HEADER_ACTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_ACTION_TYPE);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOR_HEADER_ACTION_TYPE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_ACTION_ACCESS);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASSIGN:
                            case Token::Type::MODULUS_EQUALS:
                            case Token::Type::BITWISE_AND_EQUALS:
                            case Token::Type::MULTIPLY_EQUALS:
                            case Token::Type::PLUS_EQUALS:
                            case Token::Type::MINUS_EQUALS:
                            case Token::Type::DIVIDE_EQUALS:
                            case Token::Type::BITSHIFT_LEFT_EQUALS:
                            case Token::Type::BITSHIFT_RIGHT_EQUALS:
                            case Token::Type::POW_EQUALS:
                            case Token::Type::BITWISE_OR_EQUALS:
                                ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOR_HEADER_ACTION_ACCESS:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_ACTION_ACCESS);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                break;

                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_ACTION_ACCESS);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::ACCESS);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASSIGN:
                            case Token::Type::MODULUS_EQUALS:
                            case Token::Type::BITWISE_AND_EQUALS:
                            case Token::Type::MULTIPLY_EQUALS:
                            case Token::Type::PLUS_EQUALS:
                            case Token::Type::MINUS_EQUALS:
                            case Token::Type::DIVIDE_EQUALS:
                            case Token::Type::BITSHIFT_LEFT_EQUALS:
                            case Token::Type::BITSHIFT_RIGHT_EQUALS:
                            case Token::Type::POW_EQUALS:
                            case Token::Type::BITWISE_OR_EQUALS:
                                ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                            }
                        break;
                    }

                    case ParseRuleSet::FUNCTION_FOREACH:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_FOREACH:
                                tokenTypeStack.push(Token::Type::KEYWORD_FOREACH);

                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACTION);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PARAM_SEPERATOR:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IN);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                                break;

                            case Token::Type::KEYWORD_IN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOREACH_IN:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_IN:
                                tokenTypeStack.push(Token::Type::KEYWORD_IN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_SEQUENCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                break;

                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS);
                                break;

                            case Token::Type::LBRACKET:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }

                    case ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                break;

                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::ACCESS);
                                break;

                            case Token::Type::LBRACKET:
                                tokenTypeStack.push(Token::Type::LBRACKET);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS);
                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PERIOD:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::ACCESS);
                                break;

                            case Token::Type::LBRACKET:
                                tokenTypeStack.push(Token::Type::LBRACKET);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS);
                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;

                            case Token::Type::RPAREN:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }

                    case ParseRuleSet::STRUCT:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_STRUCT:
                                tokenTypeStack.push(Token::Type::KEYWORD_STRUCT);

                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                                ruleSetStack.push(ParseRuleSet::STRUCT_BODY);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::STRUCT_BODY:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::STRUCT_BODY);
                                ruleSetStack.push(ParseRuleSet::STRUCT_IDENTIFIER);
                                break;

                            case Token::Type::KEYWORD_FUNCTION:
                                ruleSetStack.push(ParseRuleSet::STRUCT_BODY);
                                ruleSetStack.push(ParseRuleSet::FUNCTION);
                                break;

                            case Token::Type::RBRACE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::STRUCT_EXPRESSION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION_IDENTIFIER);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            case Token::Type::NUMERIC_SIGNED:
                            case Token::Type::NUMERIC_UNSIGNED:
                            case Token::Type::NUMERIC_FLOAT:
                            case Token::Type::NUMERIC_DOUBLE:
                            case Token::Type::NUMERIC_HEX:
                            case Token::Type::STRING:
                            case Token::Type::KEYWORD_TRUE:
                            case Token::Type::KEYWORD_FALSE:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION_SEQUENCE);
                                break;

                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                                ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION);
                                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::STRUCT_EXPRESSION_IDENTIFIER:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                                break;

                            case Token::Type::INCREMENT:
                            case Token::Type::DECREMENT:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            case Token::Type::ASTERISK:
                            case Token::Type::PLUS:
                            case Token::Type::MINUS:
                            case Token::Type::SLASH:
                            case Token::Type::PERCENT:
                            case Token::Type::AMPERSAND:
                            case Token::Type::BITSHIFT_LEFT:
                            case Token::Type::BITSHIFT_RIGHT:
                            case Token::Type::CARET:
                            case Token::Type::PIPE:
                            case Token::Type::EQUALS:
                            case Token::Type::LESS_THAN:
                            case Token::Type::GREATER_THAN:
                            case Token::Type::AND:
                            case Token::Type::OR:
                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::STRUCT_EXPRESSION_SEQUENCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ASTERISK:
                            case Token::Type::PLUS:
                            case Token::Type::MINUS:
                            case Token::Type::SLASH:
                            case Token::Type::PERCENT:
                            case Token::Type::AMPERSAND:
                            case Token::Type::BITSHIFT_LEFT:
                            case Token::Type::BITSHIFT_RIGHT:
                            case Token::Type::CARET:
                            case Token::Type::PIPE:
                            case Token::Type::EQUALS:
                            case Token::Type::LESS_THAN:
                            case Token::Type::GREATER_THAN:
                            case Token::Type::AND:
                            case Token::Type::OR:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION);
                                break;

                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::STRUCT_IDENTIFIER:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                                ruleSetStack.push(ParseRuleSet::STRUCT_IDENTIFIER_ACTION);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::STRUCT_IDENTIFIER_ACTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::DECLARATION:
                                ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT);
                                ruleSetStack.push(ParseRuleSet::DECLARATION);
                                break;

                            case Token::Type::DECLARATION_CONST:
                                ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_FORCE);
                                ruleSetStack.push(ParseRuleSet::DECLARATION);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }

                    case ParseRuleSet::ENUM:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::KEYWORD_ENUM:
                                tokenTypeStack.push(Token::Type::KEYWORD_ENUM);

                                ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                                ruleSetStack.push(ParseRuleSet::ENUM_BODY);
                                ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                                ruleSetStack.push(ParseRuleSet::ENUM_DECLARATION);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::ENUM_BODY:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::ENUM_IDENTIFIER_ACTION);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            case Token::Type::RBRACE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::ENUM_EXPRESSION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                                break;

                            case Token::Type::NUMERIC_SIGNED:
                            case Token::Type::NUMERIC_UNSIGNED:
                            case Token::Type::NUMERIC_FLOAT:
                            case Token::Type::NUMERIC_DOUBLE:
                            case Token::Type::NUMERIC_HEX:
                            case Token::Type::STRING:
                            case Token::Type::KEYWORD_TRUE:
                            case Token::Type::KEYWORD_FALSE:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION_SEQUENCE);
                                break;

                            case Token::Type::LPAREN:
                                ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                                ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION);
                                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                                break;

                            case Token::Type::PARAM_SEPERATOR:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::ENUM_EXPRESSION_SEQUENCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ASTERISK:
                            case Token::Type::PLUS:
                            case Token::Type::MINUS:
                            case Token::Type::SLASH:
                            case Token::Type::PERCENT:
                            case Token::Type::AMPERSAND:
                            case Token::Type::BITSHIFT_LEFT:
                            case Token::Type::BITSHIFT_RIGHT:
                            case Token::Type::CARET:
                            case Token::Type::PIPE:
                            case Token::Type::EQUALS:
                            case Token::Type::LESS_THAN:
                            case Token::Type::GREATER_THAN:
                            case Token::Type::AND:
                            case Token::Type::OR:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION);
                                break;

                            case Token::Type::PARAM_SEPERATOR:
                            case Token::Type::RBRACE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::ENUM_DECLARATION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::DECLARATION:
                                tokenTypeStack.push(Token::Type::DECLARATION);

                                ruleSetStack.push(ParseRuleSet::DATATYPE);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::ENUM_IDENTIFIER_ACTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ASSIGN:
                                tokenTypeStack.push(Token::Type::ASSIGN);

                                ruleSetStack.push(ParseRuleSet::ENUM_IDENTIFIER_ACTION_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION);
                                break;

                            case Token::Type::PARAM_SEPERATOR:
                                ruleSetStack.push(ParseRuleSet::ENUM_IDENTIFIER_ACTION_SEQUENCE);
                                break;

                            case Token::Type::RBRACE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }
                    case ParseRuleSet::ENUM_IDENTIFIER_ACTION_SEQUENCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PARAM_SEPERATOR:
                                tokenTypeStack.push(Token::Type::PARAM_SEPERATOR);

                                ruleSetStack.push(ParseRuleSet::ENUM_BODY);
                                break;

                            case Token::Type::RBRACE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }

                        break;
                    }

                    case ParseRuleSet::IDENTIFIER:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::IDENTIFIER:
                                tokenTypeStack.push(Token::Type::IDENTIFIER);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::STRING:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::STRING:
                                tokenTypeStack.push(Token::Type::STRING);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::DECLARATION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::DECLARATION:
                            case Token::Type::DECLARATION_CONST:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::DATATYPE_SEQUENCE);
                                ruleSetStack.push(ParseRuleSet::DATATYPE);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::DATATYPE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::DATATYPE:
                                tokenTypeStack.push(Token::Type::DATATYPE);

                            default:
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::DATATYPE_SEQUENCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::POINTER:
                                tokenTypeStack.push(Token::Type::POINTER);

                            default:
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::DECLARATION_ASSIGNMENT:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ASSIGN:
                                tokenTypeStack.push(Token::Type::ASSIGN);

                                ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION);
                                break;

                            case Token::Type::END_OF_LINE:
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::UNINITIALIZED:
                            case Token::Type::KEYWORD_NULLPTR:
                                tokenTypeStack.push(currentToken.type);
                                break;

                            default:
                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::DECLARATION_ASSIGNMENT_FORCE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ASSIGN:
                                tokenTypeStack.push(Token::Type::ASSIGN);

                                ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::ASSIGNMENT:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::ASSIGN:
                            case Token::Type::MODULUS_EQUALS:
                            case Token::Type::BITWISE_AND_EQUALS:
                            case Token::Type::MULTIPLY_EQUALS:
                            case Token::Type::PLUS_EQUALS:
                            case Token::Type::MINUS_EQUALS:
                            case Token::Type::DIVIDE_EQUALS:
                            case Token::Type::BITSHIFT_LEFT_EQUALS:
                            case Token::Type::BITSHIFT_RIGHT_EQUALS:
                            case Token::Type::POW_EQUALS:
                            case Token::Type::BITWISE_OR_EQUALS:
                                tokenTypeStack.push(currentToken.type);

                                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::ACCESS:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PERIOD:
                                tokenTypeStack.push(Token::Type::PERIOD);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::PARAM_SEPERATOR:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::PARAM_SEPERATOR:
                                tokenTypeStack.push(Token::Type::PARAM_SEPERATOR);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::OPEN_PAREN:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LPAREN:
                                tokenTypeStack.push(Token::Type::LPAREN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::OPEN_BRACE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LBRACE:
                                tokenTypeStack.push(Token::Type::LBRACE);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::OPEN_BRACKET:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::LBRACKET:
                                tokenTypeStack.push(Token::Type::LBRACKET);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::CLOSE_PAREN:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::RPAREN:
                                tokenTypeStack.push(Token::Type::RPAREN);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::CLOSE_BRACE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::RBRACE:
                                tokenTypeStack.push(Token::Type::RBRACE);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::CLOSE_BRACKET:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::RBRACKET:
                                tokenTypeStack.push(Token::Type::RBRACKET);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    case ParseRuleSet::END_OF_LINE:
                    {
                        switch (currentToken.type)
                        {
                            case Token::Type::END_OF_LINE:
                                tokenTypeStack.push(Token::Type::END_OF_LINE);
                                break;

                            default:
                                localDidError = true;
                                break;
                        }
                        break;
                    }
                    
                    default:
                        localDidError = true;
                        break;
                }

                if (localDidError)
                    break;
            }

            if (localDidError || ruleSetStack.size() > 1 || tokenTypeStack.size() > 1)
            {
                errorCount += 1;
            }

            std::string_view compileUnitName = compileUnit.attributes.name;
            if (compileUnitName.length() == 0)
            {
                compileUnitName = compileUnit.name;
            }

            if (compileUnit.attributes.parseResult == !localDidError)
            {
                NC_LOG_MESSAGE("Compile Unit (%.*s) passed syntax check", compileUnitName.length(), compileUnitName.data());
            }
            else
            {
                NC_LOG_MESSAGE("Compile Unit (%.*s) failed syntax check result", compileUnitName.length(), compileUnitName.data());
            }
        });

    if (errorCount == 0)
    {
        NC_LOG_MESSAGE("Module (%s) passed syntax check", moduleInfo.name.c_str());
    }
    else
    {
        NC_LOG_MESSAGE("Module (%s) failed to pass syntax check", moduleInfo.name.c_str());
    }

    return errorCount == 0;
}

bool Parser::CreateAST(ModuleInfo& /*moduleInfo*/)
{
    return true;
}

bool Parser::CheckSemantics(ModuleInfo& /*moduleInfo*/)
{
    return true;
}