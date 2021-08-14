#include "pch/Build.h"
#include "Parser.h"
#include "../Module.h"
#include "../Tree.h"
#include "Typer.h"

i8 GetBinaryPrecedence(Token::Kind kind)
{
    switch (kind)
    {
    case Token::Kind::Op_Multiply:
    case Token::Kind::Op_Divide:
    case Token::Kind::Op_Modulo:
        return 30;

    case Token::Kind::Op_Add:
    case Token::Kind::Op_Subtract:
        return 24;

    case Token::Kind::Op_LessThan:
    case Token::Kind::Op_LessEqual:
    case Token::Kind::Op_GreaterThan:
    case Token::Kind::Op_GreaterEqual:
        return 20;

    case Token::Kind::Op_Equal:
    case Token::Kind::Op_NotEqual:
        return 19;

    case Token::Kind::Op_Assign:
        return 1;

    default:
        return 0;
    }
}
Binary::Kind GetBinaryKind(Token::Kind kind)
{
    switch (kind)
    {
    case Token::Kind::Op_Equal:
        return Binary::Kind::Equal;
    case Token::Kind::Op_NotEqual:
        return Binary::Kind::NotEqual;
    case Token::Kind::Op_LessThan:
        return Binary::Kind::LessThan;
    case Token::Kind::Op_LessEqual:
        return Binary::Kind::LessEqual;
    case Token::Kind::Op_GreaterThan:
        return Binary::Kind::GreaterThan;
    case Token::Kind::Op_GreaterEqual:
        return Binary::Kind::GreaterEqual;
    case Token::Kind::Op_Add:
        return Binary::Kind::Add;
    case Token::Kind::Op_Subtract:
        return Binary::Kind::Subtract;
    case Token::Kind::Op_Multiply:
        return Binary::Kind::Multiply;
    case Token::Kind::Op_Divide:
        return Binary::Kind::Divide;
    case Token::Kind::Op_Modulo:
        return Binary::Kind::Modulo;
    case Token::Kind::Op_Assign:
        return Binary::Kind::Assign;

    default:
        return Binary::Kind::None;
    }
}

void Parser::Process(Module* module)
{
    Statement* stmt = ParseBlock(module);

    assert(stmt->kind == Statement::Kind::Compound);
    assert(stmt->compound.scope->parent == nullptr);

    Scope* globalScope = stmt->compound.scope;
    
    ListNode* node;
    /*ListIterate(&stmt->compound.statements, node)
    {
        Statement* statement = ListGetStructPtr(node, Statement, listNode);
        if (statement->kind != Statement::Kind::Comment)
        {
            DebugHandler::PrintError("Parser : The Global Scope may only define: Includes, Aliases, Structs, Enums and Functions.");
            exit(1);
        }
    }*/

    ListIterate(&globalScope->declarations, node)
    {
        Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);

        if (declaration->kind == Declaration::Kind::Variable)
        {
            DebugHandler::PrintError("Parser : Global Variables are not allowed. (Line: %u, Column: %u)", declaration->token->line, declaration->token->column);
            exit(1);
        }
    }

    module->parserInfo.block = &stmt->compound;
    //TreePrinter::PrintStatement(stmt);

    DebugHandler::PrintSuccess("Parser : Successfully Processsed Module (%s)", module->nameHash.name.c_str());
}

Statement* Parser::ParseBlock(Module* module)
{
    Compound* compound = Statement::CreateCompound();
    compound->scope = EnterScope(module);

    while (Token* token = module->lexerInfo.PeekToken())
    {
        if (token->kind == Token::Kind::End_of_File ||
            token->kind == Token::Kind::Curley_Bracket_Close)
            break;
        
        if (Statement* statement = ParseDeclarationOrStatement(module))
        {
            List::AddNodeBack(&compound->statements, &statement->listNode);
        }
    }

    ExitScope(module);
    return reinterpret_cast<Statement*>(compound);
}

Scope* Parser::EnterScope(Module* module)
{
    Scope* currentScope = module->parserInfo.currentScope;
    Scope* scope = new Scope();

    if (currentScope)
    {
        List::AddNodeBack(&currentScope->scopeChildren, &scope->listNode);
    }

    scope->parent = currentScope;
    module->parserInfo.currentScope = scope;

    return scope;
}

