#pragma once
#include "pch/Build.h"
#include "Tree.h"

Statement* Statement::Create(Kind kind)
{
    Statement* stmt = new Statement();
    stmt->kind = kind;

    return stmt;
}

Compound* Statement::CreateCompound()
{
    Compound* compound = reinterpret_cast<Compound*>(Create(Statement::Kind::Compound));
    compound->statements.Init();

    return compound;
}

Type* Type::Create(Kind kind)
{
    Type* type = new Type();
    type->kind = kind;

    return type;
}

Type* Type::CreateBasic(u32 size, u32 alignment, bool isResolved, bool isSigned)
{
    Type* type = Create(Type::Kind::Basic);
    type->size = size;
    type->alignment = alignment;
    type->isResolved = isResolved;
    type->basic.isSigned = isSigned;

    return type;
}

Type* Type::CreatePointer()
{
    Type* type = Create(Type::Kind::Pointer);
    type->size = 8;
    type->alignment = 8;
    type->pointer.count = 0;

    return type;
}

Type* Type::CreateStruct()
{
    Type* type = Create(Type::Kind::Struct);
    type->structType.members.Init();

    return type;
}

Type* Type::CreateFunction()
{
    Type* type = Create(Type::Kind::Function);
    type->function.argumentTypes.Init();

    return type;
}

bool Type::IsSigned()
{
    switch (kind)
    {
        case Kind::Basic:
        {
            return basic.isSigned;
        }
        case Kind::Struct:
        {
            return false;
        }
        case Kind::Pointer:
        {
            return pointer.type->IsSigned();
        }

        default:
        {
            DebugHandler::PrintError("Type::IsSigned : Unhandled Type::Kind(%u)", kind);
            exit(1);
        }
    }
}

bool Type::IsBasic()
{
    return kind == Kind::Basic;;
}

bool Type::IsPointer()
{
    return kind == Kind::Pointer;
}

Expression* Expression::Create(Kind kind)
{
    Expression* expr = new Expression();
    expr->kind = kind;

    return expr;
}

Expression* Expression::CreateCall()
{
    Expression* expr = Create(Kind::Call);
    expr->call.arguments.Init();

    return expr;
}

Expression* Expression::CreatePrimary(Primary::Kind kind)
{
    Expression* expr = Create(Kind::Primary);
    expr->primary.kind = kind;
    expr->primary.declaration = nullptr;
    return expr;
}

Expression* Expression::CreateUnary(Unary::Kind kind)
{
    Expression* expr = Create(Kind::Unary);
    expr->unary.kind = kind;
    return expr;
}

Expression* Expression::CreateBinary(Binary::Kind kind)
{
    Expression* expr = Create(Kind::Binary);
    expr->binary.kind = kind;
    return expr;
}

u32 TreePrinter::identationLevel = 0;
bool TreePrinter::identationMask[MaxIndentationLevel] = { false };

const char* UnaryKindNames[] =
{
    "None",
    "Deref",
    "Address Of"
};

const char* Unary::GetKindName()
{
    assert(kind >= Kind::None && kind < Kind::Count);
    return UnaryKindNames[static_cast<u8>(kind)];
}

const char* BinaryKindNames[] =
{
    "None",
    "+",
    "-",
    "*",
    "/",
    "%",
    "==",
    "!=",
    "<",
    "<=",
    ">",
    ">=",
    "="
};

const char* Binary::GetKindName()
{
    assert(kind >= Kind::None && kind < Kind::Count);
    return BinaryKindNames[static_cast<u8>(kind)];
}

