#include "pch/Build.h"
#include "Typer.h"
#include "../Module.h"

Type* Type_Char = Type::CreateBasic(1, 1, true, true);
Type* Type_Bool = Type::CreateBasic(1, 1, true, true);
Type* Type_U8 = Type::CreateBasic(1, 1, true, false);
Type* Type_U16 = Type::CreateBasic(2, 2, true, false);
Type* Type_U32 = Type::CreateBasic(4, 4, true, false);
Type* Type_U64 = Type::CreateBasic(8, 8, true, false);
Type* Type_I8 = Type::CreateBasic(1, 1, true, true);
Type* Type_I16 = Type::CreateBasic(2, 2, true, true);
Type* Type_I32 = Type::CreateBasic(4, 4, true, true);
Type* Type_I64 = Type::CreateBasic(8, 8, true, true);

void Typer::Process(Module* module)
{
    module->typerInfo.currentScope = nullptr;

    u32 passes = 0;

    while (module->typerInfo.typeResolved && module->typerInfo.unresolvedTypes)
    {
        module->typerInfo.typeResolved = false;
        module->typerInfo.unresolvedTypes = false;

        TypeScope(module, module->parserInfo.block->scope);

        passes++;
    }

    if (module->typerInfo.unresolvedTypes)
    {
        DebugHandler::PrintError("Typer : Failed to type Module (%s)", module->nameHash.name.c_str());
        exit(1);
    }

    DebugHandler::PrintSuccess("Typer : Successfully Processsed Module (%s) (Passes: %u)", module->nameHash.name.c_str(), passes);
}

void Typer::EnterScope(Module* module, Scope* scope)
{
    module->typerInfo.currentScope = scope;
}

void Typer::ExitScope(Module* module)
{
    assert(module->typerInfo.currentScope);
    module->typerInfo.currentScope = module->typerInfo.currentScope->parent;
}

bool Typer::IsValidType(Type* type)
{
    return type->kind != Type::Kind::Unknown;
}

bool Typer::IsPointer(Expression* expression)
{
    assert(expression->type);
    return expression->type->IsPointer();
}

Declaration* Typer::LookupInScope(Scope* scope, u32 nameHash)
{
    ListNode* node;
    ListIterate(&scope->declarations, node)
    {
        Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);

        if (nameHash == declaration->token->nameHash.hash)
            return declaration;
    }

    if (scope->parent)
    {
        return LookupInScope(scope->parent, nameHash);
    }

    return nullptr;
}

Declaration* Typer::LookupInCurrentScope(Module* module, u32 nameHash)
{
    return LookupInScope(module->typerInfo.currentScope, nameHash);
}

StructMember* Typer::LookupMemberInStruct(u32 nameHash, Type* type)
{
    assert(type->kind == Type::Kind::Struct);
    assert(type->structType.scope);

    ListNode* node;
    ListIterate(&type->structType.scope->members, node)
    {
        StructMember* member = ListGetStructPtr(node, StructMember, scopeNode);
        assert(!member->isAnonymous);

        if (nameHash == member->token->nameHash.hash)
            return member;
    }

    return nullptr;
}

void Typer::ResolvedType(Module* module)
{
    module->typerInfo.typeResolved = true;
}

void Typer::UnresolvedType(Module* module)
{
    module->typerInfo.unresolvedTypes = true;
}

u32 Typer::AlignNumber(u32 number, u32 alignment)
{
    u32 offset = number % alignment;
    if (offset)
    {
        return number + alignment - offset;
    }

    return number;
}

void Typer::ComputeStructOffsets(Type* type)
{
    assert(type->kind == Type::Kind::Struct);
    Struct* structType = &type->structType;

    u32 offset = 0;
    u32 alignment = 0;
    u32 size = 0;

    ListNode* node;
    ListIterate(&structType->members, node)
    {
        StructMember* member = ListGetStructPtr(node, StructMember, listNode);

        if (member->type->kind == Type::Kind::Struct)
        {
            ComputeStructOffsets(member->type);
        }

        assert(member->type);
        assert(member->type->size);

        if (structType->isStruct)
        {
            offset = AlignNumber(offset, member->type->size);
            member->offset = offset;

            size += member->type->size;
            offset += member->type->size;
        }
        else
        {
            // Union
            member->offset = 0;

            if (member->type->size > size)
            {
                size = member->type->size;
            }
        }

        if (member->type->alignment > alignment)
        {
            alignment = member->type->alignment;
        }
    }

    type->alignment = alignment;
    type->size = AlignNumber(size, alignment);
}