void Parser::ExitScope(Module* module)
{
    assert(module->parserInfo.currentScope);
    module->parserInfo.currentScope = module->parserInfo.currentScope->parent;
}

StructScope* Parser::EnterStructScope(Module* module)
{
    StructScope* scope = new StructScope();

    scope->parent = module->parserInfo.currentStructScope;
    module->parserInfo.currentStructScope = scope;

    return scope;
}

void Parser::ExitStructScope(Module* module)
{
    assert(module->parserInfo.currentStructScope);
    module->parserInfo.currentStructScope = module->parserInfo.currentStructScope->parent;
}

void Parser::EnterLoop(Module* module, Loop* loop)
{
    Loop* currentLoop = module->parserInfo.currentLoop;

    loop->parent = currentLoop;
    module->parserInfo.currentLoop = loop;
}

void Parser::ExitLoop(Module* module)
{
    assert(module->parserInfo.currentLoop);
    module->parserInfo.currentLoop = module->parserInfo.currentLoop->parent;
}

Statement* Parser::ParseDeclarationOrStatement(Module* module)
{
    Statement* stmt;
    if (TryParseDeclaration(module, stmt))
    {
        if (stmt)
            return stmt;

        return nullptr;
    }

    return ParseStatement(module);
}

bool Parser::TryParseDeclaration(Module* module, Statement*& stmt)
{
    stmt = nullptr;

    Token* keywordToken = module->lexerInfo.PeekToken();
    if (keywordToken->kind != Token::Kind::Identifier &&
        keywordToken->kind != Token::Kind::Keyword_Struct &&
        keywordToken->kind != Token::Kind::Keyword_Enum &&
        keywordToken->kind != Token::Kind::Keyword_Fn)
        return false;

    Token* declToken = module->lexerInfo.PeekToken(1);
    if (keywordToken->kind == Token::Kind::Identifier &&
        (declToken->kind != Token::Kind::Colon && declToken->kind != Token::Kind::DoubleColon))
        return false;

    Declaration* declaration = new Declaration();

    if (keywordToken->kind == Token::Kind::Identifier)
    {
        declaration->token = keywordToken;
        declaration->kind = Declaration::Kind::Variable;

        module->lexerInfo.SkipToken(Token::Kind::Identifier);
        module->lexerInfo.SkipToken(Token::Kind::Colon);

        declaration->type = ParseType(module);

        DeclarationPushToCurrentScope(module, declaration);

        assert(declaration->type);

        Token* token = module->lexerInfo.PeekToken();
        if (token->kind == Token::Kind::Op_Assign)
        {
            stmt = Statement::Create(Statement::Kind::Expression);
            stmt->expression = Expression::Create(Expression::Kind::Binary);

            stmt->expression->binary.kind = Binary::Kind::Assign;
            stmt->expression->binary.op = module->lexerInfo.ConsumeToken();

            stmt->expression->binary.left = Expression::CreatePrimary(Primary::Kind::Identifier);
            stmt->expression->binary.left->primary.token = declaration->token;

            stmt->expression->binary.right = ParseExpression(module, Expression::InitialPriority);
        }

        module->lexerInfo.SkipToken(Token::Kind::Semicolon);
        return true;
    }
    else if (keywordToken->kind == Token::Kind::Keyword_Struct)
    {
        declaration->token = declToken;
        declaration->kind = Declaration::Kind::Type;
        declaration->type = ParseStructDeclaration(module, false);

        DeclarationPushToCurrentScope(module, declaration);
        return true;
    }
    else if (keywordToken->kind == Token::Kind::Keyword_Enum)
    {
        declaration->token = declToken;
        declaration->kind = Declaration::Kind::Type;

        // Parse Enum

        return false;
    }
    else if (keywordToken->kind == Token::Kind::Keyword_Fn)
    {
        declaration->token = declToken;
        declaration->kind = Declaration::Kind::Function;
        declaration->type = Type::CreateFunction();

        module->lexerInfo.SkipToken(Token::Kind::Keyword_Fn);
        module->lexerInfo.SkipToken(Token::Kind::Identifier);
        module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Open);

        Function* function = &declaration->function;
        function->scope = EnterScope(module);

        while (Token* token = module->lexerInfo.PeekToken())
        {
            if (token->kind == Token::Kind::End_of_File ||
                token->kind == Token::Kind::Parenthesis_Close)
                break;

            ParseFunctionArgument(module);

            token = module->lexerInfo.PeekToken();
            if (token->kind != Token::Kind::Parenthesis_Close)
            {
                module->lexerInfo.SkipToken(Token::Kind::Comma);
            }
        }
        
        Token* token = module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Close);
        if (token->kind == Token::Kind::Arrow)
        {
            module->lexerInfo.SkipToken(Token::Kind::Arrow);
            function->returnType = ParseType(module);
        }
        else
        {
            function->returnType = Type::Create(Type::Kind::Void);
        }

        function->body = ParseStatementCompound(module);
        declaration->function.flags.nativeCall = false;

        ExitScope(module);
        DeclarationPushToCurrentScope(module, declaration);
        return true;
    }

    return false;
}

