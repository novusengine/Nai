#pragma once
#include <pch/Build.h>
#include "Lexer/Token.h"
#include "Lexer/Lexer.h"
#include "AST.h"

#include <vector>
#include "../Utils/RobinHood.h"
#include "../Utils/DebugHandler.h"

struct ModuleInfo
{
    ModuleInfo(LexerFile& lexerOutput) : _tokens(lexerOutput.tokens), _tokensNum(lexerOutput.tokens.size()) 
    {
        // Pre allocate 8 functions per module
        _functionNodes.reserve(8);
    }

    inline void SetRegistryCount(uint16_t count)
    {
        _registryCount = count;
    }

    inline bool GetToken(Token** out)
    {
        ZoneScopedNC("GetToken", tracy::Color::Green)
        return _GetToken(_tokenIndex, out);
    }
    inline bool EatToken(Token** out)
    {
        ZoneScopedNC("EatToken", tracy::Color::Green1)
        return _GetToken(_tokenIndex++, out);
    }
    inline bool EatToken()
    {
        ZoneScopedNC("EatToken", tracy::Color::Green1)
        Token* token = nullptr;
        return _GetToken(_tokenIndex++, &token);
    }
    inline bool PeekToken(Token** out)
    {
        ZoneScopedNC("PeekToken", tracy::Color::Green2)
        return _GetToken(_tokenIndex + 1, out);
    }
    inline void SetCurrentToken(const Token* token)
    {
        if (token)
        {
            _lineNum = token->lineNum;
            _colNum = token->colNum;
        }
    }

    inline const size_t& GetTokenIndex() { return _tokenIndex; }
    inline const size_t& GetTokenCount() { return _tokensNum; }
    inline void ResetIndex() { _tokenIndex = 0; }

    void AddFunctionNode(ASTFunctionDecl* fnDecl)
    {
        return _functionNodes.push_back(fnDecl);
    }
    std::vector<ASTFunctionDecl*>& GetFunctionNodes()
    {
        return _functionNodes;
    }

    // Helper Functions
    void InitVariable(ASTVariable* variable, Token* token, ASTFunctionDecl* fnDecl)
    {
        variable->UpdateToken(token);
        
        size_t nameHashed = variable->GetNameHashed();
        for (ASTVariable* var : fnDecl->GetVariables())
        {
            if (nameHashed == var->GetNameHashed())
            {
                variable->parent = var;
                break;
            }
        }

        // Create DataType for variable if previously undeclared
        if (!variable->parent)
            variable->dataType = GetDataType();

        fnDecl->AddVariable(variable);
    }
    ASTFunctionDecl* GetFunctionByNameHash(size_t nameHash)
    {
        for (ASTFunctionDecl* fnDecl : _functionNodes)
        {
            if (nameHash == fnDecl->GetNameHashed())
                return fnDecl;
        }

        return nullptr;
    }

    // Allocator Functions
    ASTSequence* GetSequence();
    ASTExpression* GetExpression();
    ASTValue* GetValue();
    ASTVariable* GetVariable();
    ASTDataType* GetDataType();
    ASTWhileStatement* GetWhileStatement();
    ASTIfStatement* GetIfStatement();
    ASTJmpStatement* GetJmpStatement();
    ASTReturnStatement* GetReturnStatement();
    ASTFunctionDecl* GetFunctionDecl();
    ASTFunctionParameter* GetFunctionParameter();
    ASTFunctionCall* GetFunctionCall();
    ASTFunctionArgument* GetFunctionArgument();
    ByteInstruction* GetByteInstruction();

