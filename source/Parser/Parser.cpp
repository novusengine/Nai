#include <pch/Build.h>
#include "Parser.h"
#include "../Memory/BlockAllocator.h"
#include <limits>

thread_local Memory::BlockAllocator allocator(64 * 1024 * 1024); // Preallocate 64 MB per thread

ASTSequence* ModuleInfo::GetSequence()
{
    ZoneScopedNC("ModuleInfo::GetSequence", tracy::Color::Aquamarine)
    return allocator.New<ASTSequence>();
}
ASTExpression* ModuleInfo::GetExpression()
{
    ZoneScopedNC("ModuleInfo::GetExpression", tracy::Color::Aquamarine1)
    return allocator.New<ASTExpression>();
}
ASTValue* ModuleInfo::GetValue()
{
    ZoneScopedNC("ModuleInfo::GetValue", tracy::Color::Aquamarine2)
    return allocator.New<ASTValue>();
}
ASTVariable* ModuleInfo::GetVariable()
{
    ZoneScopedNC("ModuleInfo::GetVariable", tracy::Color::Aquamarine2)
    return allocator.New<ASTVariable>();
}
ASTDataType* ModuleInfo::GetDataType()
{
    ZoneScopedNC("ModuleInfo::GetDataType", tracy::Color::Aquamarine2)
    return allocator.New<ASTDataType>();
}
ASTWhileStatement* ModuleInfo::GetWhileStatement()
{
    ZoneScopedNC("ModuleInfo::GetWhileStatement", tracy::Color::Aquamarine3);
    return allocator.New<ASTWhileStatement>();
}
ASTIfStatement* ModuleInfo::GetIfStatement()
{
    ZoneScopedNC("ModuleInfo::GetIfStatement", tracy::Color::Aquamarine3)
    return allocator.New<ASTIfStatement>();
}
ASTJmpStatement* ModuleInfo::GetJmpStatement()
{
    ZoneScopedNC("ModuleInfo::GetJmpStatement", tracy::Color::Aquamarine3)
        return allocator.New<ASTJmpStatement>();
}
ASTReturnStatement* ModuleInfo::GetReturnStatement()
{
    ZoneScopedNC("ModuleInfo::GetReturnStatement", tracy::Color::Aquamarine3)
    return allocator.New<ASTReturnStatement>();
}
ASTFunctionDecl* ModuleInfo::GetFunctionDecl()
{
    ZoneScopedNC("ModuleInfo::GetFunctionDecl", tracy::Color::Aquamarine4)
    return allocator.New<ASTFunctionDecl>();
}
ASTFunctionParameter* ModuleInfo::GetFunctionParameter()
{
    ZoneScopedNC("ModuleInfo::GetFunctionParameter", tracy::Color::Aquamarine4)
    return allocator.New<ASTFunctionParameter>();
}
ASTFunctionCall* ModuleInfo::GetFunctionCall()
{
    ZoneScopedNC("ModuleInfo::GetFunctionCall", tracy::Color::Aquamarine4)
    return allocator.New<ASTFunctionCall>();
}
ASTFunctionArgument* ModuleInfo::GetFunctionArgument()
{
    ZoneScopedNC("ModuleInfo::GetFunctionArgument", tracy::Color::Aquamarine4)
    return allocator.New<ASTFunctionArgument>();
}

ByteInstruction* ModuleInfo::GetByteInstruction()
{
    ZoneScopedNC("ModuleInfo::GetByteInstruction", tracy::Color::Aquamarine4)
        return allocator.New<ByteInstruction>();
}

Parser::Parser()
{
    // Setup Primitive Types
    _typeNameHashToNaiType["auto"_djb2] = NaiType::INVALID;
    _typeNameHashToNaiType["void"_djb2] = NaiType::NAI_VOID;
    _typeNameHashToNaiType["bool"_djb2] = NaiType::U8;
    _typeNameHashToNaiType["i8"_djb2] = NaiType::I8;
    _typeNameHashToNaiType["u8"_djb2] = NaiType::U8;
    _typeNameHashToNaiType["i16"_djb2] = NaiType::I16;
    _typeNameHashToNaiType["u16"_djb2] = NaiType::U16;
    _typeNameHashToNaiType["i32"_djb2] = NaiType::I32;
    _typeNameHashToNaiType["u32"_djb2] = NaiType::U32;
    _typeNameHashToNaiType["i64"_djb2] = NaiType::I64;
    _typeNameHashToNaiType["u64"_djb2] = NaiType::U64;
    _typeNameHashToNaiType["f32"_djb2] = NaiType::F32;
    _typeNameHashToNaiType["f64"_djb2] = NaiType::F64;
}

bool Parser::Run(ModuleInfo& moduleInfo)
{
    ZoneScopedNC("Parser::Run", tracy::Color::Blue)

    // Ensure that our allocator is reset
    allocator.Reset();

    // Ensure ModuleInfo is valid for use
    moduleInfo.ResetIndex();
    
    const size_t& tokenIndex = moduleInfo.GetTokenIndex();
    const size_t& tokenCount = moduleInfo.GetTokenCount();
    
    while (tokenIndex < tokenCount)
    {
        Token* topToken = nullptr;
        moduleInfo.GetToken(&topToken); // We can gaurantee a token exists here due to the (while) statement's condition

        if (topToken->subType == TokenSubType::KEYWORD_FUNCTION)
        {
            // Eat KEYWORD_FUNCTION
            moduleInfo.EatToken();

            if (!ParseFunction(moduleInfo))
                return false;
        }
        else if (topToken->subType == TokenSubType::KEYWORD_STRUCT)
        {

        }
        else if (topToken->subType == TokenSubType::KEYWORD_ENUM)
        {

        }
        else
        {
            if (topToken->type == TokenType::IDENTIFIER)
            {
                if (topToken->subType == TokenSubType::FUNCTION_CALL)
                {
                    moduleInfo.ReportError("Global Function calls are not allowed.", nullptr);
                }
                else
                {
                    moduleInfo.ReportError("Global Variables are not allowed.", nullptr);
                }
            }
            else
            {
                moduleInfo.ReportError("Expected to find a Struct, Enum or Function declaration.", nullptr);
            }

            return false;
        }
    }

    return RunSemanticCheck(moduleInfo);
}
bool Parser::RunSemanticCheck(ModuleInfo& moduleInfo)
{
    std::vector<ASTFunctionDecl*>& fnNodes = moduleInfo.GetFunctionNodes();

    for (ASTFunctionDecl* fnDecl : fnNodes)
    {
        // Check Parameter List
        {
            if (!CheckFunctionParameters(moduleInfo, fnDecl))
                break;
        }

        // Check Body
        {
            if (!CheckFunctionBody(moduleInfo, fnDecl))
                break;
        }
    }

    return true;
}

