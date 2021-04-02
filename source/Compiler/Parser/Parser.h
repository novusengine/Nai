#pragma once
#include <pch/Build.h>
#include "../ModuleInfo.h"
#include "../Token.h"

class Compiler;
class Parser
{
public:
    //static bool Process(ModuleInfo& moduleInfo);
    static bool CheckSyntax(ModuleInfo& moduleInfo);
    static bool ResolveImports(Compiler* compiler, ModuleInfo& moduleInfo);
    static bool CreateAst(ModuleInfo& moduleInfo);
    static bool CheckSemantics(ModuleInfo& moduleInfo);

public:
    // Check Syntax Helper Functions
    static bool CheckSyntaxImport(ModuleInfo& moduleInfo, ModuleDefinition& definition, const int& numTokens);
    static bool CheckSyntaxAttribute(CompileUnit& compileUnit, const int& numTokens);
    static bool CheckSyntaxFunction(CompileUnit& compileUnit, const int& numTokens);
    static bool CheckSyntaxStruct(CompileUnit& compileUnit, const int& numTokens);
    static bool CheckSyntaxEnum(CompileUnit& compileUnit, const int& numTokens);

public:
    // Import Helper Functions
    static bool CheckModuleAtPath(Compiler* compiler, ModuleInfo& moduleInfo, ModuleImport& currentImport, fs::path& path);

public:
    // Ast Helper Functions
    static bool CreateFunctionAst(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex);
    static bool CreateFunctionHeaderAst(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex);
    static bool CreateFunctionBodyAst(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex);

    static bool GetExpressionFromTokenIndex(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex, AstExpression& expression);
    static bool GetDataTypeFromTokenIndex(CompileUnit& compileUnit, int& tokenIndex, AstDataType& dataType);
    static bool GetVariableFromTokenIndex(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex, AstVariable*& variable);
    static bool GetVariableDeclarationFromTokenIndex(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex, AstVariable*& variable);
    static bool GetReturnFromTokenIndex(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex, AstReturn*& ret);

public:
    // Semantics Helper Functions

private:
    Parser() { }
};