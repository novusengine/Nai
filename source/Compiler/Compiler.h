#pragma once
#include "pch/Build.h"
#include "robin_hood.h"
#include "Utils/BucketArray.h"

#include "Module.h"
#include "Frontend/Lexer.h"
#include "Frontend/Parser.h"
#include "Frontend/Typer.h"
#include "Backend/Bytecode/Bytecode.h"
#include "Backend/Bytecode/Interpreter.h"

struct Compiler
{
public:
    Compiler() : modules(1 * 1024 * 1024) { }

    BucketArray<Module> modules;
    robin_hood::unordered_map<u32, u32> modulePathHashToModuleIndex;
};