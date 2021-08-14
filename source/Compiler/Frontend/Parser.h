#pragma once
#include "pch/Build.h"

struct Module;
struct Type;
struct Statement;
struct Scope;
struct StructScope;
struct StructMember;
struct Declaration;
struct Expression;
struct Loop;

class Parser
{
public:
    static void Process(Module* module);

private:
    friend struct NativeFunction;

    static Statement* ParseBlock(Module* module);
    static Scope* EnterScope(Module* module);
    static void ExitScope(Module* module);
    static StructScope* EnterStructScope(Module* module);
    static void ExitStructScope(Module* module);
    static void EnterLoop(Module* module, Loop* loop);
    static void ExitLoop(Module* module);

    static Statement* ParseDeclarationOrStatement(Module* module);
    static bool TryParseDeclaration(Module* module, Statement*& stmt);
    static Statement* ParseStatement(Module* module);

    static Type* ParseStructDeclaration(Module* module, bool isAnonymous);
    static StructMember* ParseStructMember(Module* module);
    static bool StructHasMember(Module* module, StructMember* member);
    static void StructPushMemberToScope(Module* module, StructMember* member);
    static void DeclarationPushToScope(Module* module, Declaration* declaration, Scope* scope);
    static void DeclarationPushToCurrentScope(Module* module, Declaration* declaration);
    static Declaration* FindDeclarationInScope(Declaration* declaration, Scope* scope);
    static void ParseFunctionArgument(Module* module);
    static Statement* ParseStatementLoop(Module* module);
    static Statement* ParseStatementConditional(Module* module);
    static Statement* ParseStatementExpression(Module* module);

    static Statement* ParseStatementCompound(Module* module);
    static Expression* ParseExpression(Module* module, i8 priority);
    static Expression* ParseExpressionUnary(Module* module);
    static Expression* ParseExpressionSuffix(Module* module, Expression* prev);
    static Expression* ParseExpressionPrimary(Module* module);
    static Expression* ParseExpressionMemory(Module* module);

    static Type* ParseType(Module* module);
    static Type* ParseTypeSuffix(Module* module, Type* type);
};