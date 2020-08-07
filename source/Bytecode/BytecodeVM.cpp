#pragma once
#include <pch/Build.h>
#include "BytecodeVM.h"
#include "../Lexer/Lexer.h"

#include <Utils/DebugHandler.h>
#include <Utils/FileReader.h>

BytecodeVM::BytecodeVM(uint16_t contextNum, uint16_t registryCount)
{        
    // Required Number of Contexts is 1, and we must always have at least 1 registry
    assert(contextNum > 0 && registryCount > 0);

    _contexts.resize(contextNum);

    for (BytecodeContext& context : _contexts)
    {
        context.Init(registryCount);
    }
}
BytecodeVM::~BytecodeVM()
{
    for (BytecodeContext& context : _contexts)
    {
        context.Destruct();
    }
}

bool BytecodeVM::RunScript(fs::path filePath)
{
    if (!fs::exists(filePath))
    {
        NC_LOG_ERROR("RunScript: Attempt to run script using non-existing path (%s)", filePath.string().c_str());
        return false;
    }

    std::vector<ByteInstruction*>* instructions = nullptr;
    if (!Compile(filePath, &instructions))
        return false;

    if (!Run(instructions))
        return false;

    NC_LOG_SUCCESS("RunScript: Loaded (%s) successfully", filePath.filename().string().c_str());
    return true;
}

bool BytecodeVM::Compile(fs::path& filePath, std::vector<ByteInstruction*>** output)
{
    std::string filePathStr = filePath.string();

    FileReader reader(filePathStr, filePath.filename().string());
    if (!reader.Fetch())
    {
        NC_LOG_ERROR("RunScript: Failed to open script (%s)", filePathStr.c_str());
        return false;
    }

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    LexerFile lexerFile(reader.GetBuffer(), reader.Length());
    if (!_lexer.Process(lexerFile))
        return false;

    ModuleInfo& moduleInfo = _modules.emplace_back(lexerFile);
    if (!_parser.Run(moduleInfo))
        return false;

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Compiler Frontend Finished in %f seconds", time_span.count());

    size_t mainFnNameHash = StringUtils::hash_djb2("main", 4);
    ASTFunctionDecl* fnDecl = moduleInfo.GetFunctionByNameHash(mainFnNameHash);
    if (!fnDecl)
    {
        NC_LOG_ERROR("RunScript: Failed to find 'main' function in script (%s)", filePathStr.c_str());
        return false;
    }

    t1 = std::chrono::high_resolution_clock::now();

    if (!_bytecodeGenerater.Run(moduleInfo))
        return false;

    t2 = std::chrono::high_resolution_clock::now();
    time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Compiler Backend Finished in %f seconds", time_span.count());

    *output = &fnDecl->GetInstructions();

    return true;
}

bool BytecodeVM::Run(std::vector<ByteInstruction*>* instructions)
{
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    BytecodeContext& context = GetContext();

    if (!context.RunInstructions(instructions))
        return false;

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Script Executed in %f seconds", time_span.count());

    return true;
}
