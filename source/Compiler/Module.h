#pragma once
#include "pch/Build.h"
#include "robin_hood.h"
#include <vector>
#include <filesystem>
#include <functional>

#include "Token.h"
#include "Import.h"
#include "Tree.h"
#include "Backend/Bytecode/ByteOpcode.h"
#include "Utils/NameHash.h"
#include "Utils/StringTable.h"

struct LexerInfo
{
public:
    char* buffer = nullptr;
    size_t size = 0;
    u64 index = 0;
    u64 tokenIndex = 0;

    u32 line = 1;
    u32 column = 1;

    std::vector<Token> tokens;

public:
    char PeekBuffer(i64 offset = 0) 
    {
        if (index + offset >= size)
            return 0;

        return buffer[index + offset]; 
    }
    void SkipBuffer(i64 offset = 1) { index += offset; }
    Token& NewToken() { return tokens.emplace_back(); }

    Token* PeekToken(i64 offset = 0)
    {
        if (tokenIndex + offset >= tokens.size())
            return &tokens.back();

        return &tokens[tokenIndex + offset];
    }
    Token* ConsumeToken()
    {
        if (tokenIndex == tokens.size())
            return &tokens.back();

        return &tokens[tokenIndex++];
    }

    Token* ExpectToken(Token::Kind kind)
    {
        Token* token = ConsumeToken();
        if (token->kind != kind)
        {
            Error(token, "Expecting '%s' but got '%s'", Token::GetTokenKindName(kind), Token::GetTokenKindName(token->kind));
        }

        return token;
    }

    Token* SkipToken(Token::Kind kind)
    {
        Token* token = ConsumeToken();
        if (token->kind != kind)
        {
            Error(token, "Expecting '%s' but got '%s'", Token::GetTokenKindName(kind), Token::GetTokenKindName(token->kind));
        }

        return PeekToken();
    }

    static constexpr u32 ReportNumLineHistory = 3;
    template <typename... Args>
    void Error(Token* token, const char* message, Args... args)
    {
        // Tokens created outside of the compiler pipeline are reported using nameHash field
        if (token->ExternalToken())
        {
            DebugHandler::PrintColor("Error:\n\n", ColorCode::RED);
            DebugHandler::Print("%.*s", token->nameHash.length, token->nameHash.name);

            for (u32 i = 0; i < token->nameHash.length; i++)
            {
                DebugHandler::PrintColor("^", ColorCode::RED);
            }

            DebugHandler::Print_NoNewLine("\n");
            DebugHandler::PrintColor(message, ColorCode::MAGENTA, args...);
            DebugHandler::Print_NoNewLine("\n");

            exit(1);
        }

        const char* startPtr = buffer;
        const char* currentPtr = token->nameHash.name;

        u32 i = 0;
        while (i < ReportNumLineHistory)
        {
            while (currentPtr != startPtr && *currentPtr != '\n')
            {
                currentPtr--;
            }

            if (currentPtr == startPtr)
                break;

            if (++i == ReportNumLineHistory)
            {
                currentPtr++;
            }
            else
            {
                currentPtr--;
            }
        }

        DebugHandler::PrintColor("Error:\n", ColorCode::RED);

        u32 lineNum = token->line - i;

        for (u32 j = 0; j < i; j++)
        {
            DebugHandler::Print_NoNewLine(" %3d | ", lineNum++);

            while (char c = *currentPtr++)
            {
                if (c == '\n' || c == '\r')
                    break;

                DebugHandler::Print_NoNewLine("%c", c);
            }

            DebugHandler::Print("");
        }

        DebugHandler::Print_NoNewLine("       ");

        for (i = 1; i < token->column; i++)
        {
            DebugHandler::Print_NoNewLine(" ");
        }

        for (i = 0; i < token->nameHash.length; i++)
        {
            DebugHandler::PrintColor("^", ColorCode::RED);
        }

        DebugHandler::Print_NoNewLine("\n       ");
        DebugHandler::PrintColor(message, ColorCode::MAGENTA, args...);
        DebugHandler::Print("\n");

        exit(1);
    }

public:
    LexerInfo& operator=(const LexerInfo& a);
};

