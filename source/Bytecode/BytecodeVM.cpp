#pragma once
#include <pch/Build.h>
#include "BytecodeVM.h"
#include "../Lexer/Lexer.h"

#include <Utils/DebugHandler.h>

BytecodeVM::BytecodeVM(uint32_t contextNum)
{        
    // Required Number of Contexts is 1, and we must always have at least 1 registry
    assert(contextNum > 0 && NAI_BYTECODE_STACK_SIZE > 0);

    _contexts.resize(contextNum);

    for (BytecodeContext& context : _contexts)
    {
        context.Init(NAI_BYTECODE_STACK_SIZE);
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
        NC_LOG_ERROR("RunScript: Attempt to run script using non-existing path (%s)", filePathStr.c_str());
        return false;
    }
    
    FileReader reader(filePathStr, filePath.filename().string());
    if (!reader.Fetch())
    {
        NC_LOG_ERROR("RunScript: Failed to open script (%s)", filePath.c_str());
        return false;
    }

    ModuleInfo& moduleInfo = _modules.emplace_back();
    if (!Compile(reader, moduleInfo))
    {
        _modules.pop_back();
        return false;
    }

    size_t mainFnNameHash = StringUtils::hash_djb2("main", 4);
    if (!Run(moduleInfo, mainFnNameHash))
        return false;

    NC_LOG_SUCCESS("RunScript: Loaded (%s) successfully", filePath.filename().string().c_str());
    return true;
}

bool BytecodeVM::Compile(FileReader& reader, ModuleInfo& moduleInfo)
{
    ZoneScoped;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    LexerFile& lexerFile = moduleInfo.GetLexerFile();
    lexerFile.Init(reader.GetBuffer(), reader.Length());
    if (!_lexer.Process(lexerFile))
        return false;

    if (!_parser.Run(moduleInfo))
        return false;

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Compiler Frontend Finished in %f seconds", time_span.count());

    t1 = std::chrono::high_resolution_clock::now();
    if (!_bytecodeGenerater.Run(moduleInfo))
        return false;

    t2 = std::chrono::high_resolution_clock::now();
    time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Compiler Backend Finished in %f seconds", time_span.count());
    return true;
}

bool BytecodeVM::Run(ModuleInfo& moduleInfo, size_t fnNameHash)
{
    ZoneScoped;
    ASTFunctionDecl* fnDecl = moduleInfo.GetFunctionByNameHash(fnNameHash);
    if (!fnDecl)
    {
        NC_LOG_ERROR("RunScript: Failed to find '%.*s' function in script (MODULE NAME HERE)", fnDecl->GetNameSize(), fnDecl->GetName());
        return false;
    }

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    BytecodeContext& context = GetContext();

    if (!context.Prepare())
        return false;

    if (!context.RunInstructions(moduleInfo, fnDecl->GetInstructions()))
        return false;

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    NC_LOG_SUCCESS("RunScript: Script Executed in %f seconds", timeSpan.count());

    return true;
}

bool VMFunctionCall::Parse(BytecodeVM* vm, std::string signature, void* inCallback)
{
    _lexerFile.Init(signature.data(), signature.size());

    if (!vm->GetLexer().Process(_lexerFile))
        return false;

    auto& tokens = _lexerFile.GetTokens();

    size_t tokenOffset = 0;
    const Token& fnKeyword = tokens[tokenOffset++];
    if (fnKeyword.type != TokenType::KEYWORD && fnKeyword.subType != TokenSubType::KEYWORD_FUNCTION)
        return false;

    const Token& fnDecl = tokens[tokenOffset++];
    if (fnDecl.type != TokenType::IDENTIFIER && fnDecl.subType != TokenSubType::FUNCTION_DECLARATION)
        return false;

    const Token& lParen = tokens[tokenOffset++];
    if (lParen.type != TokenType::LPAREN)
        return false;

    // Parse Parameter List here

    const Token& rParen = tokens[tokenOffset++];
    if (rParen.type != TokenType::RPAREN)
        return false;

    if (tokenOffset < tokens.size())
    {
        const Token& returnTypeOperator = tokens[tokenOffset++];
        if (returnTypeOperator.type != TokenType::OPERATOR && returnTypeOperator.subType != TokenSubType::OP_RETURN_TYPE)
            return false;

        const Token& returnDataType = tokens[tokenOffset++];
        if (returnDataType.type != TokenType::DATATYPE)
            return false;

        returnData = Parser::GetTypeFromChar(returnDataType.value, returnDataType.valueSize);

        // TODO: How do we handle custom types later?
        if (returnData == NaiType::CUSTOM)
            return false;
    }

    if (tokenOffset != tokens.size())
        return false;

    // Setup Data
    name = std::string_view(fnDecl.value, fnDecl.valueSize);

    callback = inCallback;
    return true;
}
