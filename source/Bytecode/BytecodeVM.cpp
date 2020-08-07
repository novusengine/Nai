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
    ZoneScoped;

    if (!fs::exists(filePath))
    {
        NC_LOG_ERROR("RunScript: Attempt to run script using non-existing path (%s)", filePath.string().c_str());
        return false;
    }

    ASTFunctionDecl* fnDecl = nullptr;
    if (!Compile(filePath, fnDecl))
        return false;

    if (!Run(fnDecl))
        return false;

    NC_LOG_SUCCESS("RunScript: Loaded (%s) successfully", filePath.filename().string().c_str());
    return true;
}

bool BytecodeVM::Compile(fs::path& filePath, ASTFunctionDecl*& mainFnDecl)
{
    ZoneScoped;

    std::string filePathStr = filePath.string();

    FileReader reader(filePathStr, filePath.filename().string());
    if (!reader.Fetch())
    {
        NC_LOG_ERROR("RunScript: Failed to open script (%s)", filePathStr.c_str());
        return false;
    }

    LexerFile lexerFile(reader.GetBuffer(), reader.Length());
    if (!_lexer.Process(lexerFile))
        return false;

    ModuleInfo& moduleInfo = _modules.emplace_back(lexerFile);
    if (!_parser.Run(moduleInfo))
        return false;

    size_t mainFnNameHash = StringUtils::hash_djb2("main", 4);
    ASTFunctionDecl* fnDecl = moduleInfo.GetFunctionByNameHash(mainFnNameHash);
    if (!fnDecl)
    {
        NC_LOG_ERROR("RunScript: Failed to find 'main' function in script (%s)", filePathStr.c_str());
        return false;
    }

    if (!_bytecodeGenerater.Run(moduleInfo))
        return false;

    mainFnDecl = fnDecl;
    return true;
}

bool BytecodeVM::Run(ASTFunctionDecl* fnDecl)
{
    ZoneScoped;
    BytecodeContext& context = GetContext();

    if (!context.Prepare())
        return false;

    if (!context.RunInstructions(fnDecl->GetInstructions()))
        return false;

    return true;
}