void Typer::FixStructOffsets(Type* type, u32 offset)
{
    assert(type->kind == Type::Kind::Struct);
    Struct* structType = &type->structType;

    if (structType->scope)
    {
        offset = 0;
    }

    ListNode* node;
    ListIterate(&structType->members, node)
    {
        StructMember* member = ListGetStructPtr(node, StructMember, listNode);

        member->offset += offset;
        if (member->type->kind == Type::Kind::Struct)
        {
            FixStructOffsets(member->type, member->offset);
        }
    }
}

void Typer::TypeScope(Module* module, Scope* scope)
{
    EnterScope(module, scope);

    ListNode* node;
    ListIterate(&scope->declarations, node)
    {
        Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);

        if (declaration->kind == Declaration::Kind::Type ||
            declaration->kind == Declaration::Kind::Variable)
        {
            ResolveTypeDeclaration(module, declaration);
        }
        else if (declaration->kind == Declaration::Kind::Function)
        {
            if (!declaration->function.returnType)
            {
                declaration->function.returnType = Type::Create(Type::Kind::Void);
            }

            ResolveTypeFunction(module, declaration);
        }
    }

    ExitScope(module);
}

Type* Typer::ResolveType(Module* module, Type* type)
{
    if (type->isResolved)
        return type;

    switch (type->kind)
    {
        case Type::Kind::Basic:
        {
            return type;
        }
        case Type::Kind::Pointer:
        {
            if (type->pointer.type->kind == Type::Kind::Void)
            {
                type->isResolved = true;
                return type;
            }

            type->pointer.type = ResolveType(module, type->pointer.type);

            if (type->pointer.type->isResolved)
                type->isResolved = true;

            return type;
        }

        case Type::Kind::Struct:
        {
            return ResolveTypeStruct(module, type);
        }

        case Type::Kind::Unknown:
        {
            return ResolveTypeUnknown(module, type);
        }

        case Type::Kind::Void:
        {
            type->isResolved = true;
            return type;
        }

        case Type::Kind::Function:
        {
            ListNode* node;
            ListIterate(&type->function.argumentTypes, node)
            {
                Type* argumentType = ListGetStructPtr(node, Type, listNode);
                Type* newType = ResolveType(module, argumentType);

                newType->listNode.next = argumentType->listNode.next;
                newType->listNode.prev = argumentType->listNode.prev;

                argumentType->listNode.next->prev = &newType->listNode;
                argumentType->listNode.prev->next = &newType->listNode;
            }

            return type;
        }

        default:
        {
            DebugHandler::PrintError("Typer : Unknown Type::Kind(%u)", type->kind);
            exit(1);
        }

    }
}

Type* Typer::ResolveTypeStruct(Module* module, Type* type)
{
    assert(type->kind == Type::Kind::Struct);
    assert(type->structType.scope);

    StructScope* scope = type->structType.scope;
    type->isResolved = true;

    ListNode* node;
    ListIterate(&scope->members, node)
    {
        StructMember* member = ListGetStructPtr(node, StructMember, scopeNode);
        if (member->isAnonymous && member->type->isResolved)
            continue;

        member->type = ResolveType(module, member->type);

        if (!member->type->isResolved)
            type->isResolved = false;
    }

    return type;
}

Type* Typer::ResolveTypeUnknown(Module* module, Type* type)
{
    assert(type->kind == Type::Kind::Unknown);

    Declaration* declaration = LookupInCurrentScope(module, type->unknown.token->nameHash.hash);
    if (!declaration || declaration->kind != Declaration::Kind::Type)
    {
        module->lexerInfo.Error(type->unknown.token, "This type is undeclared");
    }

    if (IsValidType(declaration->type))
    {
        ResolvedType(module);
        return declaration->type;
    }
    else
    {
        UnresolvedType(module);
        return type;
    }
}