Statement* Parser::ParseStatement(Module* module)
{
    Token* token = module->lexerInfo.PeekToken();

    if (token->kind == Token::Kind::Comment)
    {
        Statement* stmt = Statement::Create(Statement::Kind::Comment);
        stmt->comment.token = token;

        module->lexerInfo.SkipToken(Token::Kind::Comment);
        return stmt;
    }
    else if (token->kind == Token::Kind::Curley_Bracket_Open)
    {
        return ParseStatementCompound(module);
    }
    else if (token->kind == Token::Kind::Keyword_Return)
    {
        Statement* stmt = Statement::Create(Statement::Kind::Return);

        Token* next = module->lexerInfo.SkipToken(Token::Kind::Keyword_Return);
        if (next->kind == Token::Kind::Semicolon)
        {
            stmt->returnStmt.expression = nullptr;
        }
        else
        {
            stmt->returnStmt.expression = ParseExpression(module, Expression::InitialPriority);
        }

        module->lexerInfo.SkipToken(Token::Kind::Semicolon);

        return stmt;
    }
    else if (token->kind == Token::Kind::Keyword_Loop)
    {
        return ParseStatementLoop(module);
    }
    else if (token->kind == Token::Kind::Keyword_If)
    {
        return ParseStatementConditional(module);
    }
    else if (token->kind == Token::Kind::Keyword_Continue)
    {
        if (!module->parserInfo.currentLoop)
        {
            module->lexerInfo.Error(token, "You cannot 'continue' outside a loop");
        }

        Statement* stmt = Statement::Create(Statement::Kind::Continue);
        module->lexerInfo.SkipToken(Token::Kind::Keyword_Continue);
        module->lexerInfo.SkipToken(Token::Kind::Semicolon);
        return stmt;
    }
    else if (token->kind == Token::Kind::Keyword_Break)
    {
        if (!module->parserInfo.currentLoop)
        {
            module->lexerInfo.Error(token, "You cannot 'break' outside a loop");
        }

        Statement* stmt = Statement::Create(Statement::Kind::Break);
        module->lexerInfo.SkipToken(Token::Kind::Keyword_Break);
        module->lexerInfo.SkipToken(Token::Kind::Semicolon);
        return stmt;
    }

    return ParseStatementExpression(module);
}

