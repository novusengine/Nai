#include <pch/Build.h>
#include "Parser.h"

void Parser::Init()
{
    ZoneScoped;

    _rules =
    {
        { TokenType::NTS_START,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::KEYWORD, 2 },
            }
        },
        { TokenType::NTS_BODY,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::KEYWORD, 2 },
                { TokenType::RBRACE, 3 }
            }
        },
        { TokenType::NTS_DECLARATION,
            { 
                { TokenType::DECLARATION, 1 }
            } 
        },
        { TokenType::NTS_DECLARATION_TERMINATOR,
            { 
                { TokenType::OP_ASSIGN, 1 },
                { TokenType::END_OF_LINE, 2 }
            } 
        },
        { TokenType::NTS_VALUE,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::LITERAL, 2 },
                { TokenType::KEYWORD, 2 }
            } 
        },
        { TokenType::NTS_EXPRESSION,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::LITERAL, 2 },
                { TokenType::KEYWORD, 3 },
                { TokenType::OP_NOT, 4 },
                { TokenType::LPAREN, 5 }
            } 
        },
        { TokenType::NTS_EXPRESSION_RECURSIVE,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::LITERAL, 2 },
                { TokenType::KEYWORD, 3 },
                { TokenType::OP_NOT, 4 },
                { TokenType::LPAREN, 5 }
            } 
        },
        { TokenType::NTS_EXPRESSION_SEQUENCE,
            {
                { TokenType::OPERATOR, 1 },
                { TokenType::OP_MODULUS, 1 },
                { TokenType::OP_BITWISE_AND, 1 },
                { TokenType::OP_MULTIPLY, 1 },
                { TokenType::OP_ADD, 1 },
                { TokenType::OP_SUBTRACT, 1 },
                { TokenType::OP_DIVIDE, 1 },
                { TokenType::OP_LESS, 1 },
                { TokenType::OP_GREATER, 1 },
                { TokenType::OP_BITWISE_OR, 1 },
                { TokenType::LPAREN, 2 },
                { TokenType::RPAREN, 2 },
                { TokenType::END_OF_LINE, 2 }
            } 
        },
        { TokenType::NTS_ARGUMENT_LIST,
            {
                { TokenType::LPAREN, 1 }
            }
        },
        { TokenType::NTS_ARGUMENT_DECLARATION,
            {
                { TokenType::DECLARATION, 1 }
            }
        },
        { TokenType::NTS_ARGUMENT_DECLARATION_SEQUENCE,
            {
                { TokenType::OP_ASSIGN, 1 },
                { TokenType::PARAM_SEPERATOR, 2 },
                { TokenType::RPAREN, 3 }
            }
        },
        { TokenType::NTS_ARGUMENT_TERMINATOR,
            {
                { TokenType::PARAM_SEPERATOR, 1 },
                { TokenType::RPAREN, 2 }
            }
        },
        { TokenType::NTS_PARAMETER_LIST,
            {
                { TokenType::LPAREN, 1 }
            }
        },
        { TokenType::NTS_PARAMETER_TERMINATOR,
            {
                { TokenType::PARAM_SEPERATOR, 1 },
                { TokenType::RPAREN, 2 }
            }
        },
        { TokenType::NTS_STRUCT_MEMBER,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::RBRACE, 2 },
            }
        },
        { TokenType::NTS_ENUM_MEMBER,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::RBRACE, 2 },
            }
        },
        { TokenType::NTS_ENUM_TERMINATOR,
            {
                { TokenType::PARAM_SEPERATOR, 1 },
                { TokenType::RBRACE, 2 },
            }
        },
        { TokenType::NTS_IF_TERMINATOR,
            {
                { TokenType::KEYWORD, 1 },
                { TokenType::RBRACE, 2 },
                { TokenType::NTS_BODY, 2 },
            }
        },
        { TokenType::NTS_RETURN_TYPE,
            {
                { TokenType::OP_SUBTRACT, 1 },
                { TokenType::LBRACE, 2 },
            }
        }
    };
}

void Parser::Process(ParserFile& parserFile)
{
    ZoneScoped;

    // File have no tokens to parse
    if (parserFile.lexerOutput.tokens.size() == 0)
        return;

    //printf("Syntax Analyzer Starting\n");

    if (!CheckSyntax(parserFile))
    {
        printf("Syntax Analyzer Failed\n");
        return;
    }

    printf("Syntax Analyzer Done\n\n");
    //BuildAST(parserFile);
}

