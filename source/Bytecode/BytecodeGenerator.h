#pragma once
#include <pch/Build.h>
#include "../Parser/Parser.h"

class BytecodeGenerator
{
public:
    BytecodeGenerator() { }

    bool Run(ModuleInfo& moduleInfo);

    void Generate_FuncHeader(ASTFunctionDecl* fnDecl, uint16_t& registerNum);
    void Generate_FuncBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum);

    void Generate_Variable(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTVariable* var);
    void Generate_Expression(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, ASTExpression* expression);
    uint64_t GetExpressionValueFromNode(ASTNode* node);

    void Generate_ReturnStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTReturnStatement* returnStmt);

    void Generate_IfStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTIfStatement* stmt);
    void Generate_IfHead(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, uint64_t** nextInstructionIndex, ASTExpression* expression);
    void Generate_IfBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTSequence* body);

    void Generate_WhileStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, ASTWhileStatement* stmt);
    void Generate_LoopHead(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, std::vector<uint64_t*>& breakJmpPtrs, ASTExpression* expression);
    void Generate_LoopBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, size_t loopStartIndex, std::vector<uint64_t*>& breakJmpPtrs, ASTSequence* body);
};