bool Parser::ParseFunction(ModuleInfo& moduleInfo)
{
    ZoneScopedNC("Parser::ParseFunction", tracy::Color::Blue1)

    Token* identifier = nullptr;
    if (!moduleInfo.EatToken(&identifier) || (identifier->type != TokenType::IDENTIFIER && identifier->subType != TokenSubType::FUNCTION_DECLARATION))
    {
        moduleInfo.ReportError("Expected to find a function name.", nullptr);
        return false;
    }

    ASTFunctionDecl* fnDecl = moduleInfo.GetFunctionDecl();
    fnDecl->UpdateToken(identifier);
    fnDecl->returnType = moduleInfo.GetDataType();
    fnDecl->body = moduleInfo.GetSequence();

    if (!ParseFunctionParameterList(moduleInfo, fnDecl))
        return false;

    if (!ParseFunctionReturnType(moduleInfo, fnDecl))
        return false;
    
    if (!ParseFunctionBody(moduleInfo, fnDecl))
        return false;

    moduleInfo.AddFunctionNode(fnDecl);
    return true;
}
bool Parser::ParseFunctionParameterList(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl)
{
    ZoneScopedNC("Parser::ParseFunctionParameterList", tracy::Color::Blue2)

    Token* leftParen = nullptr;
    if (!moduleInfo.EatToken(&leftParen) || leftParen->type != TokenType::LPAREN)
    {
        moduleInfo.ReportError("Expected to find opening function parentheses.", nullptr);
        return false;
    }

    Token* token = nullptr;
    if (!moduleInfo.GetToken(&token))
    {
        moduleInfo.ReportError("Expected to find 'closing parentheses'.", nullptr);
        return false;
    }

    if (token->type != TokenType::RPAREN)
    {
        // Parse Parameter List
        while (true)
        {
            Token* identifierToken = nullptr;
            if (!moduleInfo.EatToken(&identifierToken) || identifierToken->type != TokenType::IDENTIFIER)
            {
                moduleInfo.ReportError("Expected to find 'function parameter'.", nullptr);
                return false;
            }

            if (identifierToken->type != TokenType::IDENTIFIER)
            {
                moduleInfo.ReportError("Expected to find identifier for parameter.", nullptr);
                return false;
            }
            else if (identifierToken->subType == TokenSubType::FUNCTION_CALL)
            {
                moduleInfo.ReportError("Expected to find identifier for parameter (Did you accidentally add parentheses?).", nullptr);
                return false;
            }

            Token* declarationToken = nullptr;
            if (!moduleInfo.EatToken(&declarationToken) || declarationToken->type != TokenType::DECLARATION)
            {
                moduleInfo.ReportError("Expected to find declaration operator for parameter(%.*s).", nullptr, identifierToken->valueSize, identifierToken->value);
                return false;
            }

            ASTFunctionParameter* param = moduleInfo.GetFunctionParameter();
            param->UpdateToken(identifierToken);
            param->dataType = moduleInfo.GetDataType();

            fnDecl->AddParameter(param);

            if (declarationToken->subType == TokenSubType::NONE || declarationToken->subType == TokenSubType::CONST_DECLARATION)
            {
                Token* dataTypeToken = nullptr;
                if (!moduleInfo.EatToken(&dataTypeToken) || dataTypeToken->type != TokenType::DATATYPE)
                {
                    moduleInfo.ReportError("Expected to find data type for parameter(%.*s).", nullptr, identifierToken->valueSize, identifierToken->value);
                    return false;
                }

                NaiType type = GetTypeFromChar(dataTypeToken->value, dataTypeToken->valueSize);
                param->dataType->UpdateToken(dataTypeToken);
                param->dataType->SetType(type);

                Token* assignToken = nullptr;
                if (!moduleInfo.GetToken(&assignToken) || (assignToken->type != TokenType::OP_ASSIGN && assignToken->type != TokenType::PARAM_SEPERATOR && assignToken->type != TokenType::RPAREN))
                {
                    moduleInfo.ReportError("Expected to find 'assignment operator', 'param seperator' or 'closing parentheses' for parameter(%.*s).", nullptr, identifierToken->valueSize, identifierToken->value);
                    return false;
                }

                if (assignToken->type == TokenType::OP_ASSIGN)
                {
                    // Eat OP_ASSIGN
                    moduleInfo.EatToken();

                    param->expression = moduleInfo.GetExpression();;
                    if (!ParseExpression(moduleInfo, fnDecl, param->expression))
                        return false;
                }
            }
            else if (declarationToken->subType == TokenSubType::DECLARATION_ASSIGN || declarationToken->subType == TokenSubType::CONST_DECLARATION_ASSIGN)
            {
                moduleInfo.ReportError("Declaration Assignment is not allowed for parameters(Name: %.*s).", nullptr, identifierToken->valueSize, identifierToken->value);
                return false;
            }

            Token* paramTerminator = nullptr;
            if (!moduleInfo.GetToken(&paramTerminator) || (paramTerminator->type != TokenType::RPAREN && paramTerminator->type != TokenType::PARAM_SEPERATOR))
            {
                moduleInfo.ReportError("Expected to find parameter seperator OR closing parentheses for parameter(%.*s).", nullptr, identifierToken->valueSize, identifierToken->value);
                return false;
            }

            if (paramTerminator->type == TokenType::PARAM_SEPERATOR)
            {
                // Eat PARAM_SEPERATOR
                moduleInfo.EatToken();
            }
            else if (paramTerminator->type == TokenType::RPAREN)
            {
                break;
            }
        }
    }

    // Eat RightParen
    moduleInfo.EatToken();

    return true;
}
bool Parser::ParseFunctionReturnType(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl)
{
    ZoneScopedNC("Parser::ParseFunctionParameterList", tracy::Color::Blue2)

    Token* typeToken = nullptr;
    if (!moduleInfo.GetToken(&typeToken) || (typeToken->type != TokenType::LBRACE && typeToken->subType != TokenSubType::OP_RETURN_TYPE))
    {
        moduleInfo.ReportError("Expected to find the function body or return type.", nullptr);
        return false;
    }

    // If we see LBRACE, we need to default the function's data type to void
    if (typeToken->type == TokenType::LBRACE)
    {
        fnDecl->returnType->SetType(NaiType::NAI_VOID);
    }
    else
    {
        // Eat Return Type Operator
        moduleInfo.EatToken();

        Token* dataTypeToken = nullptr;
        if (!moduleInfo.EatToken(&dataTypeToken) || (dataTypeToken->type != TokenType::DATATYPE))
        {
            moduleInfo.ReportError("Expected to function return type.", nullptr);
            return false;
        }

        NaiType type = GetTypeFromChar(dataTypeToken->value, dataTypeToken->valueSize);
        fnDecl->returnType->UpdateToken(dataTypeToken);
        fnDecl->returnType->SetType(type);

        if (type == NaiType::INVALID)
        {
            moduleInfo.ReportError("Function has invalid return type (type: %.*s).", nullptr, dataTypeToken->valueSize, dataTypeToken->value);
            return false;
        }
    }

    return true;
}
bool Parser::ParseFunctionBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl)
{
    ZoneScopedNC("Parser::ParseFunctionBody", tracy::Color::Blue3)

    Token* leftBrace = nullptr;
    if (!moduleInfo.EatToken(&leftBrace) || leftBrace->type != TokenType::LBRACE)
    {
        moduleInfo.ReportError("Expected to find opening body brace for function(%.*s).", nullptr, fnDecl->GetNameSize(), fnDecl->GetName());
        return false;
    }

    ASTSequence* currentSequence = fnDecl->body;

    Token* token = nullptr;
    if (!moduleInfo.GetToken(&token))
    {
        moduleInfo.ReportError("Expected to find 'closing bracket'.", nullptr);
        return false;
    }

    if (token->type != TokenType::RBRACE)
    {
        ZoneScopedNC("ParseFunctionBody::ParseBody", tracy::Color::Red)

        // Parse Function Body
        while (true)
        {
            ZoneScopedNC("ParseFunctionBody::Iteration", tracy::Color::Red1)

            Token* startToken = nullptr;
            if (!moduleInfo.EatToken(&startToken) || (startToken->type != TokenType::IDENTIFIER && startToken->type != TokenType::KEYWORD))
            {
                moduleInfo.ReportError("Expected to find identifier or keyword.", nullptr);
                return false;
            }

            if (startToken->type == TokenType::IDENTIFIER)
            {
                ZoneScopedNC("Iteration::Identifier", tracy::Color::Red2)
                if (startToken->subType == TokenSubType::FUNCTION_CALL)
                {
                    ZoneScopedNC("Identifier::FunctionCall", tracy::Color::Red3)
                    ASTFunctionCall* fnCall = moduleInfo.GetFunctionCall();
                    fnCall->UpdateToken(startToken);

                    if (!ParseFunctionCall(moduleInfo, fnDecl, fnCall))
                        return false;

                    currentSequence->left = fnCall;
                }
                else
                {
                    ZoneScopedNC("Identifier::Variable", tracy::Color::Red3)

                    ASTVariable* variable = moduleInfo.GetVariable();
                    moduleInfo.InitVariable(variable, startToken, fnDecl);

                    bool isDeclared = variable->parent;

                    Token* declToken = nullptr;
                    if (!moduleInfo.EatToken(&declToken) || (declToken->type != TokenType::DECLARATION && !declToken->IsAssignOperator()))
                    {
                        moduleInfo.ReportError("Expected to find 'Declaration' or 'Assignment' for Variable(%.*s).", nullptr, startToken->valueSize, startToken->value);
                        return false;
                    }

                    if (declToken->type == TokenType::DECLARATION)
                    {
                        ZoneScopedNC("Variable::Declaration", tracy::Color::Red4)
                        if (declToken->subType == TokenSubType::NONE || declToken->subType == TokenSubType::CONST_DECLARATION)
                        {
                            ZoneScopedNC("Declaration::NonAssign", tracy::Color::Purple)
                            Token* dataTypeToken = nullptr;
                            if (!moduleInfo.EatToken(&dataTypeToken) || dataTypeToken->type != TokenType::DATATYPE)
                            {
                                moduleInfo.ReportError("Expected to find data type for parameter(%.*s).", nullptr, declToken->valueSize, declToken->value);
                                return false;
                            }

                            if (!isDeclared)
                            {
                                NaiType type = GetTypeFromChar(dataTypeToken->value, dataTypeToken->valueSize);
                                variable->dataType->UpdateToken(dataTypeToken);
                                variable->dataType->SetType(type);
                            }

                            Token* assignToken = nullptr;
                            if (!moduleInfo.GetToken(&assignToken) || (assignToken->type != TokenType::OP_ASSIGN && assignToken->type != TokenType::END_OF_LINE))
                            {
                                moduleInfo.ReportError("Expected to find 'assignment operator' or 'end of line' for variable(%.*s).", nullptr, variable->GetNameSize(), variable->GetName());
                                return false;
                            }

                            if (assignToken->type == TokenType::OP_ASSIGN)
                            {
                                // Eat OP_ASSIGN
                                moduleInfo.EatToken();

                                variable->expression = moduleInfo.GetExpression();
                                variable->expression->op = ASTOperatorType::ASSIGN;
                                if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                                    return false;
                            }
                        }
                        else if (declToken->subType == TokenSubType::DECLARATION_ASSIGN || declToken->subType == TokenSubType::CONST_DECLARATION_ASSIGN)
                        {
                            variable->dataType->SetType(NaiType::AUTO);

                            variable->expression = moduleInfo.GetExpression();
                            variable->expression->op = ASTOperatorType::ASSIGN;
                            if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                                return false;
                        }
                    }
                    else if (declToken->IsAssignOperator())
                    {
                        if (!isDeclared)
                        {
                            // Assignment to undeclared variable
                            moduleInfo.ReportError("Use of undeclared variable(%.*s).", nullptr, startToken->valueSize, startToken->value);
                            return false;
                        }

                        variable->expression = moduleInfo.GetExpression();
                        if (!variable->expression->UpdateOperator(declToken))
                        {
                            // Found unsupported operator
                            moduleInfo.ReportError("Use of unsupported operator(%.*s).", nullptr, declToken->valueSize, declToken->value);
                            return false;
                        }

                        if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                            return false;
                    }

                    currentSequence->left = variable;
                }

                Token* nextToken = nullptr;
                if (!moduleInfo.EatToken(&nextToken) || nextToken->type != TokenType::END_OF_LINE)
                {
                    moduleInfo.ReportError("Expected to find 'End of Line'.", nullptr);
                    return false;
                }

            }
            else if (startToken->type == TokenType::KEYWORD)
            {
                // This catches all unallowed keywords (Functions, Structs, Enums, Breaks and Continues)
                if (startToken->subType <= TokenSubType::KEYWORD_CONTINUE)
                {
                    if (startToken->subType >= TokenSubType::KEYWORD_BREAK)
                    {
                        moduleInfo.ReportError("Continue/Break is not allowed outside of a 'for' or 'while' loop.", nullptr);
                        return false;
                    }
                    else
                    {
                        moduleInfo.ReportError("Nested Functions, Structs or Enums are not allowed.", nullptr);
                        return false;
                    }
                }

                if (startToken->subType == TokenSubType::KEYWORD_ELSEIF)
                {
                    moduleInfo.ReportError("Found 'elseif' without parenting 'if statement'.", nullptr);
                    return false;
                }

                // Parses both 'if & 'elseif'
                if (startToken->subType == TokenSubType::KEYWORD_IF)
                {
                    ASTIfStatement* ifStmt = moduleInfo.GetIfStatement();
                    ifStmt->UpdateToken(startToken);
                    ifStmt->ifType = IFStatementType::IF;

                    if (!ParseIfStatement(moduleInfo, fnDecl, ifStmt))
                        return false;

                    currentSequence->left = ifStmt;
                }
                else if (startToken->subType == TokenSubType::KEYWORD_WHILE)
                {
                    ASTWhileStatement* whileStmt = moduleInfo.GetWhileStatement();
                    whileStmt->UpdateToken(startToken);

                    if (!ParseWhileStatement(moduleInfo, fnDecl, whileStmt))
                        return false;

                    currentSequence->left = whileStmt;
                }
                else if (startToken->subType == TokenSubType::KEYWORD_RETURN)
                {
                    ASTReturnStatement* returnStmt = moduleInfo.GetReturnStatement();

                    Token* expressionToken = nullptr;
                    if (!moduleInfo.GetToken(&expressionToken))
                    {
                        moduleInfo.ReportError("Expected to find 'end of line' for return statement.", nullptr);
                        return false;
                    }

                    if (expressionToken->type != TokenType::END_OF_LINE)
                    {
                        if (fnDecl->returnType->GetType() == NaiType::NAI_VOID)
                        {
                            moduleInfo.ReportError("Unexpected return value for function(%.*s) with return type 'void'.", nullptr, fnDecl->GetNameSize(), fnDecl->GetName());
                            return false;
                        }

                        returnStmt->value = moduleInfo.GetExpression();
                        returnStmt->value->UpdateToken(expressionToken);

                        if (!ParseExpression(moduleInfo, fnDecl, returnStmt->value))
                            return false;

                        Token* endToken = nullptr;
                        if (!moduleInfo.EatToken(&endToken) || endToken->type != TokenType::END_OF_LINE)
                        {
                            moduleInfo.ReportError("Expected to find 'end of line' for return statement.", nullptr);
                            return false;
                        }
                    }
                    else
                    {
                        // Eat END_OF_LINE
                        moduleInfo.EatToken();
                    }


                    currentSequence->left = returnStmt;
                }
            }

            currentSequence->right = moduleInfo.GetSequence();
            currentSequence = currentSequence->right;

            Token* endToken = nullptr;
            if (!moduleInfo.GetToken(&endToken))
            {
                moduleInfo.ReportError("Expected to find 'closing bracket'.", nullptr);
                return false;
            }

            if (endToken->type == TokenType::RBRACE)
                break;
        }
    }

    // Eat RightBrace
    moduleInfo.EatToken();
    return true;
}
bool Parser::ParseFunctionCall(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTFunctionCall* out)
{
    ZoneScopedNC("Parser::ParseFunctionCall", tracy::Color::Blue4)

    Token* leftParen = nullptr;
    if (!moduleInfo.EatToken(&leftParen) || leftParen->type != TokenType::LPAREN)
    {
        moduleInfo.ReportError("Expected to find opening parentheses for function call(%.*s).", nullptr, out->GetNameSize(), out->GetName());
        return false;
    }

    Token* token = nullptr;
    if (!moduleInfo.GetToken(&token))
    {
        moduleInfo.ReportError("Expected to find 'closing parentheses'.", nullptr);
        return false;
    }

    if (token->type != TokenType::RPAREN)
    {
        // Parse Parameter List
        while (true)
        {
            Token* argToken = nullptr;
            if (!moduleInfo.EatToken(&argToken) || (argToken->type != TokenType::IDENTIFIER && argToken->type != TokenType::LITERAL))
            {
                moduleInfo.ReportError("Expected to find 'identifier' or 'literal' for argument.", nullptr);
                return false;
            }

            ASTFunctionArgument* argument = moduleInfo.GetFunctionArgument();
            if (argToken->type == TokenType::IDENTIFIER)
            {
                if (argToken->subType == TokenSubType::FUNCTION_CALL)
                {
                    ASTFunctionCall* fnCall = moduleInfo.GetFunctionCall();
                    fnCall->UpdateToken(argToken);

                    if (!ParseFunctionCall(moduleInfo, fnDecl, fnCall))
                        return false;

                    argument->value = fnCall;
                }
                else
                {
                    ASTVariable* variable = moduleInfo.GetVariable();
                    moduleInfo.InitVariable(variable, argToken, fnDecl);

                    bool isDeclared = variable->parent;

                    if (!isDeclared)
                    {
                        // Assignment to undeclared variable
                        moduleInfo.ReportError("Use of undeclared variable(%.*s).", nullptr, variable->GetNameSize(), variable->GetName());
                        return false;
                    }

                    argument->value = variable;
                }
            }
            else if (argToken->type == TokenType::LITERAL)
            {
                ASTValue* param = moduleInfo.GetValue();
                param->UpdateToken(argToken);
                param->UpdateValue();

                argument->value = param;
            }

            Token* expressionSequenceToken = nullptr;
            if (!moduleInfo.GetToken(&expressionSequenceToken))
            {
                moduleInfo.ReportError("Expected to find 'right parentheses' for function argument(Name: %.*s).", nullptr, argToken->valueSize, argToken->value);
                return false;
            }

            if (expressionSequenceToken->IsExpressionOperator())
            {
                // Eat expressionSequenceToken
                moduleInfo.EatToken();

                ASTExpression* expression = moduleInfo.GetExpression();

                if (!expression->UpdateOperator(expressionSequenceToken))
                {
                    // Found unsupported operator
                    moduleInfo.ReportError("Use of unsupported operator(%.*s).", nullptr, expressionSequenceToken->valueSize, expressionSequenceToken->value);
                    return false;
                }

                expression->left = argument->value;
                expression->right = moduleInfo.GetExpression();

                argument->value = expression;
                out->AddArgument(argument);

                if (!ParseExpression(moduleInfo, fnDecl, static_cast<ASTExpression*>(expression->right)))
                    return false;
            }
            else
            {
                out->AddArgument(argument);
            }

            Token* terminatorToken = nullptr;
            if (!moduleInfo.GetToken(&terminatorToken) || (terminatorToken->type != TokenType::PARAM_SEPERATOR && terminatorToken->type != TokenType::RPAREN))
            {
                moduleInfo.ReportError("Expected to find 'parameter seperator' or 'right parentheses' for function argument(Name: %.*s).", nullptr, argToken->valueSize, argToken->value);
                return false;
            }
            
            if (terminatorToken->type == TokenType::PARAM_SEPERATOR)
            {
                // Eat PARAM_SEPERATOR
                moduleInfo.EatToken();
            }
            else if (terminatorToken->type == TokenType::RPAREN)
            {
                break;
            }
        }
    }

    // Eat RightParen
    moduleInfo.EatToken();

    return true;
}
bool Parser::ParseExpression(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTExpression* out)
{
    ZoneScopedNC("Parser::ParseExpression", tracy::Color::AliceBlue)

    Token* token = nullptr;
    if (!moduleInfo.GetToken(&token) || (token->type != TokenType::IDENTIFIER && token->type != TokenType::LITERAL))
    {
        moduleInfo.ReportError("Expected to find variable OR literal for expression value.", nullptr);
        return false;
    }

    // Eat token
    moduleInfo.EatToken();

    ASTNode* valuePtr = nullptr;
    if (token->type == TokenType::IDENTIFIER)
    {
        if (token->subType == TokenSubType::FUNCTION_CALL)
        {
            ASTFunctionCall* fnCall = moduleInfo.GetFunctionCall();

            if (!ParseFunctionCall(moduleInfo, fnDecl, fnCall))
                return false;

            valuePtr = fnCall;
        }
        else
        {
            ASTVariable* variable = moduleInfo.GetVariable();
            moduleInfo.InitVariable(variable, token, fnDecl);

            bool isDeclared = variable->parent;

            if (!isDeclared)
            {
                // Assignment to undeclared variable
                moduleInfo.ReportError("Use of undeclared variable(%.*s).", nullptr, variable->GetNameSize(), variable->GetName());
                return false;
            }

            valuePtr = variable;
        }
    }
    else if (token->type == TokenType::LITERAL)
    {
        ASTValue* value = moduleInfo.GetValue();
        value->UpdateToken(token);
        value->UpdateValue();

        valuePtr = value;
    }

    Token* expressionSequenceToken = nullptr;
    if (!moduleInfo.GetToken(&expressionSequenceToken))
    {
        moduleInfo.ReportError("Expected 'end of line' for expression.", nullptr);
        return false;
    }

    if (expressionSequenceToken->IsExpressionOperator())
    {
        // Eat expressionSequenceToken
        moduleInfo.EatToken();

        out->left = valuePtr;
        if (!out->UpdateOperator(expressionSequenceToken))
        {
            moduleInfo.ReportError("Expected expressional operator for expression value.", nullptr);
            return false;
        }

        Token* leftParenthesesToken = nullptr;
        if (!moduleInfo.GetToken(&leftParenthesesToken))
        {
            moduleInfo.ReportError("Expected 'expressional operator', 'param seperator', 'right parentheses' or 'end of line' for expression.", nullptr);
            return false;
        }

        ASTExpression* rightExpression = moduleInfo.GetExpression();
        out->right = rightExpression;

        // Convert "If" statement into recursive function call (ParseSubExpression)
        if (leftParenthesesToken->type == TokenType::LPAREN)
        {
            // Eat LPAREN
            moduleInfo.EatToken();

            ASTExpression* expression = moduleInfo.GetExpression();
            rightExpression->left = expression;

            if (!ParseExpression(moduleInfo, fnDecl, expression))
                return false;

            Token* rightParenthesesToken = nullptr;
            if (!moduleInfo.EatToken(&rightParenthesesToken) || rightParenthesesToken->type != TokenType::RPAREN)
            {
                moduleInfo.ReportError("Missing closing parentheses.", nullptr);
                return false;
            }
        }
        else
        {
            if (!ParseExpression(moduleInfo, fnDecl, rightExpression))
                return false;
        }
    }
    else
    {
        if (!out->left)
        {
            out->left = valuePtr;
        }
        else
        {
            out->right = valuePtr;
        }
    }

    return true;
}
bool Parser::ParseWhileStatement(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTWhileStatement* out)
{
    ZoneScopedNC("Parser::ParseWhileStatement", tracy::Color::BlueViolet);

    out->body = moduleInfo.GetSequence();
    out->condition = moduleInfo.GetExpression();

    if (!ParseWhileStatementCondition(moduleInfo, fnDecl, out))
        return false;

    if (!ParseWhileStatementBody(moduleInfo, fnDecl, out))
        return false;

    return true;
}
bool Parser::ParseWhileStatementCondition(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTWhileStatement* out)
{
    ZoneScopedNC("Parser::ParseWhileStatementCondition", tracy::Color::BlueViolet);

    Token* leftParen = nullptr;
    if (!moduleInfo.EatToken(&leftParen) || leftParen->type != TokenType::LPAREN)
    {
        moduleInfo.ReportError("Expected to find opening while parentheses.", nullptr);
        return false;
    }

    if (!ParseExpression(moduleInfo, fnDecl, out->condition))
        return false;

    Token* rightParen = nullptr;
    if (!moduleInfo.EatToken(&rightParen) || rightParen->type != TokenType::RPAREN)
    {
        moduleInfo.ReportError("Expected to find closing while parentheses.", nullptr);
        return false;
    }

    return true;
}
bool Parser::ParseWhileStatementBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTWhileStatement* out)
{
    ZoneScopedNC("Parser::ParseWhileStatementBody", tracy::Color::BlueViolet);

    Token* leftBrace = nullptr;
    if (!moduleInfo.EatToken(&leftBrace) || leftBrace->type != TokenType::LBRACE)
    {
        moduleInfo.ReportError("Expected to find 'opening brace' for while statement(%.*s).", nullptr, out->GetNameSize(), out->GetName());
        return false;
    }

    ASTSequence* currentSequence = out->body;

    Token* token = nullptr;
    if (!moduleInfo.GetToken(&token))
    {
        moduleInfo.ReportError("Expected to find 'closing brace' for while statement(%.*s).", nullptr, out->GetNameSize(), out->GetName());
        return false;
    }

    if (token->type != TokenType::RBRACE)
    {
        ZoneScopedNC("ParseWhileStatementBody::ParseBody", tracy::Color::Red);

        // Parse While Statement Body
        while (true)
        {
            ZoneScopedNC("ParseWhileStatementBody::Iteration", tracy::Color::Red1);

            Token* startToken = nullptr;
            if (!moduleInfo.EatToken(&startToken) || (startToken->type != TokenType::IDENTIFIER && startToken->type != TokenType::KEYWORD))
            {
                moduleInfo.ReportError("Expected to find identifier or keyword.", nullptr);
                return false;
            }

            if (startToken->type == TokenType::IDENTIFIER)
            {
                ZoneScopedNC("Iteration::Identifier", tracy::Color::Red2);

                if (startToken->subType == TokenSubType::FUNCTION_CALL)
                {
                    ZoneScopedNC("Identifier::FunctionCall", tracy::Color::Red3);

                    ASTFunctionCall* fnCall = moduleInfo.GetFunctionCall();
                    fnCall->UpdateToken(startToken);

                    if (!ParseFunctionCall(moduleInfo, fnDecl, fnCall))
                        return false;

                    currentSequence->left = fnCall;
                }
                else
                {
                    ZoneScopedNC("Identifier::Variable", tracy::Color::Red3);

                    ASTVariable* variable = moduleInfo.GetVariable();
                    moduleInfo.InitVariable(variable, startToken, fnDecl);

                    bool isDeclared = variable->parent;

                    Token* declToken = nullptr;
                    if (!moduleInfo.EatToken(&declToken) || (declToken->type != TokenType::DECLARATION && !declToken->IsAssignOperator()))
                    {
                        moduleInfo.ReportError("Expected to find 'Declaration' or 'Assignment' for Variable(%.*s).", nullptr, startToken->valueSize, startToken->value);
                        return false;
                    }

                    if (declToken->type == TokenType::DECLARATION)
                    {
                        ZoneScopedNC("Variable::Declaration", tracy::Color::Red4);
                        if (declToken->subType == TokenSubType::NONE || declToken->subType == TokenSubType::CONST_DECLARATION)
                        {
                            ZoneScopedNC("Declaration::NonAssign", tracy::Color::Purple);

                            Token* dataTypeToken = nullptr;
                            if (!moduleInfo.EatToken(&dataTypeToken) || dataTypeToken->type != TokenType::DATATYPE)
                            {
                                moduleInfo.ReportError("Expected to find data type for parameter(%.*s).", nullptr, declToken->valueSize, declToken->value);
                                return false;
                            }

                            if (!isDeclared)
                            {
                                NaiType type = GetTypeFromChar(dataTypeToken->value, dataTypeToken->valueSize);
                                variable->dataType->UpdateToken(dataTypeToken);
                                variable->dataType->SetType(type);
                            }

                            Token* assignToken = nullptr;
                            if (!moduleInfo.GetToken(&assignToken) || (assignToken->type != TokenType::OP_ASSIGN && assignToken->type != TokenType::END_OF_LINE))
                            {
                                moduleInfo.ReportError("Expected to find 'assignment operator' or 'end of line' for variable(%.*s).", nullptr, variable->GetNameSize(), variable->GetName());
                                return false;
                            }

                            if (assignToken->type == TokenType::OP_ASSIGN)
                            {
                                // Eat OP_ASSIGN
                                moduleInfo.EatToken();

                                variable->expression = moduleInfo.GetExpression();
                                variable->expression->op = ASTOperatorType::ASSIGN;
                                if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                                    return false;
                            }
                        }
                        else if (declToken->subType == TokenSubType::DECLARATION_ASSIGN || declToken->subType == TokenSubType::CONST_DECLARATION_ASSIGN)
                        {
                            variable->dataType->SetType(NaiType::AUTO);

                            variable->expression = moduleInfo.GetExpression();
                            variable->expression->op = ASTOperatorType::ASSIGN;
                            if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                                return false;
                        }
                    }
                    else if (declToken->IsAssignOperator())
                    {
                        if (!isDeclared)
                        {
                            // Assignment to undeclared variable
                            moduleInfo.ReportError("Use of undeclared variable(%.*s).", nullptr, startToken->valueSize, startToken->value);
                            return false;
                        }

                        variable->expression = moduleInfo.GetExpression();
                        variable->expression->UpdateOperator(declToken);
                        if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                            return false;
                    }

                    currentSequence->left = variable;
                }

                Token* nextToken = nullptr;
                if (!moduleInfo.EatToken(&nextToken) || nextToken->type != TokenType::END_OF_LINE)
                {
                    moduleInfo.ReportError("Expected to find 'End of Line'.", nullptr);
                    return false;
                }

            }
            else if (startToken->type == TokenType::KEYWORD)
            {
                // This catches all unallowed keywords (Functions, Structs and Enums)
                if (startToken->subType < TokenSubType::KEYWORD_BREAK)
                {
                    moduleInfo.ReportError("Nested Functions, Structs or Enums are not allowed.", nullptr);
                    return false;
                }

                if (startToken->subType == TokenSubType::KEYWORD_ELSEIF)
                {
                    moduleInfo.ReportError("Unexpected 'elseif'.", nullptr);
                    return false;
                }

                // Parses both 'if & 'elseif'
                if (startToken->subType == TokenSubType::KEYWORD_IF)
                {
                    ASTIfStatement* ifStmt = moduleInfo.GetIfStatement();
                    if (!ParseIfStatement(moduleInfo, fnDecl, ifStmt))
                        return false;

                    currentSequence->left = ifStmt;
                }
                else if (startToken->subType == TokenSubType::KEYWORD_CONTINUE)
                {
                    ASTJmpStatement* jmpStmt = moduleInfo.GetJmpStatement();

                    jmpStmt->UpdateToken(startToken);
                    jmpStmt->jmpType = JMPStatementType::CONTINUE;

                    currentSequence->left = jmpStmt;

                    if (!moduleInfo.EatToken())
                    {
                        moduleInfo.ReportError("Expected to find 'end of line' for continue statement(%.*s)", nullptr, jmpStmt->GetNameSize(), jmpStmt->GetName());
                        return false;
                    }
                }
                else if (startToken->subType == TokenSubType::KEYWORD_BREAK)
                {
                    ASTJmpStatement* jmpStmt = moduleInfo.GetJmpStatement();

                    jmpStmt->UpdateToken(startToken);
                    jmpStmt->jmpType = JMPStatementType::BREAK;

                    currentSequence->left = jmpStmt;

                    if (!moduleInfo.EatToken())
                    {
                        moduleInfo.ReportError("Expected to find 'end of line' for break statement(%.*s)", nullptr, jmpStmt->GetNameSize(), jmpStmt->GetName());
                        return false;
                    }
                }
            }

            currentSequence->right = moduleInfo.GetSequence();
            currentSequence = currentSequence->right;

            Token* endToken = nullptr;
            if (!moduleInfo.GetToken(&endToken))
            {
                moduleInfo.ReportError("Expected to find 'closing brace' for while statement(%.*s).", nullptr, out->GetNameSize(), out->GetName());
                return false;
            }

            if (endToken->type == TokenType::RBRACE)
                break;
        }
    }

    // Eat RightBrace
    moduleInfo.EatToken();

    return true;
}
bool Parser::ParseIfStatement(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTIfStatement* out)
{
    ZoneScopedNC("Parser::ParseIfStatement", tracy::Color::BlueViolet);

    out->body = moduleInfo.GetSequence();

    // We only Parse a condition if we see an "if" or "elseif"
    if (out->ifType != IFStatementType::ELSE)
    {
        out->condition = moduleInfo.GetExpression();

        if (!ParseIfStatementCondition(moduleInfo, fnDecl, out))
            return false;
    }

    if (!ParseIfStatementBody(moduleInfo, fnDecl, out))
        return false;

    if (!ParseIfStatementSequence(moduleInfo, fnDecl, out))
        return false;

    return true;
}
bool Parser::ParseIfStatementCondition(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTIfStatement* out)
{
    ZoneScopedNC("Parser::ParseIfStatementCondition", tracy::Color::BlueViolet);

    Token* leftParen = nullptr;
    if (!moduleInfo.EatToken(&leftParen) || leftParen->type != TokenType::LPAREN)
    {
        moduleInfo.ReportError("Expected to find opening if parentheses.", nullptr);
        return false;
    }

    if (!ParseExpression(moduleInfo, fnDecl, out->condition))
        return false;

    Token* rightParen = nullptr;
    if (!moduleInfo.EatToken(&rightParen) || rightParen->type != TokenType::RPAREN)
    {
        moduleInfo.ReportError("Expected to find closing if parentheses.", nullptr);
        return false;
    }

    return true;
}
bool Parser::ParseIfStatementBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTIfStatement* out)
{
    ZoneScopedNC("Parser::ParseIfStatementBody", tracy::Color::BlueViolet)

    Token* leftBrace = nullptr;
    if (!moduleInfo.EatToken(&leftBrace) || leftBrace->type != TokenType::LBRACE)
    {
        moduleInfo.ReportError("Expected to find 'opening brace' for if statement(%.*s).", nullptr, out->GetNameSize(), out->GetName());
        return false;
    }

    ASTSequence* currentSequence = out->body;

    Token* token = nullptr;
    if (!moduleInfo.GetToken(&token))
    {
        moduleInfo.ReportError("Expected to find 'closing brace' for if statement(%.*s).", nullptr, out->GetNameSize(), out->GetName());
        return false;
    }

    if (token->type != TokenType::RBRACE)
    {
        ZoneScopedNC("ParseIfStatementBody::ParseBody", tracy::Color::Red)

        // Parse Function Body
        while (true)
        {
            ZoneScopedNC("ParseIfStatementBody::Iteration", tracy::Color::Red1)

            Token* startToken = nullptr;
            if (!moduleInfo.EatToken(&startToken) || (startToken->type != TokenType::IDENTIFIER && startToken->type != TokenType::KEYWORD))
            {
                moduleInfo.ReportError("Expected to find identifier or keyword.", nullptr);
                return false;
            }

            if (startToken->type == TokenType::IDENTIFIER)
            {
                ZoneScopedNC("Iteration::Identifier", tracy::Color::Red2)

                if (startToken->subType == TokenSubType::FUNCTION_CALL)
                {
                    ZoneScopedNC("Identifier::FunctionCall", tracy::Color::Red3)
                    
                    ASTFunctionCall* fnCall = moduleInfo.GetFunctionCall();
                    fnCall->UpdateToken(startToken);

                    if (!ParseFunctionCall(moduleInfo, fnDecl, fnCall))
                        return false;

                    currentSequence->left = fnCall;
                }
                else
                {
                    ZoneScopedNC("Identifier::Variable", tracy::Color::Red3)

                    ASTVariable* variable = moduleInfo.GetVariable();
                    moduleInfo.InitVariable(variable, startToken, fnDecl);

                    bool isDeclared = variable->parent;

                    Token* declToken = nullptr;
                    if (!moduleInfo.EatToken(&declToken) || (declToken->type != TokenType::DECLARATION && !declToken->IsAssignOperator()))
                    {
                        moduleInfo.ReportError("Expected to find 'Declaration' or 'Assignment' for Variable(%.*s).", nullptr, startToken->valueSize, startToken->value);
                        return false;
                    }

                    if (declToken->type == TokenType::DECLARATION)
                    {
                        ZoneScopedNC("Variable::Declaration", tracy::Color::Red4)
                        if (declToken->subType == TokenSubType::NONE || declToken->subType == TokenSubType::CONST_DECLARATION)
                        {
                            ZoneScopedNC("Declaration::NonAssign", tracy::Color::Purple)
                            
                            Token* dataTypeToken = nullptr;
                            if (!moduleInfo.EatToken(&dataTypeToken) || dataTypeToken->type != TokenType::DATATYPE)
                            {
                                moduleInfo.ReportError("Expected to find data type for parameter(%.*s).", nullptr, declToken->valueSize, declToken->value);
                                return false;
                            }

                            if (!isDeclared)
                            {
                                NaiType type = GetTypeFromChar(dataTypeToken->value, dataTypeToken->valueSize);
                                variable->dataType->UpdateToken(dataTypeToken);
                                variable->dataType->SetType(type);
                            }

                            Token* assignToken = nullptr;
                            if (!moduleInfo.GetToken(&assignToken) || (assignToken->type != TokenType::OP_ASSIGN && assignToken->type != TokenType::END_OF_LINE))
                            {
                                moduleInfo.ReportError("Expected to find 'assignment operator' or 'end of line' for variable(%.*s).", nullptr, variable->GetNameSize(), variable->GetName());
                                return false;
                            }

                            if (assignToken->type == TokenType::OP_ASSIGN)
                            {
                                // Eat OP_ASSIGN
                                moduleInfo.EatToken();

                                variable->expression = moduleInfo.GetExpression();
                                if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                                    return false;
                            }
                        }
                        else if (declToken->subType == TokenSubType::DECLARATION_ASSIGN || declToken->subType == TokenSubType::CONST_DECLARATION_ASSIGN)
                        {
                            variable->dataType->SetType(NaiType::AUTO);

                            variable->expression = moduleInfo.GetExpression();
                            if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                                return false;
                        }
                    }
                    else if (declToken->IsAssignOperator())
                    {
                        if (!isDeclared)
                        {
                            // Assignment to undeclared variable
                            moduleInfo.ReportError("Use of undeclared variable(%.*s).", nullptr, startToken->valueSize, startToken->value);
                            return false;
                        }

                        variable->expression = moduleInfo.GetExpression();
                        if (!ParseExpression(moduleInfo, fnDecl, variable->expression))
                            return false;
                    }

                    currentSequence->left = variable;
                }

                Token* nextToken = nullptr;
                if (!moduleInfo.EatToken(&nextToken) || nextToken->type != TokenType::END_OF_LINE)
                {
                    moduleInfo.ReportError("Expected to find 'End of Line'.", nullptr);
                    return false;
                }

            }
            else if (startToken->type == TokenType::KEYWORD)
            {
                // This catches all unallowed keywords (Functions, Structs, Enums, Breaks and Continues)
                if (startToken->subType <= TokenSubType::KEYWORD_CONTINUE)
                {
                    if (startToken->subType >= TokenSubType::KEYWORD_BREAK)
                    {
                        moduleInfo.ReportError("Continue/Break is not allowed outside of a 'for'/'while' loop.", nullptr);
                        return false;
                    }
                    else
                    {
                        moduleInfo.ReportError("Nested Functions, Structs or Enums are not allowed.", nullptr);
                        return false;
                    }
                }

                if (startToken->subType == TokenSubType::KEYWORD_ELSEIF)
                {
                    moduleInfo.ReportError("Unexpected 'elseif'.", nullptr);
                    return false;
                }

                // Parses both 'if & 'elseif'
                if (startToken->subType == TokenSubType::KEYWORD_IF)
                {
                    ASTIfStatement* ifStmt = moduleInfo.GetIfStatement();
                    if (!ParseIfStatement(moduleInfo, fnDecl, ifStmt))
                        return false;

                    currentSequence->left = ifStmt;
                }
            }

            currentSequence->right = moduleInfo.GetSequence();
            currentSequence = currentSequence->right;

            Token* endToken = nullptr;
            if (!moduleInfo.GetToken(&endToken))
            {
                moduleInfo.ReportError("Expected to find 'closing brace' for if statement(%.*s).", nullptr, out->GetNameSize(), out->GetName());
                return false;
            }

            if (endToken->type == TokenType::RBRACE)
                break;
        }
    }

    // Eat RightBrace
    moduleInfo.EatToken();

    return true;
}
bool Parser::ParseIfStatementSequence(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl , ASTIfStatement* out)
{
    ZoneScopedNC("Parser::ParseIfStatementSequence", tracy::Color::BlueViolet)
    
    Token* sequenceToken = nullptr;
    if (!moduleInfo.GetToken(&sequenceToken))
    {
        moduleInfo.ReportError("Expected to find 'closing bracket'.", nullptr);
        return false;
    }

    if (sequenceToken->type == TokenType::KEYWORD)
    {
        if (sequenceToken->subType == TokenSubType::KEYWORD_ELSEIF || sequenceToken->subType == TokenSubType::KEYWORD_ELSE)
        {
            // We have already seen an "else", but elseif or else was seen again
            if (out->ifType == IFStatementType::ELSE)
            {
                moduleInfo.ReportError("Unexpected 'elseif' or 'else' found where 'else' has already been seen to complete if chain.", nullptr);
                return false;
            }

            out->next = moduleInfo.GetIfStatement();
            out->next->UpdateToken(sequenceToken);

            if (sequenceToken->subType == TokenSubType::KEYWORD_ELSEIF)
            {
                out->next->ifType = IFStatementType::ELSEIF;
            }
            else
            {
                out->next->ifType = IFStatementType::ELSE;
            }

            // Eat SequenceToken
            moduleInfo.EatToken();

            if (!ParseIfStatement(moduleInfo, fnDecl, out->next))
                return false;
        }
    }

    return true;
}