void Typer::ResolveTypeDeclaration(Module* module, Declaration* declaration)
{
    if (declaration->type->isResolved)
        return;

    declaration->type = ResolveType(module, declaration->type);

    if (declaration->type->kind == Type::Kind::Struct && declaration->type->isResolved)
    {
        ComputeStructOffsets(declaration->type);
        FixStructOffsets(declaration->type, 0);
    }
}

void Typer::ResolveTypeFunction(Module* module, Declaration* declaration)
{
    assert(declaration->kind == Declaration::Kind::Function);
    Function* function = &declaration->function;

    TypeScope(module, function->scope);
    EnterScope(module, function->scope);
    declaration->type = ResolveType(module, declaration->type);

    assert(function->scope != function->body->compound.scope);
    TypeScope(module, function->body->compound.scope);
    ResolveTypeStatement(module, function->body);

    function->returnType = ResolveType(module, function->returnType);

    ExitScope(module);
}

void Typer::ResolveTypeStatement(Module* module, Statement* statement)
{
    switch (statement->kind)
    {
        case Statement::Kind::Compound:
        {
            ResolveTypeStatementCompound(module, statement);
            break;
        }
        case Statement::Kind::Comment:
        {
            break;
        }
        case Statement::Kind::Expression:
        {
            ResolveTypeStatementExpression(module, statement);
            break;
        }
        case Statement::Kind::Return:
        {
            ResolveTypeStatementReturn(module, statement);
            break;
        }
        case Statement::Kind::Conditional:
        {
            ResolveTypeStatementConditional(module, statement);
            break;
        }
        case Statement::Kind::Loop:
        {
            ResolveTypeStatementLoop(module, statement);
            break;
        }
        case Statement::Kind::Continue:
        case Statement::Kind::Break:
        {
            break;
        }

        default:
        {
            DebugHandler::PrintError("Typer : Unknown Statement::Kind(%u)", statement->kind);
            exit(1);
        }
    }
}

void Typer::ResolveTypeStatementCompound(Module* module, Statement* statement)
{
    assert(statement->kind == Statement::Kind::Compound);
    Compound* compound = &statement->compound;

    EnterScope(module, compound->scope);

    ListNode* node;
    ListIterate(&compound->statements, node)
    {
        Statement* stmt = ListGetStructPtr(node, Statement, listNode);
        ResolveTypeStatement(module, stmt);
    }

    ExitScope(module);
}

void Typer::ResolveTypeStatementExpression(Module* module, Statement* statement)
{
    assert(statement->kind == Statement::Kind::Expression);

    Expression* expression = statement->expression;
    ResolveTypeExpression(module, expression);
}

void Typer::ResolveTypeStatementReturn(Module* module, Statement* statement)
{
    assert(statement->kind == Statement::Kind::Return);

    Return* returnStmt = &statement->returnStmt;
    if (returnStmt->expression)
    {
        ResolveTypeExpression(module, returnStmt->expression);
    }
}

void Typer::ResolveTypeStatementConditional(Module* module, Statement* statement)
{
    assert(statement->kind == Statement::Kind::Conditional);
    Conditional* conditional = &statement->conditional;

    ResolveTypeExpression(module, conditional->condition);
    ResolveTypeStatement(module, conditional->trueBody);

    if (conditional->falseBody)
    {
        ResolveTypeStatement(module, conditional->falseBody);
    }
}

void Typer::ResolveTypeStatementLoop(Module* module, Statement* statement)
{
    assert(statement->kind == Statement::Kind::Loop);
    Loop* loop = &statement->loop;

    EnterScope(module, loop->body->compound.scope);
    ResolveTypeExpression(module, loop->condition);
    ExitScope(module);

    ResolveTypeStatement(module, loop->body);
}

