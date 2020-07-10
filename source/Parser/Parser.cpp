#include <pch/Build.h>
#include "Parser.h"
#include "AST.h"


void Parser::Init()
{
    ZoneScoped;

    _rules =
    {
        { TokenType::NTS_START,
            {
                { TokenType::KEYWORD, 1 }
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
                { TokenType::PARAM_SEPERATOR, 2 },
                { TokenType::RPAREN, 3 }
            }
        },
        { TokenType::NTS_STRUCT_MEMBER,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::KEYWORD, 2 },
                { TokenType::RBRACE, 3 }
            }
        },
        { TokenType::NTS_ENUM_MEMBER,
            {
                { TokenType::IDENTIFIER, 1 },
                { TokenType::RBRACE, 2 }
            }
        },
        { TokenType::NTS_ENUM_TERMINATOR,
            {
                { TokenType::PARAM_SEPERATOR, 1 },
                { TokenType::RBRACE, 2 }
            }
        },
        { TokenType::NTS_IF_TERMINATOR,
            {
                { TokenType::KEYWORD, 1 },
                { TokenType::RBRACE, 2 },
                { TokenType::NTS_BODY, 2 }
            }
        },
        { TokenType::NTS_RETURN_TYPE,
            {
                { TokenType::OP_SUBTRACT, 1 },
                { TokenType::LBRACE, 2 }
            }
        }
    };

    // Setup Default Return Type Token
    defaultReturnTypeToken->type = TokenType::DATATYPE;
    defaultReturnTypeToken->value = defaultReturnTypeName.data();
    defaultReturnTypeToken->valueSize = 4;

    // Setup Infer Type Token
    inferTypeToken->type = TokenType::DATATYPE;
    inferTypeToken->value = inferTypeName.data();
    inferTypeToken->valueSize = 4;
}

void Parser::Process(ModuleParser& parser)
{
    ZoneScoped;

    // Module have no tokens to parse / Module has already been parsed
    if (parser.GetTokens().size() == 0 || parser.GetStack().size() > 0)
        return;

    printf("Syntax Analyzer Started\n");

    if (!CheckSyntax(parser))
    {
        printf("Syntax Analyzer Failed\n");
        return;
    }
    else
    {
        printf("Syntax Analyzer Done\n\n");
    }


    printf("Parser Started\n");
    BuildAST(parser);
    printf("Parser Done\n");
}

