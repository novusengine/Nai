#pragma once
#include <pch/Build.h>
#include <mutex>
#include <vector>
#include <filesystem>
#include <Utils/FileReader.h>

#include "../Lexer/Lexer.h"
#include "../Parser/Parser.h"
#include "BytecodeGenerator.h"
#include "BytecodeContext.h"

namespace fs = std::filesystem;

class BytecodeVM;
struct VMFunctionCall
{
public:
    bool Parse(BytecodeVM* vm, std::string signature, void* inCallback);

public:
    std::string_view name;

    NaiType returnData;
    std::vector<NaiType> arguments;
    
    void* callback = nullptr;

private:
    LexerFile _lexerFile;
};

class BytecodeVM
{
public:
    BytecodeVM(uint32_t contextNum);
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
    bool Compile(FileReader& reader, ModuleInfo& moduleInfo);
    bool Run(ModuleInfo& moduleInfo, size_t fnNameHash);

    friend struct VMFunctionCall;
    Lexer& GetLexer() { return _lexer; }
private:
    uint16_t _contextIndex = 0; // Round Robin when we handle GetContext
    std::mutex _contextMutex;
    std::vector<BytecodeContext> _contexts;
    std::vector<ModuleInfo> _modules;

    Lexer _lexer;

    Parser _parser;
    BytecodeGenerator _bytecodeGenerater;
};