void Typer::ResolveTypeExpression(Module* module, Expression* expression)
{
    switch (expression->kind)
    {
        case Expression::Kind::Primary:
        {
            ResolveTypeExpressionPrimary(module, expression);
            break;
        }
        case Expression::Kind::Unary:
        {
            ResolveTypeExpressionUnary(module, expression);
            break;
        }
        case Expression::Kind::Binary:
        {
            ResolveTypeExpressionBinary(module, expression);
            break;
        }
        case Expression::Kind::Call:
        {
            ResolveTypeExpressionCall(module, expression);
            break;
        }
        case Expression::Kind::Dot:
        {
            ResolveTypeExpressionDot(module, expression);
            break;
        }

        case Expression::Kind::MemoryNew:
        case Expression::Kind::MemoryFree:
        {
            ResolveTypeExpressionMemory(module, expression);
            break;
        }

        default:
        {
            DebugHandler::PrintError("Typer : Unknown Expression::Kind(%u)", expression->kind);
            exit(1);
        }
    }
}

void Typer::ResolveTypeExpressionPrimary(Module* module, Expression* expression)
{
    if (expression->type)
        return;

    Primary* primary = &expression->primary;

    if (primary->kind == Primary::Kind::Identifier)
    {
        if (primary->declaration == nullptr)
        {
            Declaration* decl = LookupInCurrentScope(module, primary->token->nameHash.hash);
            if (!decl)
            {
                module->lexerInfo.Error(primary->token, "Name is undeclared (%.*s)", primary->token->nameHash.length, primary->token->nameHash.name);
            }

            primary->declaration = decl;
        }

        if (IsValidType(primary->declaration->type))
        {
            expression->type = primary->declaration->type;
            ResolvedType(module);
        }
        else
        {
            UnresolvedType(module);
        }
    }
    else if (primary->kind == Primary::Kind::Number)
    {
        expression->type = Type_U64;
        ResolvedType(module);
    }
    else if (primary->kind == Primary::Kind::String)
    {
        expression->type = Type::CreatePointer();
        expression->type->pointer.type = Type_Char;
        ResolvedType(module);
    }
}

void Typer::ResolveTypeExpressionUnary(Module* module, Expression* expression)
{
    if (expression->type)
        return;

    Unary* unary = &expression->unary;
    ResolveTypeExpression(module, unary->operand);

    if (!unary->operand->type)
    {
        UnresolvedType(module);
        return;
    }

    if (unary->kind == Unary::Kind::AddressOf)
    {
        expression->type = Type::CreatePointer();
        expression->type->pointer.type = unary->operand->type;

        ResolvedType(module);
    }
    else if (unary->kind == Unary::Kind::Deref)
    {
        Type* type = unary->operand->type;

        if (type->kind != Type::Kind::Pointer)
        {
            module->lexerInfo.Error(unary->op, "You cannot deref a non-pointer object");
        }

        if (type->pointer.type->kind == Type::Kind::Void)
        {
            module->lexerInfo.Error(unary->op, "You cannot deref a void pointer");
        }

        expression->type = type->pointer.type;
        ResolvedType(module);
    }
}

void Typer::ResolveTypeExpressionBinary(Module* module, Expression* expression)
{
    if (expression->type)
        return;

    Binary* binary = &expression->binary;

    ResolveTypeExpression(module, binary->right);
    ResolveTypeExpression(module, binary->left);

    Type* leftType = binary->left->type;
    Type* rightType = binary->right->type;

    if (!leftType || !rightType)
        return;

    expression->type = binary->left->type;
    ResolvedType(module);

    // Check for pointer arithmetic, if so normalize so the pointer is on the left
    if (binary->kind == Binary::Kind::Add ||
        binary->kind == Binary::Kind::Subtract)
    {
        if (!IsPointer(binary->left) && IsPointer(binary->right))
        {
            Expression* temp = binary->left;

            binary->left = binary->right;
            binary->right = temp;

            rightType = binary->right->type;
            leftType = binary->left->type;
        }
    }

    if (IsPointer(binary->left))
    {
        if (binary->kind == Binary::Kind::Add)
        {
            if (IsPointer(binary->right))
            {
                module->lexerInfo.Error(binary->op, "You cannot add 2 pointers");
            }

            Expression* primaryExpr = Expression::CreatePrimary(Primary::Kind::Number);
            primaryExpr->primary.number = binary->left->type->pointer.type->size;
            assert(primaryExpr->primary.number);

            Expression* binaryMul = Expression::CreateBinary(Binary::Kind::Multiply);
            binaryMul->binary.op = binary->op;
            binaryMul->binary.left = binary->right;
            binaryMul->binary.right = primaryExpr;

            binary->right = binaryMul;

            ResolveTypeExpression(module, primaryExpr);
            ResolveTypeExpression(module, binaryMul);
        }

        if (binary->kind == Binary::Kind::Subtract)
        {
            module->lexerInfo.Error(binary->op, "Pointer subtraction is unsupported");
        }

        expression->type = binary->left->type;
    }
}

