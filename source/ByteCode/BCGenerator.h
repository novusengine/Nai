#pragma once
#include <pch/Build.h>
#include "../Parser/Parser.h"
#include "../Parser/AST.h"

struct ModuleByteCode
{
    ModuleByteCode(ModuleParser& parserOutput) : _moduleParser(parserOutput) { }

    const ModuleParser& GetParserOutput() { return _moduleParser; }

    const std::vector<Token>& GetTokens() { return _moduleParser.GetTokens(); }
    inline const Token* GetToken()
    {
        return _moduleParser.GetToken();
    }
    inline const Token* GetToken(size_t offset)
    {
        return _moduleParser.GetToken(offset);
    }
    inline const Token* GetTokenIncrement(size_t num = 1)
    {
        const Token* token = GetToken();
        IncrementIndex(num);

        return token;
    }

    inline size_t& GetIndex() { return _moduleParser.GetIndex(); }
    inline void IncrementIndex(size_t num = 1) { _moduleParser.IncrementIndex(num); }
    inline void ResetIndex() { _moduleParser.ResetIndex(); }

private:
    ModuleParser& _moduleParser;
};

class BCGenerator
{
public:
    BCGenerator(ModuleParser& parser) : _parser(parser) { }

    void Generate();
    void ParseFunction(ASTFunctionDecl* fn);

private:
     ModuleParser& _parser;
};

void HANDLE_MOVE_VAL_TO_REGISTER_A(BCVMContext* context, void* data);