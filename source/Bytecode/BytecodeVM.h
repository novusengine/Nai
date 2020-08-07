#pragma once
#include <pch/Build.h>
#include <mutex>
#include <vector>
#include <filesystem>

#include "../Lexer/Lexer.h"
#include "../Parser/Parser.h"
#include "BytecodeGenerator.h"
#include "BytecodeContext.h"

namespace fs = std::filesystem;

class BytecodeVM
{
public:
    BytecodeVM(uint16_t contextNum, uint16_t registryCount);
    ~BytecodeVM();

    bool RunScript(fs::path path);

    BytecodeContext& GetContext()
    {
        _contextMutex.lock();

        BytecodeContext& context = _contexts[_contextIndex++];

        if (_contextIndex == _contexts.size())
            _contextIndex = 0;

        _contextMutex.unlock();

        return context;
    }

private:
    bool Compile(fs::path& filePath, std::vector<ByteInstruction*>** output);
    bool Run(std::vector<ByteInstruction*>* instructions);

private:
    uint16_t _contextIndex = 0; // Round Robin when we handle GetContext
    std::mutex _contextMutex;
    std::vector<BytecodeContext> _contexts;
    std::vector<ModuleInfo> _modules;

    Lexer _lexer;

    Parser _parser;
    BytecodeGenerator _bytecodeGenerater;
};