Type* Parser::ParseStructDeclaration(Module* module, bool isAnonymous)
{
    Token* keywordToken = module->lexerInfo.ConsumeToken();

    Type* type = Type::CreateStruct();
    type->structType.isStruct = keywordToken->kind == Token::Kind::Keyword_Struct;

    if (!isAnonymous)
    {
        keywordToken = module->lexerInfo.SkipToken(Token::Kind::Identifier);
        type->structType.scope = EnterStructScope(module);
    }
    else
    {
        type->structType.scope = nullptr;
    }
    
    module->lexerInfo.SkipToken(Token::Kind::Curley_Bracket_Open);

    while (Token* token = module->lexerInfo.PeekToken())
    {
        if (token->kind == Token::Kind::End_of_File ||
            token->kind == Token::Kind::Curley_Bracket_Close)
            break;

        StructMember* member = ParseStructMember(module);
        List::AddNodeBack(&type->structType.members, &member->listNode);

        if (StructHasMember(module, member))
        {
            module->lexerInfo.Error(member->token, "struct member have been defined above");
        }

        StructPushMemberToScope(module, member);
        token = module->lexerInfo.PeekToken();
    }

    keywordToken = module->lexerInfo.SkipToken(Token::Kind::Curley_Bracket_Close);

    if (!isAnonymous)
    {
        ExitStructScope(module);
    }

    return type;
}

StructMember* Parser::ParseStructMember(Module* module)
{
    Token* token = module->lexerInfo.PeekToken();

    assert(module->parserInfo.currentStructScope);

    if (token->kind != Token::Kind::Identifier &&
        token->kind != Token::Kind::Keyword_Struct &&
        token->kind != Token::Kind::Keyword_Union)
    {
        module->lexerInfo.Error(token, "Expecting either an Identifier, Struct or Union");
    }

    StructMember* member = new StructMember();
    member->token = token;

    if (token->kind == Token::Kind::Identifier)
    {
        member->isAnonymous = false;

        module->lexerInfo.SkipToken(Token::Kind::Identifier);
        module->lexerInfo.SkipToken(Token::Kind::Colon);

        member->type = ParseType(module);
        module->lexerInfo.SkipToken(Token::Kind::Semicolon);
    }
    else
    {
        Token* next = module->lexerInfo.PeekToken(1);
        if (next->kind == Token::Kind::Identifier)
        {
            member->isAnonymous = false;
            member->token = next;
        }
        else
        {
            member->isAnonymous = true;
        }

        member->type = ParseStructDeclaration(module, member->isAnonymous);
    }

    return member;
}

bool Parser::StructHasMember(Module* module, StructMember* member)
{
    assert(module->parserInfo.currentStructScope);

    ListNode* node;
    ListIterate(&module->parserInfo.currentStructScope->members, node)
    {
        StructMember* scopeMember = ListGetStructPtr(node, StructMember, scopeNode);

        assert(scopeMember->isAnonymous == false);
        if (member->token->nameHash.hash == scopeMember->token->nameHash.hash)
            return true;
    }

    return false;
}

void Parser::StructPushMemberToScope(Module* module, StructMember* member)
{
    if (member->isAnonymous)
        return;

    List::AddNodeBack(&module->parserInfo.currentStructScope->members, &member->scopeNode);
}

void Parser::DeclarationPushToScope(Module* module, Declaration* declaration, Scope* scope)
{
    if (Declaration* existingDeclaration = FindDeclarationInScope(declaration, scope))
    {
        if (declaration->kind == Declaration::Kind::Function && declaration->function.flags.nativeCall)
        {
            module->lexerInfo.Error(existingDeclaration->token, "Native Function (%.*s) has conflicting function on line %u", declaration->token->nameHash.length, declaration->token->nameHash.name, existingDeclaration->token->line);
        }
        else
        {
            module->lexerInfo.Error(declaration->token, "Declaration (%.*s) has already been defined on line %u", declaration->token->nameHash.length, declaration->token->nameHash.name, existingDeclaration->token->line);
        }
    }

    List::AddNodeBack(&scope->declarations, &declaration->listNode);
}

void Parser::DeclarationPushToCurrentScope(Module* module, Declaration* declaration)
{
    DeclarationPushToScope(module, declaration, module->parserInfo.currentScope);
}

void Parser::ParseFunctionArgument(Module* module)
{
    Declaration* declaration = new Declaration();
    declaration->kind = Declaration::Kind::Variable;
    declaration->token = module->lexerInfo.ConsumeToken();

    if (declaration->token->kind != Token::Kind::Identifier)
    {
        module->lexerInfo.Error(declaration->token, "Expected to find Identifier as function argument name");
    }
    module->lexerInfo.SkipToken(Token::Kind::Colon);

    declaration->type = ParseType(module);
    DeclarationPushToCurrentScope(module, declaration);
}