void Typer::ResolveTypeExpressionCall(Module* module, Expression* expression)
{
    if (expression->type)
        return;

    Call* call = &expression->call;
    ResolveTypeExpression(module, call->expression);

    if (!call->expression->type)
    {
        UnresolvedType(module);
        return;
    }

    ListNode* node;
    ListIterate(&call->arguments, node)
    {
        Expression* expr = ListGetStructPtr(node, Expression, listNode);
        ResolveTypeExpression(module, expr);
    }

    assert(call->expression->kind == Expression::Kind::Primary);
    assert(call->expression->primary.kind == Primary::Kind::Identifier);

    Declaration* decl = LookupInCurrentScope(module, call->expression->primary.token->nameHash.hash);
    if (decl)
    {
        expression->type = decl->function.returnType;
        ResolvedType(module);
    }
    else
    {
        module->lexerInfo.Error(call->token, "Function(%.*s) is not declared", call->token->nameHash.length, call->token->nameHash.name);
    }
}

void Typer::ResolveTypeExpressionDot(Module* module, Expression* expression)
{
    if (expression->type)
        return;

    Dot* dot = &expression->dot;
    if (!dot->expression->type)
    {
        ResolveTypeExpression(module, dot->expression);
    }

    if (dot->expression->type)
    {
        while (dot->expression->type->kind == Type::Kind::Pointer)
        {
            Type* type = dot->expression->type->pointer.type;

            if (type->kind == Type::Kind::Void)
            {
                module->lexerInfo.Error(dot->token, "You cannot access a member from a void pointer");
            }

            Expression* unaryExpr = Expression::CreateUnary(Unary::Kind::Deref);
            unaryExpr->unary.operand = dot->expression;

            dot->expression = unaryExpr;
            dot->expression->type = type;

            ResolveTypeExpressionUnary(module, unaryExpr);
        }

        if (dot->expression->type->kind != Type::Kind::Struct)
        {
            module->lexerInfo.Error(dot->token, "You cannot access a member from a non-struct object");
        }

        StructMember* member = LookupMemberInStruct(dot->tokenMember->nameHash.hash, dot->expression->type);
        if (!member)
        {
            module->lexerInfo.Error(dot->tokenMember, "%.*s is not a struct member", dot->tokenMember->nameHash.length, dot->tokenMember->nameHash.name);
        }

        dot->offset = member->offset;
        expression->type = member->type;
        ResolvedType(module);
    }
}

void Typer::ResolveTypeExpressionMemory(Module* module, Expression* expression)
{
    bool isMemoryNew = expression->kind == Expression::Kind::MemoryNew;

    if (!expression->type)
    {
        if (isMemoryNew)
        {
            expression->type = Type_U64;
        }
        // MemoryFree does not return a value
        else
        {
            expression->type = Type::Create(Type::Kind::Void);
        }
    }

    MemoryExpression* memory = &expression->memory;
    if (memory->expression->type)
        return;

    ResolveTypeExpression(module, memory->expression);

    if (!memory->expression->type)
    {
        UnresolvedType(module);
        return;
    }

    Type* type = memory->expression->type;
    if (isMemoryNew)
    {
        if (!type->IsBasic())
        {
            module->lexerInfo.Error(memory->token, "(%.*s) expects an integer value", memory->token->nameHash.length, memory->token->nameHash.name);
        }
    }
    else
    {
        if (!type->IsBasic() && !type->IsPointer())
        {
            module->lexerInfo.Error(memory->token, "(%.*s) expects a pointer or integer value", memory->token->nameHash.length, memory->token->nameHash.name);
        }
    }
}