bool Parser::CheckFunctionParameters(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl)
{
    for (ASTFunctionParameter* parameter : fnDecl->GetParameters())
    {
        NaiType dataType = parameter->dataType->GetType();

        // Here we need to run a check if we've seen the custom type, if not report this error
        if (dataType == NaiType::CUSTOM)
        {
            moduleInfo.ReportError("Found function(%.*s) parameter with undeclared type.", parameter->dataType->token, fnDecl->GetNameSize(), fnDecl->GetName());
            return false;;
        }

        if (parameter->expression)
        {
            NaiType valueType;
            if (!GetTypeFromExpression(moduleInfo, valueType, parameter->expression))
                return false;

            if (dataType != valueType)
            {
                // TODO: Handle mismatch between numeric, strings & structs
                moduleInfo.ReportWarning("Mismatching data type for parameter(%.*s) between lVal and rVal.", parameter->token, parameter->GetNameSize(), parameter->GetName());
            }
        }
    }

    return true;
}
bool Parser::CheckFunctionBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl)
{
    ASTDataType* returnType = fnDecl->returnType;
    bool missingReturnStmt = returnType->GetType() != NaiType::NAI_VOID;

    ASTSequence* nextSequence = fnDecl->body;
    if (!nextSequence->left)
    {
        moduleInfo.ReportWarning("Found function with no body.", fnDecl->token);
        return false;
    }

    while (nextSequence)
    {
        ASTNode* left = nextSequence->left;
        if (!left)
            break;

        NaiType type = NaiType::INVALID;
        ASTNode* typeNode = nullptr;

        if (left->type == ASTNodeType::VARIABLE)
        {
            ASTVariable* variable = static_cast<ASTVariable*>(left);
            ASTDataType* dataType = variable->GetDataType();

            type = dataType->GetType();
            typeNode = variable;

            if (variable->expression)
            {
                NaiType valueType;
                if (!GetTypeFromExpression(moduleInfo, valueType, variable->expression))
                    return false;

                if (type != valueType)
                {
                    // TODO: Handle mismatch between numeric, strings & structs
                    moduleInfo.ReportWarning("Mismatching data type for variable(%.*s) between lVal and rVal.", variable->token, variable->GetNameSize(), variable->GetName());
                }
            }
        }
        else if (left->type == ASTNodeType::FUNCTION_CALL)
        {
            ASTFunctionCall* fnCall = static_cast<ASTFunctionCall*>(left);
            ASTFunctionDecl* functionDecl = moduleInfo.GetFunctionByNameHash(fnCall->GetNameHashed());
            
            std::vector<ASTFunctionParameter*> fnParameters = functionDecl->GetParameters();
            std::vector<ASTFunctionArgument*> fnArguments = fnCall->GetArguments();

            for (size_t i = 0; i < fnArguments.size(); i++)
            {
                ASTFunctionParameter* param = fnParameters[i];
                ASTFunctionArgument* arg = fnArguments[i];

                ASTNode* value = arg->value;
                NaiType argType = NaiType::INVALID;

                if (value->type == ASTNodeType::VALUE)
                {
                    ASTValue* litera = static_cast<ASTValue*>(value);

                    NaiType dataType = GetTypeFromLiteral(litera->value);
                    litera->dataType = moduleInfo.GetDataType();
                    litera->dataType->SetType(dataType);
                    argType = dataType;
                    
                }
                else if (value->type == ASTNodeType::VARIABLE)
                {
                    ASTVariable* variable = static_cast<ASTVariable*>(value);
                    argType = GetTypeFromVariable(variable);

                }
                else if (value->type == ASTNodeType::FUNCTION_CALL)
                {
                    ASTFunctionCall* fnCallArg = static_cast<ASTFunctionCall*>(value);
                    argType = GetTypeFromFunctionCall(moduleInfo, fnCallArg);
                }

                if (argType != param->dataType->GetType())
                {
                    // TODO: Handle mismatch between numeric, strings & structs
                    moduleInfo.ReportWarning("Mismatching data type for function argument(%.*s), expected type: (%.*s).", fnCall->token, arg->GetNameSize(), arg->GetName(), param->dataType->GetNameSize(), param->dataType->GetName());
                }
            }

            type = functionDecl->returnType->GetType();
            typeNode = fnCall;

        }
        else if (left->type == ASTNodeType::WHILE_STATEMENT)
        {
            ASTWhileStatement* whileStmt = static_cast<ASTWhileStatement*>(left);

            // Recurisvely Parse While Statements
            if (!GetTypeFromExpression(moduleInfo, type, whileStmt->condition))
                return false;

        }
        else if (left->type == ASTNodeType::IF_STATEMENT)
        {
            ASTIfStatement* ifStmt = static_cast<ASTIfStatement*>(left);

            // Recurisvely Parse IF Statements
            if (!GetTypeFromExpression(moduleInfo, type, ifStmt->condition))
                return false;

        }
        else if (left->type == ASTNodeType::RETURN_STATEMENT)
        {
            ASTReturnStatement* returnStmt = static_cast<ASTReturnStatement*>(left);

            if (returnStmt->value)
            {
                if (!GetTypeFromExpression(moduleInfo, type, returnStmt->value))
                    return false;

            }
            else
            {
                type = NaiType::NAI_VOID;
            }

            typeNode = returnStmt;
            missingReturnStmt = false;
        }

        // Here we need to run a check if we've seen the custom type, if not report this error
        if (type == NaiType::INVALID || type == NaiType::CUSTOM)
        {
            moduleInfo.ReportError("Undeclared data type used for (%.*s).", typeNode->token, typeNode->GetNameSize(), typeNode->GetName());
            return false;;
        }

        nextSequence = nextSequence->right;
    }

    if (missingReturnStmt)
    {
        moduleInfo.ReportError("Found function with no return statement, function expected to return type of (%.*s).", returnType->token, returnType->GetNameSize(), returnType->GetName());
    }

    return true;
}