struct ParserInfo
{
public:
    Compound* block = nullptr;
    Scope* currentScope = nullptr;
    StructScope* currentStructScope = nullptr;
    Loop* currentLoop = nullptr;
};

struct TyperInfo
{
public:
    Scope* currentScope = nullptr;
    Struct* currentStruct = nullptr;

    bool typeResolved = true;
    bool unresolvedTypes = true;
};

struct FunctionMemoryInfo
{
    u8* instructions = nullptr;
    u32 numInstructionBytes = 0;

    u32 cleanupAddress = 0;
};

struct FunctionParamInfo
{
    std::vector<Declaration*> declarations;
    u32 extraParameterStackSpace = 0;
};

struct BytecodeInfo
{
public:
    Function* currentFunction;

    std::vector<u8> opcodes;
    StringTable stringTable;

    robin_hood::unordered_map<u32, u32> hashNameToDataIndex;
    robin_hood::unordered_map<u32, Declaration*> functionHashToDeclaration;
    robin_hood::unordered_map<u32, FunctionMemoryInfo> functionHashToMemoryInfo;
    robin_hood::unordered_map<u32, FunctionParamInfo> functionHashToParamInfo;
    robin_hood::unordered_map<u32, u64> stringIndexToHeapAddress;
};

struct Module;
class Interpreter;
typedef void NativeFunctionCallbackFunc(Interpreter* interpreter);
struct NativeFunction
{
public:
    enum class PassAs
    {
        Value,
        Pointer
    };

public:
    NativeFunction(Module* module, const String& name, std::function<NativeFunctionCallbackFunc> callback);

    void AddParamChar(const String& name, PassAs passAs);
    void AddParamBool(const String& name, PassAs passAs);
    void AddParamI8(const String& name, PassAs passAs);
    void AddParamI16(const String& name, PassAs passAs);
    void AddParamI32(const String& name, PassAs passAs);
    void AddParamI64(const String& name, PassAs passAs);
    void AddParamU8(const String& name, PassAs passAs);
    void AddParamU16(const String& name, PassAs passAs);
    void AddParamU32(const String& name, PassAs passAs);
    void AddParamU64(const String& name, PassAs passAs);

    void SetReturnTypeChar(PassAs passAs);
    void SetReturnTypeBool(PassAs passAs);
    void SetReturnTypeI8(PassAs passAs);
    void SetReturnTypeI16(PassAs passAs);
    void SetReturnTypeI32(PassAs passAs);
    void SetReturnTypeI64(PassAs passAs);
    void SetReturnTypeU8(PassAs passAs);
    void SetReturnTypeU16(PassAs passAs);
    void SetReturnTypeU32(PassAs passAs);
    void SetReturnTypeU64(PassAs passAs);
    void SetReturnTypeUnknown(const String& name, PassAs passAs);

private:
    void TryAddParam(const String& name, Type* type, PassAs passAs);
    void SetReturnType(Type* type, PassAs passAs);

private:
    std::string name;
    std::string returnName;
    std::vector<std::string> _paramNames;

    u32 _numParameters = 0;
    Module* _module = nullptr;
    Declaration* _declaration = nullptr;
};

struct Module
{
public:
    NameHash nameHash;
    std::filesystem::path path;

    LexerInfo lexerInfo;
    ParserInfo parserInfo;
    TyperInfo typerInfo;
    BytecodeInfo bytecodeInfo;

    std::vector<Import> imports;
    std::vector<ImportAlias> importAliases;

    robin_hood::unordered_map<u32, u32> importPathHashToImportIndex;
    robin_hood::unordered_map<u32, u32> importPathHashToAliasIndex;
    robin_hood::unordered_map<u32, u32> importAliasToImportIndex;

    robin_hood::unordered_map<u32, std::function<NativeFunctionCallbackFunc>> _nativeFunctionHashToCallback;
};