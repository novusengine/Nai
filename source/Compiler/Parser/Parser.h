#pragma once
#include <pch/Build.h>
#include "../ModuleInfo.h"
#include "../Token.h"

class Parser
{
public:
    static bool Process(ModuleInfo& moduleInfo);
    static bool CheckSyntax(ModuleInfo& moduleInfo);
    static bool CreateAST(ModuleInfo& moduleInfo);
    static bool CheckSemantics(ModuleInfo& moduleInfo);

public:
    // AST Helper Functions

public:
    // Semantics Helper Functions

private:
    Parser() { }
};