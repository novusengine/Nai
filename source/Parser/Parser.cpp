#include <pch/Build.h>
#include "Parser.h"

void Parser::Init()
{
    _rules =
    {
        { TokenType::NTS_START,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::KEYWORD, 2 }
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
                { TokenType::LITERAL, 2 }
            } 
        },
        { TokenType::NTS_EXPRESSION,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::LITERAL, 2 },
                { TokenType::OP_NOT, 3 },
                { TokenType::LPAREN, 4 }
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
        }
    };
}

void Parser::Process(ParserFile& parserFile)
{
    // File have no tokens to parse
    if (parserFile.lexerOutput.tokens.size() == 0)
        return;

    //printf("Syntax Analyzer Starting\n");

    if (!CheckSyntax(parserFile))
    {
        //printf("Syntax Analyzer Failed\n");
        return;
    }

    //printf("Syntax Analyzer Done\n\n");
    //BuildAST(parserFile);
}

bool Parser::CheckSyntax(ParserFile& file)
{
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
                        case 2:
                        {
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
                        // OP_NOT
                        case 3:
                        {
                            stack.pop();

                            stack.push(TokenType::NTS_EXPRESSION);
                            stack.push(TokenType::OP_NOT);
                            break;
                        }
                        // LPAREN
                        case 4:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_EXPRESSION_SEQUENCE);
                            stack.push(TokenType::RPAREN);
                            stack.push(TokenType::NTS_EXPRESSION);
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

                            stack.push(TokenType::NTS_EXPRESSION);
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
                case TokenType::NTS_RETURN_TYPE:
                {
                    switch (_rules[front][token.type])
                    {

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
}