void TreePrinter::PrintStatement(Statement* statement)
{
    switch (statement->kind)
    {
        case Statement::Kind::Compound:
        {
            Compound* compound = &statement->compound;
            PrintIdentation("Compound: ");

            u32 compoundIdentation = identationLevel++;
            identationMask[compoundIdentation] = true;

            ListNode* node;
            ListIterate(&compound->statements, node)
            {
                Statement* stmt = ListGetStructPtr(node, Statement, listNode);

                if (node->next == &compound->statements)
                {
                    identationMask[compoundIdentation] = false;
                }

                PrintStatement(stmt);
            }

            identationLevel--;
            identationMask[compoundIdentation] = false;
            break;
        }
        case Statement::Kind::Comment:
        {
            Comment* comment = &statement->comment;
            PrintIdentation("Comment: ");

            identationLevel++;
            PrintComment(comment);
            identationLevel--;
            break;
        }
        case Statement::Kind::Expression:
        {
            Expression* expression = statement->expression;
            PrintIdentation("Expression: ");

            identationLevel++;
            PrintExpression(expression);
            identationLevel--;
            break;
        }
        case Statement::Kind::Return:
        {
            Return* ret = &statement->returnStmt;
            PrintIdentation("Return: ");

            identationLevel++;
            PrintReturn(ret);
            identationLevel--;
            break;
        }
        case Statement::Kind::Conditional:
        {
            Conditional* conditional = &statement->conditional;
            PrintIdentation("Conditional: ");

            identationLevel++;
            PrintConditional(conditional);
            identationLevel--;
            break;
        }
        case Statement::Kind::Loop:
        {
            Loop* loop = &statement->loop;
            PrintIdentation("Loop: ");

            identationLevel++;
            PrintLoop(loop);
            identationLevel--;
            break;
        }

        default:
        {
            DebugHandler::PrintError("TreePrinter : Unknown Statement::Kind(%u)", statement->kind);
            exit(1);
        }
    }
}

void TreePrinter::PrintComment(Comment* comment)
{
    const char* startPtr = comment->token->nameHash.name;

    u32 length = 0;
    while (true)
    {
        char c = startPtr[length];
        if (c == '\n')
            break;

        length++;
    }

    if (length == comment->token->nameHash.length)
    {
        PrintIdentation("%.*s", length, startPtr);
    }
    else
    {
        PrintIdentation("%.*s (...)", length, startPtr);
    }
}

void TreePrinter::PrintExpression(Expression* expression)
{
    switch (expression->kind)
    {
        case Expression::Kind::Primary:
        {
            Primary* primary = &expression->primary;

            if (primary->kind == Primary::Kind::Number)
            {
                PrintIdentation("Number: %u", primary->number);
            }
            else if (primary->kind == Primary::Kind::Identifier)
            {
                PrintIdentation("Identifier: %.*s", primary->token->nameHash.length, primary->token->nameHash.name);
            }
            else if (primary->kind == Primary::Kind::String)
            {
                PrintIdentation("String: %.*s", primary->token->nameHash.length, primary->token->nameHash.name);
            }
            else
            {
                DebugHandler::PrintError("TreePrinter : Unknown Primary::Kind(%u)", primary->kind);
                exit(1);
            }
            
            break;
        }    
        case Expression::Kind::Unary:
        {
            Unary* unary = &expression->unary;
            PrintIdentation("Unary: %s", unary->GetKindName());

            identationLevel++;
            PrintExpression(unary->operand);
            identationLevel--;

            break;
        }    
        case Expression::Kind::Binary:
        {
            Binary* binary = &expression->binary;
            PrintIdentation("Binary: %s", binary->GetKindName());

            u32 binaryIdentation = identationLevel++;
            {
                identationMask[binaryIdentation] = true;
                PrintExpression(binary->left);
                identationMask[binaryIdentation] = false;
                
                PrintExpression(binary->right);
            }

            identationLevel--;
            break;
        }    
        case Expression::Kind::Call:
        {
            Call* call = &expression->call;

            PrintIdentation("Call: %.*s", call->token->nameHash.length, call->token->nameHash.name);
            break;
        }
        case Expression::Kind::Dot:
        {
            Dot* dot = &expression->dot;
            PrintIdentation("Struct Access: %.*s", dot->tokenMember->nameHash.length, dot->tokenMember->nameHash.name);
            break;
        }

        default:
        {
            DebugHandler::PrintError("TreePrinter : Unknown Expression::Kind(%u)", expression->kind);
            exit(1);
        }
    }
}

void TreePrinter::PrintReturn(Return* ret)
{
    if (ret->expression)
    {
        PrintExpression(ret->expression);
    }
}

void TreePrinter::PrintConditional(Conditional* /*conditional*/)
{
}

void TreePrinter::PrintLoop(Loop* /*loop*/)
{
}