Statement* Parser::ParseStatementLoop(Module* module)
{
    module->lexerInfo.SkipToken(Token::Kind::Keyword_Loop);

    Statement* stmt = Statement::Create(Statement::Kind::Loop);
    stmt->loop.condition = ParseExpression(module, Expression::InitialPriority);

    EnterLoop(module, &stmt->loop);
    stmt->loop.body = ParseStatementCompound(module);
    ExitLoop(module);

    return stmt;
}

Statement* Parser::ParseStatementConditional(Module* module)
{
    Token* token = module->lexerInfo.SkipToken(Token::Kind::Keyword_If);

    Statement* stmt = Statement::Create(Statement::Kind::Conditional);
    stmt->conditional.condition = ParseExpression(module, Expression::InitialPriority);
    stmt->conditional.trueBody = ParseStatementCompound(module);

    token = module->lexerInfo.PeekToken();
    if (token->kind == Token::Kind::Keyword_Else)
    {
        token = module->lexerInfo.SkipToken(Token::Kind::Keyword_Else);

        if (token->kind == Token::Kind::Keyword_If)
        {
            stmt->conditional.falseBody = ParseStatementConditional(module);
        }
        else
        {
            stmt->conditional.falseBody = ParseStatementCompound(module);
        }
    }
    else
    {
        stmt->conditional.falseBody = nullptr;
    }

    return stmt;
}

Statement* Parser::ParseStatementExpression(Module* module)
{
    Statement* stmt = Statement::Create(Statement::Kind::Expression);
    stmt->expression = ParseExpression(module, Expression::InitialPriority);

    module->lexerInfo.SkipToken(Token::Kind::Semicolon);
    return stmt;
}

Statement* Parser::ParseStatementCompound(Module* module)
{
    module->lexerInfo.SkipToken(Token::Kind::Curley_Bracket_Open);
    Statement* stmt = ParseBlock(module);
    module->lexerInfo.SkipToken(Token::Kind::Curley_Bracket_Close);

    return stmt;
}

Expression* Parser::ParseExpression(Module* module, i8 priority)
{
    Expression* left = ParseExpressionUnary(module);

    while (true)
    {
        Token* token = module->lexerInfo.PeekToken();

        i8 newPriority = GetBinaryPrecedence(token->kind);
        if (newPriority == 0 || newPriority <= priority)
            return left;
        
        Expression* binaryExpr = Expression::Create(Expression::Kind::Binary);

        binaryExpr->binary.kind = GetBinaryKind(token->kind);
        binaryExpr->binary.op = module->lexerInfo.ConsumeToken();
        binaryExpr->binary.left = left;
        binaryExpr->binary.right = ParseExpression(module, newPriority);

        left = binaryExpr;
    }
}

