#pragma once
#include <pch/Build.h>
#include <mutex>
#include <vector>
#include <filesystem>
#include <Utils/FileReader.h>

#include "../Compiler.h"
#include "../ModuleInfo.h"
#include "../Lexer/Lexer.h"
#include "../Parser/Parser.h"
#include "BytecodeGenerator.h"
#include "BytecodeContext.h"

namespace fs = std::filesystem;

class BytecodeVM
{
public:
    BytecodeVM(uint32_t contextNum);
    ~BytecodeVM();

    bool RunScript(fs::path path);

    BytecodeContext& GetContext()
    {
        if (_contextIndex == _contexts.size())
            _contextIndex = 0;

        return _contexts[_contextIndex++];
    }

private:
    bool Compile(FileReader& reader, ModuleInfo& moduleInfo);
    bool Run(ModuleInfo& moduleInfo, size_t fnNameHash);

private:
    Compiler _compiler;
    std::atomic<uint16_t> _contextIndex = 0; // Round Robin when we handle GetContext
    std::vector<BytecodeContext> _contexts;

    BytecodeGenerator _bytecodeGenerater;
};