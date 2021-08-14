#pragma once
#include "pch/Build.h"
#include "../Tree.h"

extern Type* Type_Char;
extern Type* Type_Bool;
extern Type* Type_U8;
extern Type* Type_U16;
extern Type* Type_U32;
extern Type* Type_U64;
extern Type* Type_I8;
extern Type* Type_I16;
extern Type* Type_I32;
extern Type* Type_I64;

struct Module;
struct Scope;
struct Type;
struct Declaration;
struct Statement;
struct StructMember;

class Typer
{
public:
    static void Process(Module* module);

private:
    static void EnterScope(Module* module, Scope* scope);
    static void ExitScope(Module* module);
    static bool IsValidType(Type* type);
    static bool IsPointer(Expression* expression);
    static Declaration* LookupInScope(Scope* scope, u32);
    static Declaration* LookupInCurrentScope(Module* module, u32 nameHash);
    static StructMember* LookupMemberInStruct(u32 nameHash, Type* type);
    static void ResolvedType(Module* module);
    static void UnresolvedType(Module* module);

    static u32 AlignNumber(u32 number, u32 alignment);
    static void ComputeStructOffsets(Type* type);
    static void FixStructOffsets(Type* type, u32 offset);

    static void TypeScope(Module* module, Scope* scope);
    static Type* ResolveType(Module* module, Type* type);
    static Type* ResolveTypeStruct(Module* module, Type* type);
    static Type* ResolveTypeUnknown(Module* module, Type* type);
    static void ResolveTypeDeclaration(Module* module, Declaration* declaration);
    static void ResolveTypeFunction(Module* module, Declaration* declaration);
    static void ResolveTypeStatement(Module* module, Statement* statement);
    static void ResolveTypeStatementCompound(Module* module, Statement* statement);
    static void ResolveTypeStatementExpression(Module* module, Statement* statement);
    static void ResolveTypeStatementReturn(Module* module, Statement* statement);
    static void ResolveTypeStatementConditional(Module* module, Statement* statement);
    static void ResolveTypeStatementLoop(Module* module, Statement* statement);
    static void ResolveTypeExpression(Module* module, Expression* expression);
    static void ResolveTypeExpressionPrimary(Module* module, Expression* expression);
    static void ResolveTypeExpressionUnary(Module* module, Expression* expression);
    static void ResolveTypeExpressionBinary(Module* module, Expression* expression);
    static void ResolveTypeExpressionCall(Module* module, Expression* expression);
    static void ResolveTypeExpressionDot(Module* module, Expression* expression);
    static void ResolveTypeExpressionMemory(Module* module, Expression* expression);
};