Expression* Parser::ParseExpressionUnary(Module* module)
{
    Token* token = module->lexerInfo.PeekToken();

    if (token->kind == Token::Kind::Parenthesis_Open)
    {
        module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Open);
        Expression* expr = ParseExpression(module, Expression::InitialPriority);
        module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Close);

        return ParseExpressionSuffix(module, expr);
    }
    else if (token->kind == Token::Kind::Op_Multiply)
    {
        module->lexerInfo.SkipToken(Token::Kind::Op_Multiply);
        Expression* unaryExpr = Expression::Create(Expression::Kind::Unary);

        unaryExpr->unary.kind = Unary::Kind::Deref;
        unaryExpr->unary.op = token;
        unaryExpr->unary.operand = ParseExpressionUnary(module);

        return unaryExpr;
    }
    else if (token->kind == Token::Kind::Op_At)
    {
        module->lexerInfo.SkipToken(Token::Kind::Op_At);
        Expression* unaryExpr = Expression::Create(Expression::Kind::Unary);

        unaryExpr->unary.kind = Unary::Kind::AddressOf;
        unaryExpr->unary.op = token;
        unaryExpr->unary.operand = ParseExpressionUnary(module);

        return unaryExpr;
    }
    else if (token->kind == Token::Kind::Keyword_New)
    {
        module->lexerInfo.SkipToken(Token::Kind::Keyword_New);

        Expression* memoryExpr = Expression::Create(Expression::Kind::MemoryNew);
        {
            module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Open);
            memoryExpr->memory.token = token;

            Token* nextToken = module->lexerInfo.PeekToken();
            if (nextToken->kind != Token::Kind::Parenthesis_Close)
            {
                memoryExpr->memory.expression = ParseExpression(module, Expression::InitialPriority);
            }
            else
            {
                memoryExpr->memory.expression = Expression::CreatePrimary(Primary::Kind::Number);
                memoryExpr->memory.expression->primary.number = 1;
            }

            module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Close);
        }

        return memoryExpr;
    }
    else if (token->kind == Token::Kind::Keyword_Free)
    {
        module->lexerInfo.SkipToken(Token::Kind::Keyword_Free);

        Expression* memoryExpr = Expression::Create(Expression::Kind::MemoryFree);
        {
            module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Open);
            memoryExpr->memory.token = token;
            memoryExpr->memory.expression = ParseExpression(module, Expression::InitialPriority);
            module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Close);
        }

        return memoryExpr;
    }

    Expression* primary = ParseExpressionPrimary(module);
    return ParseExpressionSuffix(module, primary);
}

Expression* Parser::ParseExpressionSuffix(Module* module, Expression* prev)
{
    Token* token = module->lexerInfo.PeekToken();

    if (token->kind == Token::Kind::Parenthesis_Open)
    {
        module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Open);

        Expression* callExpr = Expression::CreateCall();
        callExpr->call.expression = prev;

        if (prev->kind == Expression::Kind::Primary)
        {
            callExpr->call.token = prev->primary.token;
        }
        else
        {
            callExpr->call.token = token;
        }

        while (Token* newToken = module->lexerInfo.PeekToken())
        {
            if (newToken->kind == Token::Kind::End_of_File ||
                newToken->kind == Token::Kind::Parenthesis_Close)
                break;

            Expression* expression = ParseExpression(module, Expression::InitialPriority);
            List::AddNodeBack(&callExpr->call.arguments, &expression->listNode);

            newToken = module->lexerInfo.PeekToken();
            if (newToken->kind != Token::Kind::Parenthesis_Close)
            {
                module->lexerInfo.SkipToken(Token::Kind::Comma);
            }
        }

        module->lexerInfo.SkipToken(Token::Kind::Parenthesis_Close);
        return ParseExpressionSuffix(module, callExpr);
    }
    else if (token->kind == Token::Kind::Bracket_Open)
    {
        module->lexerInfo.SkipToken(Token::Kind::Bracket_Open);

        Expression* binaryExpr = Expression::Create(Expression::Kind::Binary);
        binaryExpr->binary.kind = Binary::Kind::Add;
        binaryExpr->binary.op = token;
        binaryExpr->binary.left = prev;
        binaryExpr->binary.right = ParseExpression(module, Expression::InitialPriority);

        Expression* unaryExpr = Expression::Create(Expression::Kind::Unary);
        unaryExpr->unary.kind = Unary::Kind::Deref;
        unaryExpr->unary.op = token;
        unaryExpr->unary.operand = binaryExpr;

        module->lexerInfo.SkipToken(Token::Kind::Bracket_Close);
        return ParseExpressionSuffix(module, unaryExpr);
    }
    else if (token->kind == Token::Kind::Dot)
    {
        module->lexerInfo.SkipToken(Token::Kind::Dot);

        Expression* dotExpression = Expression::Create(Expression::Kind::Dot);
        dotExpression->dot.token = token;
        dotExpression->dot.tokenMember = module->lexerInfo.PeekToken();
        dotExpression->dot.expression = prev;

        module->lexerInfo.SkipToken(Token::Kind::Identifier);
        return ParseExpressionSuffix(module, dotExpression);
    }

    return prev;
}