bool Parser::GetTypeFromExpression(ModuleInfo& moduleInfo, NaiType& outType, ASTExpression* expression)
{
    ASTNode* left = expression->left;
    NaiType lType = NaiType::INVALID;
    {
        if (left->type == ASTNodeType::VALUE)
        {
            ASTValue* litera = static_cast<ASTValue*>(left);
            litera->dataType = moduleInfo.GetDataType();

            NaiType dataType = GetTypeFromLiteral(litera->value);
            litera->dataType->SetType(dataType);

            lType = dataType;
        }
        else if (left->type == ASTNodeType::VARIABLE)
        {
            ASTVariable* variable = static_cast<ASTVariable*>(left);
            lType = GetTypeFromVariable(variable);
        }
        else if (left->type == ASTNodeType::FUNCTION_CALL)
        {
            ASTFunctionCall* fnCall = static_cast<ASTFunctionCall*>(left);
            lType = GetTypeFromFunctionCall(moduleInfo, fnCall);
        }
        else if (left->type == ASTNodeType::EXPRESSION)
        {
            // TODO: Implement nested expression value logic here
        }
    }

    if (lType == NaiType::INVALID)
    {
        moduleInfo.ReportError("Failed to parse data type for expression", left->token);
        return false;
    }

    ASTNode* right = expression->right;
    NaiType rType = NaiType::INVALID;
    {
        if (right)
        {
            if (right->type == ASTNodeType::EXPRESSION)
            {
                GetTypeFromExpression(moduleInfo, rType, static_cast<ASTExpression*>(expression->right));
            }
            else if (right->type == ASTNodeType::VALUE)
            {
                ASTValue* litera = static_cast<ASTValue*>(right);
                litera->dataType = moduleInfo.GetDataType();

                NaiType dataType = GetTypeFromLiteral(litera->value);
                litera->dataType->SetType(dataType);

                rType = dataType;
            }
            else if (right->type == ASTNodeType::VARIABLE)
            {
                ASTVariable* variable = static_cast<ASTVariable*>(left);
                rType = GetTypeFromVariable(variable);
            }
            else if (right->type == ASTNodeType::FUNCTION_CALL)
            {
                ASTFunctionCall* fnCall = static_cast<ASTFunctionCall*>(left);
                rType = GetTypeFromFunctionCall(moduleInfo, fnCall);
            }
            else if (right->type == ASTNodeType::EXPRESSION)
            {
                // TODO: Implement nested expression value logic here
            }

            if (rType == NaiType::INVALID)
            {
                moduleInfo.ReportError("Failed to parse data type for expression", right->token);
                return false;
            }
        }
    }

    outType = lType;
    return true;
}
NaiType Parser::GetTypeFromLiteral(uint64_t value)
{
    if (value < std::numeric_limits<int32_t>().max())
        return NaiType::I32;
    else
    {
        return NaiType::I64;
    }

    return NaiType::INVALID;
}
NaiType Parser::GetTypeFromVariable(ASTVariable* variable)
{
    return variable->GetDataType()->GetType();
}
NaiType Parser::GetTypeFromFunctionCall(ModuleInfo& moduleInfo, ASTFunctionCall* fnCall)
{
    ASTFunctionDecl* fnDecl = moduleInfo.GetFunctionByNameHash(fnCall->GetNameHashed());
    return fnDecl->returnType->GetType();
}