bool Parser::CheckSyntax(ModuleParser& parser)
{
    ZoneScoped;

    // Get Tokens & Setup Stack
    const std::vector<Token>& tokens = parser.GetTokens();
    size_t tokensNum = tokens.size();

    std::stack<TokenType>& stack = parser.GetStack();
    stack.push(TokenType::NTS_START);

    const size_t& index = parser.GetIndex();
    while (size_t stackSize = stack.size() > 0)
    {
        if (index == tokensNum)
        {
            if (stackSize == 1 && stack.top() == TokenType::NTS_START)
                break;

            return false;
        }

        const Token* token = parser.GetToken();
        const Token* nextToken = parser.GetToken(1);

        TokenType front = stack.top();

        // Symbols Match
        if (token->type == front)
        {
            parser.IncrementIndex();
            stack.pop();
        }
        else
        {
            switch (front)
            {
                case TokenType::NTS_START:
                {
                    switch (_rules[front][token->type])
                    {
                        // KEYWORD
                        case 1:
                        {
                            // Keyword
                            stack.pop();
                            stack.push(TokenType::NTS_START);

                            if (token->subType == TokenSubType::KEYWORD_FUNCTION)
                            {
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::NTS_RETURN_TYPE);
                                stack.push(TokenType::NTS_ARGUMENT_LIST);
                                stack.push(TokenType::IDENTIFIER);
                            }
                            else if (token->subType == TokenSubType::KEYWORD_STRUCT)
                            {
                                stack.push(TokenType::END_OF_LINE);
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_STRUCT_MEMBER);
                                stack.push(TokenType::LBRACE);

                                stack.push(TokenType::STRUCT);
                            }
                            else if (token->subType == TokenSubType::KEYWORD_ENUM)
                            {
                                stack.push(TokenType::END_OF_LINE);
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_ENUM_MEMBER);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::ENUM);
                            }
                            else
                            {
                                return false;
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
                case TokenType::NTS_BODY:
                {
                    switch (_rules[front][token->type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            // Identifier
                            stack.pop();
                            stack.push(TokenType::NTS_BODY);
                            stack.push(TokenType::END_OF_LINE);

                            if (token->subType == TokenSubType::FUNCTION_CALL)
                            {
                                stack.push(TokenType::NTS_PARAMETER_LIST);
                            }
                            else
                            {
                                if (nextToken->type == TokenType::OP_ASSIGN)
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

                            if (token->subType == TokenSubType::KEYWORD_STRUCT || token->subType == TokenSubType::KEYWORD_ENUM)
                                return false;

                            if (token->subType == TokenSubType::KEYWORD_WHILE)
                            {
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::RPAREN);
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(TokenType::LPAREN);
                            }
                            else if (token->subType == TokenSubType::KEYWORD_IF)
                            {
                                stack.push(TokenType::NTS_IF_TERMINATOR);
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::RPAREN);
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(TokenType::LPAREN);
                            }
                            else if (token->subType == TokenSubType::KEYWORD_FOR)
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
                            else if (token->subType == TokenSubType::KEYWORD_BREAK || token->subType == TokenSubType::KEYWORD_CONTINUE)
                            {
                                stack.push(TokenType::END_OF_LINE);
                            }
                            else if (token->subType == TokenSubType::KEYWORD_RETURN)
                            {
                                if (nextToken->type != TokenType::END_OF_LINE)
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
                    switch (_rules[front][token->type])
                    {
                        // OP_DECLARATION
                        case 1:
                        {
                            if (token->subType == TokenSubType::NONE || token->subType == TokenSubType::CONST_DECLARATION)
                            {
                                stack.pop();

                                if (token->subType == TokenSubType::CONST_DECLARATION)
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
                            else if (token->subType == TokenSubType::DECLARATION_ASSIGN || token->subType == TokenSubType::CONST_DECLARATION_ASSIGN)
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
                    switch (_rules[front][token->type])
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
                    switch (_rules[front][token->type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();

                            if (token->subType == TokenSubType::FUNCTION_CALL)
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
                            if (nextToken->IsExpressionOperator())
                            {
                                // OP_ACCESS && OP_DECLARATION Fall in the value range checked above from .type, and they're not valid operators
                                // when forming an expression.
                                stack.push(TokenType::NTS_EXPRESSION);
                                stack.push(nextToken->type);
                            }

                            stack.push(TokenType::LITERAL);
                            break;
                        }
                        // KEYWORD
                        case 3:
                        {
                            stack.pop();

                            if (token->subType != TokenSubType::KEYWORD_TRUE && token->subType != TokenSubType::KEYWORD_FALSE)
                                return false;

                            // We need to check if this is an expression
                            if (nextToken->IsExpressionOperator())
                            {
                                // OP_ACCESS && OP_DECLARATION Fall in the value range checked above from .type, and they're not valid operators
                                // when forming an expression.
                                stack.push(TokenType::NTS_EXPRESSION);
                                stack.push(nextToken->type);
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
                    switch (_rules[front][token->type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();

                            if (token->subType == TokenSubType::FUNCTION_CALL)
                            {
                                stack.push(TokenType::NTS_PARAMETER_LIST);
                            }

                            // We need to check if this is an assign or expression
                            if (nextToken->type == TokenType::OP_ASSIGN || nextToken->IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken->type);
                            }

                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // LITERAL
                        case 2:
                        {
                            stack.pop();

                            // We need to check if this is an assign or expression
                            if (nextToken->type == TokenType::OP_ASSIGN || nextToken->IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken->type);
                            }

                            stack.push(TokenType::LITERAL);
                            break;
                        }
                        // KEYWORD
                        case 3:
                        {
                            stack.pop();

                            if (token->subType != TokenSubType::KEYWORD_TRUE && token->subType != TokenSubType::KEYWORD_FALSE)
                                return false;

                            // We need to check if this is an assign or expression
                            if (nextToken->type == TokenType::OP_ASSIGN || nextToken->IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken->type);
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
                    switch (_rules[front][token->type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();

                            if (token->subType == TokenSubType::FUNCTION_CALL)
                            {
                                stack.push(TokenType::NTS_PARAMETER_LIST);
                            }

                            // We need to check if this is an expression
                            if (nextToken->IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken->type);
                            }

                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // LITERAL
                        case 2:
                        {
                            stack.pop();

                            // We need to check if this is an expression
                            if (nextToken->IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken->type);
                            }

                            stack.push(TokenType::LITERAL);
                            break;
                        }
                        // KEYWORD
                        case 3:
                        {
                            stack.pop();

                            if (token->subType != TokenSubType::KEYWORD_TRUE && token->subType != TokenSubType::KEYWORD_FALSE)
                                return false;

                            // We need to check if this is an expression
                            if (nextToken->IsExpressionOperator())
                            {
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                                stack.push(nextToken->type);
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
                    switch (_rules[front][token->type])
                    {
                        // Expressional Operators
                        case 1:
                        {
                            stack.pop();

                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                            stack.push(token->type);
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
                    switch (_rules[front][token->type])
                    {
                        // LPAREN
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::RPAREN);

                            // We need to check if this is an expression
                            if (nextToken->type != TokenType::RPAREN)
                            {
                                stack.push(TokenType::NTS_PARAMETER_TERMINATOR);
                                stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
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
                    switch (_rules[front][token->type])
                    {
                        // OPERATOR
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_PARAMETER_TERMINATOR);
                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
                            stack.push(token->type);
                            break;
                        }
                        // PARAM_SEPERATOR
                        case 2:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_PARAMETER_TERMINATOR);
                            stack.push(TokenType::NTS_EXPRESSION_RECURSIVE);
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
                case TokenType::NTS_ARGUMENT_LIST:
                {
                    switch (_rules[front][token->type])
                    {
                        // LPAREN
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::RPAREN);

                            // We need to check if there are arguments to be parsed
                            if (nextToken->type != TokenType::RPAREN)
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
                    switch (_rules[front][token->type])
                    {
                        // OP_DECLARATION
                        case 1:
                        {
                            if (token->subType == TokenSubType::NONE || token->subType == TokenSubType::CONST_DECLARATION)
                            {
                                stack.pop();

                                if (token->subType == TokenSubType::CONST_DECLARATION)
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
                            else if (token->subType == TokenSubType::DECLARATION_ASSIGN || token->subType == TokenSubType::CONST_DECLARATION_ASSIGN)
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
                    switch (_rules[front][token->type])
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
                    switch (_rules[front][token->type])
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
                    switch (_rules[front][token->type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_STRUCT_MEMBER);

                            // We expect an identifier
                            if (token->subType != TokenSubType::NONE)
                                return false;

                            // We expect the next token to be a declarator
                            if (nextToken->type != TokenType::DECLARATION)
                                return false;

                            stack.push(TokenType::END_OF_LINE);
                            stack.push(TokenType::NTS_DECLARATION);
                            stack.push(TokenType::IDENTIFIER);
                            break;
                        }
                        // KEYWORD
                        case 2:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_STRUCT_MEMBER);

                            if (token->subType == TokenSubType::KEYWORD_FUNCTION)
                            {
                                stack.push(TokenType::RBRACE);
                                stack.push(TokenType::NTS_BODY);
                                stack.push(TokenType::LBRACE);
                                stack.push(TokenType::NTS_RETURN_TYPE);
                                stack.push(TokenType::NTS_ARGUMENT_LIST);
                                stack.push(TokenType::IDENTIFIER);
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
                case TokenType::NTS_ENUM_MEMBER:
                {
                    switch (_rules[front][token->type])
                    {
                        // IDENTIFIER
                        case 1:
                        {
                            stack.pop();
                            stack.push(TokenType::NTS_ENUM_MEMBER);

                            // We expect an identifier
                            if (token->subType != TokenSubType::NONE)
                                return false;

                            // We expect the next token to be a declarator
                            if (nextToken->type != TokenType::OP_ASSIGN)
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
                    switch (_rules[front][token->type])
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
                    switch (_rules[front][token->type])
                    {
                        // KEYWORD
                        case 1:
                        {
                            stack.pop();

                            if (token->subType != TokenSubType::KEYWORD_ELSEIF && token->subType != TokenSubType::KEYWORD_ELSE)
                                return false;

                            if (token->subType == TokenSubType::KEYWORD_ELSEIF)
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
                    switch (_rules[front][token->type])
                    {
                        // OP_SUBTRACT
                        case 1:
                        {
                            stack.pop();

                            if (token->subType != TokenSubType::OP_RETURN_TYPE)
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

    parser.ResetIndex();
    return true;
}

void Parser::BuildAST(ModuleParser& parser)
{
    ZoneScoped;

    size_t tokensNum = parser.GetTokens().size();

    size_t& index = parser.GetIndex();
    while (index < tokensNum)
    {
        const Token* token = parser.GetTokenIncrement();
        if (token->type == TokenType::KEYWORD)
        {
            if (token->subType == TokenSubType::KEYWORD_FUNCTION)
            {
                ASTFunctionDecl* functionNode = ParseFunction(parser);
                if (!functionNode)
                    return;

                parser.moduleFunctionNodes.push_back(functionNode);
            }
            else if (token->subType == TokenSubType::KEYWORD_STRUCT)
            {
                ASTStruct* structNode = ParseStruct(parser);
                if (!structNode)
                    return;

                parser.moduleStructNodes.push_back(structNode);
            }
            else if (token->subType == TokenSubType::KEYWORD_ENUM)
            {
                ASTEnum* enumNode = ParseEnum(parser);
                if (!enumNode)
                    return;

                parser.modueEnumNodes.push_back(enumNode);
            }
        }
    }

    VisitAST(parser.modueEnumNodes);
    VisitAST(parser.moduleStructNodes);
    VisitAST(parser.moduleFunctionNodes);

    parser.ResetIndex();
}

ASTFunctionDecl* Parser::ParseFunction(ModuleParser& parser)
{
    const Token* fnIdentifierToken = parser.GetTokenIncrement();
    ASTFunctionDecl* fnNode = new ASTFunctionDecl(fnIdentifierToken);

    ASTSequence* currentSequence = fnNode->top;

    // Handle Function Header
    parser.IncrementIndex(); // Skip LParen Index
    {
        while (const Token* startToken = parser.GetToken())
        {
            if (startToken->type == TokenType::RPAREN)
                break;

            ASTFunctionParam* fnParam = new ASTFunctionParam(startToken);
            fnNode->parameters.push_back(fnParam);

            // Skip Identifier & Declaration Token
            parser.IncrementIndex(2);

            const Token* dataTypeToken = parser.GetTokenIncrement();
            fnParam->dataType = new ASTDataType(dataTypeToken);

            const Token* endToken = parser.GetToken();
            if (endToken->type == TokenType::OP_ASSIGN)
            {
                // Skip OP_ASSIGN Index
                parser.IncrementIndex();
                fnParam->defaultValue = ParseExpression(parser);
            }
            else if (endToken->type == TokenType::PARAM_SEPERATOR)
            {
                // SKIP PARAM_SEPERATOR Index
                parser.IncrementIndex();
            }
        }
    }
    parser.IncrementIndex(); // Skip RParen Index

    // Check for return type
    const Token* returnOperatorToken = parser.GetToken();
    if (returnOperatorToken->type == TokenType::OP_SUBTRACT)
    {
        // Skip Return Type Operator Index
        parser.IncrementIndex();

        // Skip Return Type Index
        const Token* returnTypeToken = parser.GetTokenIncrement();
        fnNode->returnType = new ASTDataType(returnTypeToken);
    }
    else
    {
        // Default return type to void (We might be able to infer the type from a "return" if its seen later)
        fnNode->returnType = new ASTDataType(defaultReturnTypeToken);
    }

    // Handle Function Body
    parser.IncrementIndex(); // Skip LBRACE Index
    {
        while (true)
        {
            const Token* token = parser.GetToken();
            if (token->type == TokenType::RBRACE)
                break;

            if (token->type == TokenType::IDENTIFIER)
            {
                if (token->subType == TokenSubType::FUNCTION_CALL)
                {
                    currentSequence->left = ParseFunctionCall(parser);
                }
                else
                {
                    ASTVariable* variable = ParseVariable(parser);
                    if (!variable)
                        return nullptr;

                    // If DataType is a valid pointer, we know this was a declaration, this means we need to check if
                    // A variable in this function by that same name exists, if so, we have a conflict 
                    // (Scopes can change this but we currently don't have any support for scope)

                    ASTVariable* varPtr = nullptr;
                    for (ASTVariable* var : fnNode->variables)
                    {
                        if (variable->GetNameHash() == var->GetNameHash())
                        {
                            varPtr = var;
                            break;
                        }
                    }

                    if (variable->dataType)
                    {
                        if (varPtr)
                        {
                            ReportError(1, "Redeclaration of variable (Name: %.*s, Line: %i, Col: %i) at (Line: %u, Col: %u)\n", varPtr->GetNameSize(), varPtr->GetName().data(), varPtr->token->lineNum, varPtr->token->colNum, variable->token->lineNum, variable->token->colNum);
                            return nullptr;
                        }

                        fnNode->variables.push_back(variable);
                    }
                    else
                    {
                        if (!varPtr)
                        {
                            ReportError(2, "Use of undeclared variable (Name: %.*s, At Line: %i, At Col: %i)\n", variable->GetNameSize(), variable->GetName().data(), variable->token->lineNum, variable->token->colNum);
                            return nullptr;
                        }

                        variable->parent = varPtr;
                        variable->dataType = variable->parent->dataType;
                    }

                    currentSequence->left = variable;
                }
            }
            else if (token->type == TokenType::KEYWORD)
            {
                if (token->subType == TokenSubType::KEYWORD_WHILE)
                {
                    currentSequence->left = ParseKeywordWhile(parser);
                }
                else if (token->subType == TokenSubType::KEYWORD_IF)
                {
                    currentSequence->left = ParseKeywordIf(parser);
                }
                else if (token->subType == TokenSubType::KEYWORD_FOR)
                {
                    currentSequence->left = ParseKeywordFor(parser);
                }
                else if (token->subType == TokenSubType::KEYWORD_RETURN)
                {
                    currentSequence->left = ParseKeywordReturn(parser);
                }
            }

            currentSequence->right = new ASTSequence();
            currentSequence = currentSequence->right;
        }
    }
    parser.IncrementIndex(); // Skip RBrace Index

    return fnNode;
}
ASTFunctionCall* Parser::ParseFunctionCall(ModuleParser& parser)
{
    const Token* identifierToken = parser.GetTokenIncrement();
    ASTFunctionCall* functionCall = new ASTFunctionCall(identifierToken);

    // Handle Function Call
    parser.IncrementIndex(); // Skip LParen Index
    {    
        while (true)
        {
            // This will skip RParen Index
            const Token* token = parser.GetToken();
            if (token->type == TokenType::RPAREN)
            {
                break;
            }
            else if (token->type == TokenType::PARAM_SEPERATOR)
            {
                // Skip Param Seperator
                parser.IncrementIndex();
                continue;
            }

            ASTNode* node = nullptr;
            if (token->type == TokenType::IDENTIFIER && token->subType == TokenSubType::FUNCTION_CALL)
            {
                ASTFunctionCall* fn = ParseFunctionCall(parser);

                const Token* nextToken = parser.GetToken();
                if (nextToken->IsExpressionOperator())
                {
                    // Skip Operator Index
                    parser.IncrementIndex();

                    ASTOperator* op = new ASTOperator(nextToken);
                    op->left = fn;
                    op->right = ParseExpression(parser);

                    node = op;
                }
                else
                {
                    node = fn;
                }
            }
            else
            {
                node = ParseExpression(parser);
            }

            functionCall->parameters.push_back(node);
        }
    }

    parser.IncrementIndex(2); // Skip RParen & END_OF_LINE Index
    return functionCall;
}

ASTStruct* Parser::ParseStruct(ModuleParser& parser)
{
    const Token* structToken = parser.GetTokenIncrement();
    ASTStruct* structNode = new ASTStruct(structToken);

    parser.IncrementIndex(); // Skip LBRACE Index
    {
        while (const Token* startToken = parser.GetToken())
        {
            if (startToken->type == TokenType::RBRACE)
                break;

            if (startToken->type == TokenType::IDENTIFIER)
            {
                ASTVariable* variable = ParseVariable(parser);
                structNode->variables.push_back(variable);
            }
            else if (startToken->type == TokenType::KEYWORD && startToken->subType == TokenSubType::KEYWORD_FUNCTION)
            {
                // Skip KEYWORD_FUNCTION Index
                parser.IncrementIndex();

                ASTFunctionDecl* fnDecl = ParseFunction(parser);
                structNode->functions.push_back(fnDecl);
            }
        }
    }
    parser.IncrementIndex(); // Skip RBRACE Index

    return structNode;
}
ASTEnum* Parser::ParseEnum(ModuleParser& /*parser*/)
{
    return nullptr;
}

ASTVariable* Parser::ParseVariable(ModuleParser& parser)
{
    const Token* identifierToken = parser.GetTokenIncrement();
    ASTVariable* variable = new ASTVariable(identifierToken);

    const Token* nextToken = parser.GetTokenIncrement();
    if (nextToken->type == TokenType::DECLARATION)
    {
        variable->isConst = nextToken->subType == TokenSubType::CONST_DECLARATION || nextToken->subType == TokenSubType::CONST_DECLARATION_ASSIGN;

        if (nextToken->subType == TokenSubType::NONE || nextToken->subType == TokenSubType::CONST_DECLARATION)
        {
            const Token* dataTypeToken = parser.GetTokenIncrement();
            variable->dataType = new ASTDataType(dataTypeToken);

            const Token* assignToken = parser.GetTokenIncrement();
            if (assignToken->type == TokenType::OP_ASSIGN)
            {
                variable->value = ParseExpression(parser);
            }
        }
        else if (nextToken->subType == TokenSubType::DECLARATION_ASSIGN || nextToken->subType == TokenSubType::CONST_DECLARATION_ASSIGN)
        {
            variable->dataType = new ASTDataType(inferTypeToken);
            variable->value = ParseExpression(parser);
        }
    }
    else if (nextToken->type == TokenType::OP_ASSIGN)
    {
        if (variable->isConst)
        {
            ReportError(3, "Const Variables cannot be assigned to after being declared and must be set when declared (Name: %.*s, At Line: %i, At Col: %i)\n", variable->GetNameSize(), variable->GetName().data(), variable->token->lineNum, variable->token->colNum);
            return nullptr;
        }
        variable->value = ParseExpression(parser);
    }

    // Skip END_OF_LINE Index
    parser.IncrementIndex();

    return variable;
}

ASTNode* Parser::ParseExpression(ModuleParser& parser)
{
    // Parse Expression
    const Token* token = parser.GetToken();
    ASTNode* node = nullptr;

    if (token->type == TokenType::IDENTIFIER && token->subType == TokenSubType::FUNCTION_CALL)
    {
        node = ParseFunctionCall(parser);
    }
    else
    {
        node = new ASTExpression(token);

        // Skip Token Index
        parser.IncrementIndex();
    }
    
    const Token* nextToken = parser.GetToken();
    if (nextToken->IsExpressionOperator())
    {
        // Skip Operator Index
        parser.IncrementIndex();

        ASTOperator* op = new ASTOperator(nextToken);
        op->left = node;
        op->right = ParseExpression(parser);

        return op;
    }

    return node;
}

ASTNode* Parser::ParseKeywordWhile(ModuleParser& /*parser*/)
{
    return nullptr;
}
ASTNode* Parser::ParseKeywordIf(ModuleParser& /*parser*/)
{
    return nullptr;
}
ASTNode* Parser::ParseKeywordFor(ModuleParser& /*parser*/)
{
    return nullptr;
}
ASTNode* Parser::ParseKeywordReturn(ModuleParser& parser)
{
    const Token* token = parser.GetToken();
    ASTFunctionReturn* fnReturn = new ASTFunctionReturn(token);

    // Skip Return Token Index
    parser.IncrementIndex();

    const Token* nextToken = parser.GetToken();
    if (nextToken->type != TokenType::END_OF_LINE)
    {
        fnReturn->top = ParseExpression(parser);

        // Skip End of Line Index
        parser.IncrementIndex();
    }

    return fnReturn;
}

void Parser::VisitAST(const std::vector<ASTNode*>& ast)
{
    for (const ASTNode* node : ast)
    {
        VisitNode(node);
        printf("\n");
    }
}
void Parser::VisitNode(const ASTNode* node)
{
    if (!node)
        return;

    // SEQUENCES don't hold tokens, so we deal with them first
    if (node->type == ASTNodeType::SEQUENCE)
    {
        const ASTSequence* sequence = reinterpret_cast<const ASTSequence*>(node);
        printf("Sequence\n");

        VisitNode(sequence->left);
        VisitNode(sequence->right);
        return;
    }

    const std::string_view& nodeName = node->GetName();
    const int nodeNameSize = node->GetNameSize();
    
    if (node->type == ASTNodeType::FUNCTION_DECL)
    {
        const ASTFunctionDecl* fn = reinterpret_cast<const ASTFunctionDecl*>(node);

        const std::string_view& returnTypeName = fn->returnType->GetName();
        const int returnTypeNameSize = fn->returnType->GetNameSize();

        printf("Function (Name: %.*s), (Param Count: %zu), (Return Type: %.*s)\n", nodeNameSize, nodeName.data(), fn->parameters.size(), returnTypeNameSize, returnTypeName.data());
        
        size_t paramCount= 1;
        for (const ASTFunctionParam* param : fn->parameters)
        {
            const std::string_view& paramName = param->GetName();
            const int paramNameSize = param->GetNameSize();

            const std::string_view& dataTypeName = param->dataType->GetName();
            const int dataTypeNameSize = param->dataType->GetNameSize();

            printf("Function Param %zu: (Name: %.*s), (DataType: %.*s), (Default Value: %s)\n", paramCount++, paramNameSize, paramName.data(), dataTypeNameSize, dataTypeName.data(), param->defaultValue ? "Yes" : "No");
        }

        VisitNode(fn->top);

        printf("\n");
    }
    else if (node->type == ASTNodeType::FUNCTION_CALL)
    {
        const ASTFunctionCall* fn = reinterpret_cast<const ASTFunctionCall*>(node);

        const std::string_view& fnName = fn->GetName();
        const int fnNameSize = fn->GetNameSize();

        printf("Function Call (Name: %.*s), (Param Count: %zu)\n", fnNameSize, fnName.data(), fn->parameters.size());

        size_t paramCount = 1;
        for (const ASTNode* param : fn->parameters)
        {
            const std::string_view& paramName = param->GetName();
            const int paramNameSize = param->GetNameSize();

            if (param->type == ASTNodeType::EXPRESSION)
            {
                const std::string& typeName = Token::TypeToString(param->token->type);

                printf("Function Param %zu: (Value: %.*s), (Type: %s)\n", paramCount++, paramNameSize, paramName.data(), typeName.c_str());
            }
            else if (param->type == ASTNodeType::FUNCTION_CALL)
            {
                printf("Function Param %zu (Name: %.*s) BEGIN {\n", paramCount, paramNameSize, paramName.data());
                VisitNode(param);
                printf("Function Param %zu (Name: %.*s) END }\n", paramCount++, paramNameSize, paramName.data());
            }
            else if (param->type == ASTNodeType::OPERATOR)
            {
                const ASTOperator* paramNode = reinterpret_cast<const ASTOperator*>(param);
                const std::string& typeName = Token::TypeToString(param->token->type);

                printf("Function Param %zu (Name: %s) BEGIN {\n", paramCount, typeName.c_str());
                VisitNode(paramNode->left);
                VisitNode(paramNode->right);
                printf("Function Param %zu (Name: %s) END }\n", paramCount++, typeName.c_str());
            }
        }
    }
    else if (node->type == ASTNodeType::FUNCTION_RETURN)
    {
        const ASTFunctionReturn* fnReturn = reinterpret_cast<const ASTFunctionReturn*>(node);

        printf("Function Return: BEGIN {\n");
        VisitNode(fnReturn->top);
        printf("Function Return: END {\n");
    }
    else if (node->type == ASTNodeType::OPERATOR)
    {
        const ASTOperator* op = reinterpret_cast<const ASTOperator*>(node);
        const std::string& typeName = Token::TypeToString(op->token->type);

        printf("Operator: (Name: %s) BEGIN {\n", typeName.c_str());
        VisitNode(op->left);
        VisitNode(op->right);
        printf("Operator: (Name: %s) END {\n", typeName.c_str());
    }
    else if (node->type == ASTNodeType::EXPRESSION)
    {
        const ASTExpression* expression = reinterpret_cast<const ASTExpression*>(node);
        const std::string& type = expression->GetTypeName();

        printf("Expression: (Value: %.*s), (Type: %s)\n", nodeNameSize, nodeName.data(), type.c_str());
    }
    else if (node->type == ASTNodeType::STRUCT)
    {
        const ASTStruct* structNode = reinterpret_cast<const ASTStruct*>(node);

        printf("Struct (Name: %.*s), (Variables: %zu), (Functions: %zu)\n", nodeNameSize, nodeName.data(), structNode->variables.size(), structNode->functions.size());

        printf("{\n");
        for (const ASTVariable* variable : structNode->variables)
        {
            VisitNode(variable);
        }

        printf("\n");
        for (const ASTFunctionDecl* fnDecl : structNode->functions)
        {
            VisitNode(fnDecl);
        }
        printf("}\n");
    }
    else if (node->type == ASTNodeType::VARIABLE)
    {
        const ASTVariable* variable = reinterpret_cast<const ASTVariable*>(node);

        const std::string_view& dataTypeName = variable->dataType->GetName();
        const int dataTypeNameSize = variable->dataType->GetNameSize();

        printf("Variable: (Name: %.*s), (DataType: %.*s)\n", nodeNameSize, nodeName.data(), dataTypeNameSize, dataTypeName.data());
        VisitNode(variable->value);
    }
}