Expression* Parser::ParseExpressionPrimary(Module* module)
{
    Token* token = module->lexerInfo.ConsumeToken();

    Expression* primaryExpr = Expression::CreatePrimary(Primary::Kind::None);
    primaryExpr->primary.token = token;

    switch (token->kind)
    {
        case Token::Kind::Number:
        {
            primaryExpr->primary.kind = Primary::Kind::Number;
            primaryExpr->primary.number = token->number;
            break;
        }
        case Token::Kind::Identifier:
        {
            primaryExpr->primary.kind = Primary::Kind::Identifier;
            break;
        }
        case Token::Kind::String:
        {
            primaryExpr->primary.kind = Primary::Kind::String;
            primaryExpr->primary.string = new String(token->nameHash.name, token->nameHash.length);
            break;
        }

        default:
        {
            module->lexerInfo.Error(token, "Expected a primary expression");
        }
    }

    return primaryExpr;
}

Declaration* Parser::FindDeclarationInScope(Declaration* declaration, Scope* scope)
{
    ListNode* node;
    ListIterate(&scope->declarations, node)
    {
        Declaration* nodeDeclaration = ListGetStructPtr(node, Declaration, listNode);

        if (declaration->token->nameHash.hash == nodeDeclaration->token->nameHash.hash)
            return nodeDeclaration;
    }

    return nullptr;
}

Type* Parser::ParseType(Module* module)
{
    Token* token = module->lexerInfo.ConsumeToken();

    if (token->nameHash.hash == "char"_djb2)
    {
        return ParseTypeSuffix(module, Type_Char);
    }
    else if (token->nameHash.hash == "bool"_djb2)
    {
        return ParseTypeSuffix(module, Type_Char);
    }
    else if (token->nameHash.hash == "u8"_djb2)
    {
        return ParseTypeSuffix(module, Type_U8);
    }
    else if (token->nameHash.hash == "u16"_djb2)
    {
        return ParseTypeSuffix(module, Type_U16);
    }
    else if (token->nameHash.hash == "u32"_djb2)
    {
        return ParseTypeSuffix(module, Type_U32);
    }
    else if (token->nameHash.hash == "u64"_djb2)
    {
        return ParseTypeSuffix(module, Type_U64);
    }
    else if (token->nameHash.hash == "i8"_djb2)
    {
        return ParseTypeSuffix(module, Type_I8);
    }
    else if (token->nameHash.hash == "i16"_djb2)
    {
        return ParseTypeSuffix(module, Type_I16);
    }
    else if (token->nameHash.hash == "i32"_djb2)
    {
        return ParseTypeSuffix(module, Type_I32);
    }
    else if (token->nameHash.hash == "i64"_djb2)
    {
        return ParseTypeSuffix(module, Type_I64);
    }
    else if (token->kind == Token::Kind::Identifier)
    {
        Type* type = Type::Create(Type::Kind::Unknown);
        type->unknown.token = token;

        return ParseTypeSuffix(module, type);
    }

    module->lexerInfo.Error(token, "Expecting a datatype");
    return nullptr;
}

Type* Parser::ParseTypeSuffix(Module* module, Type* type)
{
    Token* token = module->lexerInfo.PeekToken();

    if (token->kind == Token::Kind::Op_Multiply)
    {
        module->lexerInfo.SkipToken(Token::Kind::Op_Multiply);

        Type* typePtr = Type::CreatePointer();
        typePtr->pointer.type = type;

        return ParseTypeSuffix(module, typePtr);
    }
    else if (token->kind == Token::Kind::Bracket_Open)
    {
        // Consume Bracket_Open
        module->lexerInfo.SkipToken(Token::Kind::Bracket_Open);

        token = module->lexerInfo.PeekToken();

        if (token->kind != Token::Kind::Number)
        {
            module->lexerInfo.Error(token, "Array Index must be compile time constant");
        }

        Type* typePtr = Type::CreatePointer();
        typePtr->pointer.count = token->number;

        module->lexerInfo.SkipToken(Token::Kind::Number);
        module->lexerInfo.SkipToken(Token::Kind::Bracket_Close);

        typePtr->pointer.type = type;
        return ParseTypeSuffix(module, typePtr);
    }

    return type;
}
