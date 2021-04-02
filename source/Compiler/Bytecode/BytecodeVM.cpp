#pragma once
#include <pch/Build.h>
#include "BytecodeVM.h"
#include "../Lexer/Lexer.h"

#include <Utils/DebugHandler.h>

BytecodeVM::BytecodeVM(uint32_t contextNum)
{        
    // Required Number of Contexts is 1
    assert(contextNum > 0);

    _contexts.resize(contextNum);

    for (BytecodeContext& context : _contexts)
    {
        context.Init();
    }

    //VMFunctionCall fnCall;
    //fnCall.Parse(this, "fn myFunc() -> i32", nullptr);
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

    std::string filePathStr = filePath.string();

    if (!fs::exists(filePath))
    {
        NC_LOG_ERROR("RunScript: Attempt to run non-existing script (%s)", filePathStr.c_str());
        return false;
    }

    _compiler.Start();
    _compiler.AddIncludePath({ "D:\\Development\\Nai\\bin\\DebugClang\\basic" });
    _compiler.AddPaths({ filePath });
    _compiler.Process();

    while (_compiler.GetStage() != Compiler::Stage::STOPPED)
    {
        std::this_thread::yield();
    }
    
    /*FileReader reader(filePathStr, filePath.filename().string());
    if (!reader.Fetch())
    {
        NC_LOG_ERROR("RunScript: Failed to read script (%s)", filePath.c_str());
        return false;
    }

    ModuleInfo& moduleInfo = _modules.emplace_back();
    if (!Compile(reader, moduleInfo))
    {
        _modules.pop_back();
        return false;
    }*/

    //size_t mainFnNameHash = StringUtils::hash_djb2("main", 4);
    //if (!Run(moduleInfo, mainFnNameHash))
        //return false;

    NC_LOG_SUCCESS("RunScript: Loaded (%s) successfully", filePath.filename().string().c_str());
    return true;
}

bool BytecodeVM::Compile(FileReader& reader, ModuleInfo& moduleInfo)
{
    ZoneScoped;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    moduleInfo.Init(reader.FileName(), reader.GetBuffer(), reader.Length());
    if (!Lexer::Process(moduleInfo))
        return false;

    /*if (!Parser::Process(moduleInfo))
        return false;*/

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Compiler Frontend Finished in %f seconds", time_span.count());
    /*if (!_parser.Run(moduleInfo))
        return false;

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Compiler Frontend Finished in %f seconds", time_span.count());

    t1 = std::chrono::high_resolution_clock::now();
    if (!_bytecodeGenerater.Run(moduleInfo))
        return false;

    t2 = std::chrono::high_resolution_clock::now();
    time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Compiler Backend Finished in %f seconds", time_span.count());*/
    return true;
}

bool BytecodeVM::Run(ModuleInfo& /*moduleInfo*/, size_t /*fnNameHash*/)
{
    ZoneScoped;
    /*ASTFunctionDecl* fnDecl = moduleInfo.GetFunctionByNameHash(fnNameHash);
    if (!fnDecl)
    {
        NC_LOG_ERROR("RunScript: Failed to find '%.*s' function in script (MODULE NAME HERE)", fnDecl->GetNameSize(), fnDecl->GetName());
        return false;
    }

    BytecodeContext& context = GetContext();

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    context.Prepare();
    bool didRunSuccessfully = context.RunInstructions(moduleInfo, fnDecl);

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto timeSpan = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    long long durationNS = timeSpan.count();
    double durationMicro = durationNS / 1000.f;
    double durationMilli = durationMicro / 1000.f;
    double durationSeconds = durationMilli / 1000.f;
    NC_LOG_SUCCESS("RunScript: Script Executed in (%.2u ns, %.2f us, %.4f ms, %.8f s)  (Errors: %u)", durationNS, durationMicro, durationMilli, durationSeconds, !didRunSuccessfully);

    return didRunSuccessfully;*/

    return true;
}