    template <typename ...Args>
    void ReportInfo(std::string str, Token* token, Args... arg)
    {
        int lineNum = token ? token->lineNum : _lineNum;
        int colNum = token ? token->colNum : _colNum;

        str += " (Line " + std::to_string(lineNum) + ", Col: " + std::to_string(colNum) + ")";
        NC_LOG_MESSAGE(str, arg...);
    }
    template <typename ...Args>
    void ReportWarning(std::string str, Token* token, Args... arg)
    {
        int lineNum = token ? token->lineNum : _lineNum;
        int colNum = token ? token->colNum : _colNum;

        str += " (Line " + std::to_string(lineNum) + ", Col: " + std::to_string(colNum) + ")";
        NC_LOG_WARNING(str, arg...);
    }
    template <typename ...Args>
    void ReportError(std::string str, Token* token, Args... arg)
    {
        int lineNum = token ? token->lineNum : _lineNum;
        int colNum = token ? token->colNum : _colNum;

        str += " (Line " + std::to_string(lineNum) + ", Col: " + std::to_string(colNum) + ")";
        NC_LOG_ERROR(str, arg...);
    }
    template <typename ...Args>
    void ReportFatal(std::string str, Token* token, Args... arg)
    {
        int lineNum = token ? token->lineNum : _lineNum;
        int colNum = token ? token->colNum : _colNum;

        str += " (Line " + std::to_string(lineNum) + ", Col: " + std::to_string(colNum) + ")";
        NC_LOG_FATAL(str, arg...);
    }
    template <typename ...Args>
    void ReportSuccess(std::string str, Token* token, Args... arg)
    {
        int lineNum = token ? token->lineNum : _lineNum;
        int colNum = token ? token->colNum : _colNum;

        str += " (Line " + std::to_string(lineNum) + ", Col: " + std::to_string(colNum) + ")";
        NC_LOG_SUCCESS(str, arg...);
    }
    template <typename ...Args>
    void ReportDeprecated(std::string str, Token* token, Args... arg)
    {
        int lineNum = token ? token->lineNum : _lineNum;
        int colNum = token ? token->colNum : _colNum;

        str += " (Line " + std::to_string(lineNum) + ", Col: " + std::to_string(colNum) + ")";
        NC_LOG_DEPRECATED(str, arg...);
    }

private:
    inline bool _GetToken(size_t index, Token** out)
    {
        ZoneScopedNC("_GetToken", tracy::Color::Green3)

        if (index >= _tokensNum)
            return false;

        Token& token = _tokens[index];

        _lineNum = token.lineNum;
        _colNum = token.colNum;
        *out = &token;

        return true;
    }

    uint16_t _registryCount = 0;

    std::vector<ASTFunctionDecl*> _functionNodes;
    std::vector<Token>& _tokens;
    size_t _tokensNum = 0;
    size_t _tokenIndex = 0;

    int _lineNum = 1;
    int _colNum = 1;
};

class Parser
{
public:
    Parser();

    bool Run(ModuleInfo& moduleInfo);
    bool RunSemanticCheck(ModuleInfo& moduleInfo);

    // Syntax Analysis & AST Building
    bool ParseFunction(ModuleInfo& moduleInfo);
    bool ParseFunctionParameterList(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl);
    bool ParseFunctionReturnType(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl);
    bool ParseFunctionBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl);
    bool ParseFunctionCall(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTFunctionCall* out);
    bool ParseExpression(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTExpression* out);
    bool ParseExpressionSequence(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTExpression* out);
    bool GetExpressionValueFromToken(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, Token* token, ASTNode** out);
    bool ParseWhileStatement(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTWhileStatement* out);
    bool ParseWhileStatementCondition(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTWhileStatement* out);
    bool ParseWhileStatementBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTWhileStatement* out);
    bool ParseIfStatement(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTIfStatement* out);
    bool ParseIfStatementCondition(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTIfStatement* out);
    bool ParseIfStatementBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTIfStatement* out);
    bool ParseIfStatementSequence(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTIfStatement* out);

    // Semantics
    bool CheckFunctionParameters(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl);
    bool CheckFunctionBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl);

    bool GetTypeFromExpression(ModuleInfo& moduleInfo, NaiType& outType, ASTExpression* expression);
    NaiType GetTypeFromLiteral(uint64_t value);
    NaiType GetTypeFromVariable(ASTVariable* variable);
    NaiType GetTypeFromFunctionCall(ModuleInfo& moduleInfo, ASTFunctionCall* fnCall);

    // Utils
    inline NaiType GetTypeFromChar(const char* str, int size)
    {
        ZoneScopedNC("GetTypeFromChar", tracy::Color::Purple)

        size_t hashedStr = StringUtils::hash_djb2(str, size);

        const auto itr = _typeNameHashToNaiType.find(static_cast<uint32_t>(hashedStr));
        if (itr == _typeNameHashToNaiType.end())
            return NaiType::CUSTOM;

        return itr->second;
    }
private:
    robin_hood::unordered_map<uint32_t, NaiType> _typeNameHashToNaiType;
};