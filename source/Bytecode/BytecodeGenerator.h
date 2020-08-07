#pragma once
#include <pch/Build.h>
#include "../Parser/Parser.h"

class BytecodeGenerator
{
public:
    BytecodeGenerator() { }

    bool Run(ModuleInfo& moduleInfo);
};