bool Parser::CheckSyntax(ParserFile& file)
{
    ZoneScoped;

    assert(file.stack.size() == 0);
    size_t index = 0;
    
    // Setup Stack
    std::stack<TokenType>& stack = file.stack;
    stack.push(TokenType::NTS_START);

    while (size_t stackSize = stack.size() > 0)
    {
        if (index == file.lexerOutput.tokens.size())
        {
            if (stackSize == 1 && stack.top() == TokenType::NTS_START)
                break;

            return false;
        }

        Token& token = file.lexerOutput.GetToken(index);
        Token nextToken;
        if (index + 1 < file.lexerOutput.tokens.size())
            nextToken = file.lexerOutput.GetToken(index + 1);

        TokenType front = stack.top();

        // Symbols Match
        if (token.type == front)
        {
            index++;
            stack.pop();
        }
        else
        {
            switch (front)
            {
                case TokenType::NTS_START:
                {
                    switch (_rules[front][token.type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            // Identifier
                            stack.pop();
                            stack.push(TokenType::NTS_START);
                            stack.push(TokenType::END_OF_LINE);

                            if (token.subType == TokenSubType::FUNCTION_CALL)
                            {
                                stack.push(TokenType::NTS_PARAMETER_LIST);
                            }
                            else
                            {
                                if (nextToken.type == TokenType::OP_ASSIGN)
                                {
                                    stack.push(TokenType::NTS_EXPRESSION);
                                    stack.push(TokenType::OP_ASSIGN);
                                }
                                else
                                {
                                    stack.push(TokenType::NTS_DECLARATION);
                                }
                            }

                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // KEYWORD
                        case 2:
                        {
                            // Keyword
                            stack.pop();
                            stack.push(TokenType::NTS_START);

                            if (token.subType == TokenSubType::KEYWORD_FUNCTION)
                            {
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::NTS_RETURN_TYPE);
                                stack.push(TokenType::NTS_ARGUMENT_LIST);
                                stack.push(TokenType::IDENTIFIER);
                            }
                            else if (token.subType == TokenSubType::KEYWORD_STRUCT)
                            {
                                stack.push(TokenType::END_OF_LINE);
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_STRUCT_MEMBER);
                                stack.push(TokenType::LBRACE);

                                stack.push(TokenType::STRUCT);
                            }
                            else if (token.subType == TokenSubType::KEYWORD_ENUM)
                            {
                                stack.push(TokenType::END_OF_LINE);
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_ENUM_MEMBER);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::ENUM);
                            }

                            stack.push(TokenType::KEYWORD);
                            break;
                        }
                        case 3:
                        {
                            break;
                        }
                        case 4:
                        {
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                    }

                    break;
                }
                case TokenType::NTS_BODY:
                {
                    switch (_rules[front][token.type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            // Identifier
                            stack.pop();
                            stack.push(TokenType::NTS_BODY);
                            stack.push(TokenType::END_OF_LINE);

                            if (token.subType == TokenSubType::FUNCTION_CALL)
                            {
                                stack.push(TokenType::NTS_PARAMETER_LIST);
                            }
                            else
                            {
                                if (nextToken.type == TokenType::OP_ASSIGN)
                                {
                                    stack.push(TokenType::NTS_EXPRESSION);
                                    stack.push(TokenType::OP_ASSIGN);
                                }
                                else
                                {
                                    stack.push(TokenType::NTS_DECLARATION);
                                }
                            }

                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // KEYWORD
                        case 2:
                        {
                            // Keyword
                            stack.pop();
                            stack.push(TokenType::NTS_BODY);

                            if (token.subType == TokenSubType::KEYWORD_STRUCT || token.subType == TokenSubType::KEYWORD_ENUM)
                                return false;

                            if (token.subType == TokenSubType::KEYWORD_WHILE)
                            {
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::RPAREN);
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(TokenType::LPAREN);
                            }
                            else if (token.subType == TokenSubType::KEYWORD_IF)
                            {
                                stack.push(TokenType::NTS_IF_TERMINATOR);
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::RPAREN);
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(TokenType::LPAREN);
                            }
                            else if (token.subType == TokenSubType::KEYWORD_FOR)
                            {
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::RPAREN);
                                stack.push(TokenType::NTS_EXPRESSION);
                                stack.push(TokenType::END_OF_LINE);
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(TokenType::END_OF_LINE);
                                stack.push(TokenType::NTS_DECLARATION);
                                stack.push(TokenType::IDENTIFIER);
                                stack.push(TokenType::LPAREN);
                            }
                            else if (token.subType == TokenSubType::KEYWORD_BREAK || token.subType == TokenSubType::KEYWORD_CONTINUE)
                            {
                                stack.push(TokenType::END_OF_LINE);
                            }
                            else if (token.subType == TokenSubType::KEYWORD_RETURN)
                            {
                                if (nextToken.type != TokenType::END_OF_LINE)
                                {
                                    stack.push(TokenType::END_OF_LINE);
                                    stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                }
                            }

                            stack.push(TokenType::KEYWORD);
                            break;
                        }
                        // RBRACE
                        case 3:
                        {
                            stack.pop();
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                    }

                    break;
                }
                case TokenType::NTS_DECLARATION:
                {
                    switch (_rules[front][token.type])
                    {
                        // OP_DECLARATION
                        case 1:
                        {
                            if (token.subType == TokenSubType::NONE || token.subType == TokenSubType::CONST_DECLARATION)
                            {
                                stack.pop();

                                if (token.subType == TokenSubType::CONST_DECLARATION)
                                {
                                    stack.push(TokenType::NTS_EXPRESSION);
                                    stack.push(TokenType::OP_ASSIGN);
                                }
                                else
                                {
                                    stack.push(TokenType::NTS_DECLARATION_TERMINATOR);
                                }

                                stack.push(TokenType::DATATYPE);
                                stack.push(TokenType::DECLARATION);
                                break;
                            }
                            else if (token.subType == TokenSubType::DECLARATION_ASSIGN || token.subType == TokenSubType::CONST_DECLARATION_ASSIGN)
                            {
                                stack.pop();
                                stack.push(TokenType::NTS_EXPRESSION);
                                stack.push(TokenType::DECLARATION);
                                break;
                            }

                            return false;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                    }
                    break;
                }
                case TokenType::NTS_DECLARATION_TERMINATOR:
                {
                    switch (_rules[front][token.type])
                    {
                        // OP_ASSIGN
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_EXPRESSION);
                            stack.push(TokenType::OP_ASSIGN);
                            break;
                        }
                        // END_OF_LINE
                        case 2:
                        {
                            // The END_OF_LINE symbol should already be on the stack if this is reached
                            stack.pop();
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_VALUE:
                {
                    switch (_rules[front][token.type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();

                            if (token.subType == TokenSubType::FUNCTION_CALL)
                            {
                                stack.push(TokenType::NTS_PARAMETER_LIST);
                            }

                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // LITERAL
                        case 2:
                        {
                            stack.pop();

                            // We need to check if this is an expression
                            if (nextToken.IsExpressionOperator())
                            {
                                // OP_ACCESS && OP_DECLARATION Fall in the value range checked above from .type, and they're not valid operators
                                // when forming an expression.
                                stack.push(TokenType::NTS_EXPRESSION);
                                stack.push(nextToken.type);
                            }

                            stack.push(TokenType::LITERAL);
                            break;
                        }
                        // KEYWORD
                        case 3:
                        {
                            stack.pop();

                            if (token.subType != TokenSubType::KEYWORD_TRUE && token.subType != TokenSubType::KEYWORD_FALSE)
                                return false;

                            // We need to check if this is an expression
                            if (nextToken.IsExpressionOperator())
                            {
                                // OP_ACCESS && OP_DECLARATION Fall in the value range checked above from .type, and they're not valid operators
                                // when forming an expression.
                                stack.push(TokenType::NTS_EXPRESSION);
                                stack.push(nextToken.type);
                            }

                            stack.push(TokenType::KEYWORD);
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_EXPRESSION:
                {
                    switch (_rules[front][token.type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();

                            if (token.subType == TokenSubType::FUNCTION_CALL)
                            {
                                stack.push(TokenType::NTS_EXPRESSION_SEQUENCE);
                                stack.push(TokenType::NTS_PARAMETER_LIST);
                            }

                            // We need to check if this is an assign or expression
                            if (nextToken.type == TokenType::OP_ASSIGN || nextToken.IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken.type);
                            }

                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // LITERAL
                        case 2:
                        {
                            stack.pop();

                            // We need to check if this is an assign or expression
                            if (nextToken.type == TokenType::OP_ASSIGN || nextToken.IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken.type);
                            }

                            stack.push(TokenType::LITERAL);
                            break;
                        }
                        // KEYWORD
                        case 3:
                        {
                            stack.pop();

                            if (token.subType != TokenSubType::KEYWORD_TRUE && token.subType != TokenSubType::KEYWORD_FALSE)
                                return false;

                            // We need to check if this is an assign or expression
                            if (nextToken.type == TokenType::OP_ASSIGN || nextToken.IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken.type);
                            }

                            stack.push(TokenType::KEYWORD);
                            break;
                        }
                        // OP_NOT
                        case 4:
                        {
                            stack.pop();

                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                            stack.push(TokenType::OP_NOT);
                            break;
                        }
                        // LPAREN
                        case 5:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_EXPRESSION_SEQUENCE);
                            stack.push(TokenType::RPAREN);
                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                            stack.push(TokenType::LPAREN);
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_EXPRESSION_RECURSIVE:
                {
                    switch (_rules[front][token.type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();

                            if (token.subType == TokenSubType::FUNCTION_CALL)
                            {
                                stack.push(TokenType::NTS_EXPRESSION_SEQUENCE);
                                stack.push(TokenType::NTS_PARAMETER_LIST);
                            }

                            // We need to check if this is an expression
                            if (nextToken.IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken.type);
                            }

                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // LITERAL
                        case 2:
                        {
                            stack.pop();

                            // We need to check if this is an expression
                            if (nextToken.IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken.type);
                            }

                            stack.push(TokenType::LITERAL);
                            break;
                        }
                        // KEYWORD
                        case 3:
                        {
                            stack.pop();

                            if (token.subType != TokenSubType::KEYWORD_TRUE && token.subType != TokenSubType::KEYWORD_FALSE)
                                return false;

                            // We need to check if this is an expression
                            if (nextToken.IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken.type);
                            }

                            stack.push(TokenType::KEYWORD);
                            break;
                        }
                        // OP_NOT
                        case 4:
                        {
                            stack.pop();

                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                            stack.push(TokenType::OP_NOT);
                            break;
                        }
                        // LPAREN
                        case 5:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_EXPRESSION_SEQUENCE);
                            stack.push(TokenType::RPAREN);
                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                            stack.push(TokenType::LPAREN);
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_EXPRESSION_SEQUENCE:
                {
                    switch (_rules[front][token.type])
                    {
                        // Expressional Operators
                        case 1:
                        {
                            stack.pop();

                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                            stack.push(token.type);
                            break;
                        }
                        // LPAREN / RPAREN / END OF LINE
                        case 2:
                        {
                            stack.pop();
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_PARAMETER_LIST:
                {
                    switch (_rules[front][token.type])
                    {
                        // LPAREN
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::RPAREN);

                            // We need to check if this is an expression
                            if (nextToken.type != TokenType::RPAREN)
                            {
                                stack.push(TokenType::NTS_PARAMETER_TERMINATOR);
                                stack.push(TokenType::NTS_EXPRESSION);
                            }
                            stack.push(TokenType::LPAREN);

                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_PARAMETER_TERMINATOR:
                {
                    switch (_rules[front][token.type])
                    {
                        // PARAM_SEPERATOR
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_PARAMETER_TERMINATOR);
                            stack.push(TokenType::NTS_EXPRESSION);
                            stack.push(TokenType::PARAM_SEPERATOR);

                            break;
                        }
                        // RPAREN
                        case 2:
                        {
                            stack.pop();
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_ARGUMENT_LIST:
                {
                    switch (_rules[front][token.type])
                    {
                        // LPAREN
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::RPAREN);

                            // We need to check if there are arguments to be parsed
                            if (nextToken.type != TokenType::RPAREN)
                            {
                                stack.push(TokenType::NTS_ARGUMENT_DECLARATION);
                                stack.push(TokenType::IDENTIFIER);
                            }

                            stack.push(TokenType::LPAREN);
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_ARGUMENT_DECLARATION:
                {
                    switch (_rules[front][token.type])
                    {
                        // OP_DECLARATION
                        case 1:
                        {
                            if (token.subType == TokenSubType::NONE || token.subType == TokenSubType::CONST_DECLARATION)
                            {
                                stack.pop();

                                if (token.subType == TokenSubType::CONST_DECLARATION)
                                {
                                    stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                    stack.push(TokenType::OP_ASSIGN);
                                }
                                else
                                {
                                    stack.push(TokenType::NTS_ARGUMENT_DECLARATION_SEQUENCE);
                                }

                                stack.push(TokenType::DATATYPE);
                                stack.push(TokenType::DECLARATION);
                                break;
                            }
                            else if (token.subType == TokenSubType::DECLARATION_ASSIGN || token.subType == TokenSubType::CONST_DECLARATION_ASSIGN)
                            {
                                stack.pop();
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(TokenType::DECLARATION);
                                break;
                            }

                            return false;
                        }

                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                    }
                    break;
                }
                case TokenType::NTS_ARGUMENT_DECLARATION_SEQUENCE:
                {
                    switch (_rules[front][token.type])
                    {
                        // OP_ASSIGN
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_ARGUMENT_TERMINATOR);
                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                            stack.push(TokenType::OP_ASSIGN);
                            break;
                        }
                        // PARAM_SEPERATOR
                        case 2:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_ARGUMENT_DECLARATION);
                            stack.push(TokenType::IDENTIFIER);

                            stack.push(TokenType::PARAM_SEPERATOR);

                            break;
                        }
                        // RPAREN
                        case 3:
                        {
                            stack.pop();
                            break;
                        }

                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                    }
                    break;
                }
                case TokenType::NTS_ARGUMENT_TERMINATOR:
                {
                    switch (_rules[front][token.type])
                    {
                        // PARAM_SEPERATOR
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_ARGUMENT_DECLARATION);
                            stack.push(TokenType::IDENTIFIER);

                            stack.push(TokenType::PARAM_SEPERATOR);

                            break;
                        }
                        // RPAREN
                        case 2:
                        {
                            stack.pop();
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_STRUCT_MEMBER:
                {
                    switch (_rules[front][token.type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_STRUCT_MEMBER);

                            // We expect an identifier
                            if (token.subType != TokenSubType::NONE)
                                return false;

                            // We expect the next token to be a declarator
                            if (nextToken.type != TokenType::DECLARATION)
                                return false;

                            stack.push(TokenType::END_OF_LINE);
                            stack.push(TokenType::NTS_DECLARATION);
                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // RBRACE
                        case 2:
                        {
                            stack.pop();
                            break;
                        }

                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                    }
                    break;
                }
                case TokenType::NTS_ENUM_MEMBER:
                {
                    switch (_rules[front][token.type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_ENUM_MEMBER);

                            // We expect an identifier
                            if (token.subType != TokenSubType::NONE)
                                return false;

                            // We expect the next token to be a declarator
                            if (nextToken.type != TokenType::OP_ASSIGN)
                                return false;

                            stack.push(TokenType::NTS_ENUM_TERMINATOR);
                            stack.push(TokenType::NTS_EXPRESSION);
                            stack.push(TokenType::OP_ASSIGN);
                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // RBRACE
                        case 2:
                        {
                            stack.pop();
                            break;
                        }

                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                    }
                    break;
                }
                case TokenType::NTS_ENUM_TERMINATOR:
                {
                    switch (_rules[front][token.type])
                    {
                        // PARAM_SEPERATOR
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_ENUM_MEMBER);

                            stack.push(TokenType::PARAM_SEPERATOR);
                            break;
                        }
                        // RPAREN
                        case 2:
                        {
                            stack.pop();
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_IF_TERMINATOR:
                {
                    switch (_rules[front][token.type])
                    {
                        // KEYWORD
                        case 1:
                        {
                            stack.pop();

                            if (token.subType != TokenSubType::KEYWORD_ELSEIF && token.subType != TokenSubType::KEYWORD_ELSE)
                                return false;

                            if (token.subType == TokenSubType::KEYWORD_ELSEIF)
                            {
                                stack.push(TokenType::NTS_IF_TERMINATOR);

                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);

                                stack.push(TokenType::RPAREN);
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(TokenType::LPAREN);
                            }
                            else
                            {
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                            }

                            stack.push(TokenType::KEYWORD);
                            break;
                        }
                        // NTS_BODY / RBRACE
                        case 2:
                        {
                            stack.pop();
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                        }
                    break;
                }
                case TokenType::NTS_RETURN_TYPE:
                {
                    switch (_rules[front][token.type])
                    {
                        // OP_SUBTRACT
                        case 1:
                        {
                            stack.pop();

                            if (token.subType != TokenSubType::OP_RETURN_TYPE)
                                return false;

                            stack.push(TokenType::DATATYPE);
                            stack.push(TokenType::OP_SUBTRACT);
                            break;
                        }
                        // LBRACE
                        case 2:
                        {
                            stack.pop();
                            break;
                        }
                        // We defaulted, this means the syntax is bad
                        default:
                            return false;
                    }
                    break;
                }

                // We defaulted, this means the syntax is bad
                default:
                    return false;
            }
        }
    }

    return true;
}

void Parser::BuildAST(ParserFile&)
{
    ZoneScoped;
}
