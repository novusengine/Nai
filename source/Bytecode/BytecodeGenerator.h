#pragma once
#include <pch/Build.h>
#include "../Parser/Parser.h"

class BytecodeGenerator
{
public:
    BytecodeGenerator() { }

    bool Run(ModuleInfo& moduleInfo);

    uint16_t GetRegisterNumForFunction(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl);

    void Generate_Func(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl);
    void Generate_FuncHeader(ASTFunctionDecl* fnDecl);
    void Generate_FuncBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl);
    void Generate_FuncCall(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTFunctionCall* fnCall);

    void Generate_Variable(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTVariable* var);
    uint16_t Generate_Expression(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTExpression* expression);
    uint16_t Generate_ExpressionNode(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTNode* node);
    uint64_t GetExpressionValueFromNode(ASTNode* node);

    void Generate_ReturnStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTReturnStatement* returnStmt);

    void Generate_IfStmtChain(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTIfStatement* stmt);
    void Generate_IfStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, std::vector<uint64_t*>* escapeJmpPtrs, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTIfStatement* stmt);
    void Generate_IfHead(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint64_t** nextInstructionIndex, ASTExpression* expression);
    void Generate_IfBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, std::vector<uint64_t*>* escapeJmpPtrs, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTIfStatement* stmt);

    void Generate_WhileStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTWhileStatement* stmt);
    void Generate_LoopHead(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, std::vector<uint64_t*>& breakJmpPtrs, ASTExpression* expression);
    void Generate_LoopBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, size_t loopStartIndex, std::vector<uint64_t*>& breakJmpPtrs, ASTSequence* body);
};