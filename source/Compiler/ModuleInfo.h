#pragma once
#include <pch/Build.h>
#include <vector>

#include "Token.h"
#include "../Memory/BlockAllocator.h"

struct CompileUnitAttributes
{
    std::string_view name = "Unnamed";
    bool parseResult = true;
};

struct CompileUnit
{
    enum class Type : uint8_t
    {
        NONE,
        ENUM,
        STRUCT,
        FUNCTION
    };

    Type type = Type::NONE;

    std::string_view name;
    const char* startPtr;
    const char* endPtr;

    int startLineNum;
    int startColumnNum;

    CompileUnitAttributes attributes;
    std::vector<Token> tokens;
};

struct ModuleInfo
{
    ModuleInfo() { }

    void Init(std::string moduleName, char* inFileBuffer, size_t inFileBufferSize)
    {
        name = moduleName;
        fileBuffer = inFileBuffer;
        fileBufferSize = static_cast<long>(inFileBufferSize);
    }

    std::string name;

    char* fileBuffer = nullptr;
    long fileBufferSize = 0;

    std::vector<CompileUnit> compileUnits;
};