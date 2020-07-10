#pragma once
#include <pch/Build.h>
#include <vector>
#include <stack>
#include "Lexer/Token.h"
#include "Lexer/Lexer.h"
#include "../Utils/RobingHood.h"

struct ASTNode;
struct ModuleParser
{
    ModuleParser(LexerFile& lexerOutput) : _lexerFile(lexerOutput) 
    {
        moduleFunctionNodes.reserve(8); // Preallocate space for 8 functions 
        moduleStructNodes.reserve(8); // Preallocate space for 8 Structs
        modueEnumNodes.reserve(4); // Preallocate space for 8 Enums
    }

    const LexerFile& GetLexerOutput() { return _lexerFile; }
    std::stack<TokenType>& GetStack() { return _stack; }

    const std::vector<Token>& GetTokens() { return _lexerFile.tokens; }
    inline const Token* GetToken() 
    { 
        return _GetToken(_parseIndex);
    }
    inline const Token* GetToken(size_t offset)
    {
        return _GetToken(_parseIndex + offset); 
    }
    inline const Token* GetTokenIncrement(size_t num = 1)
    {
        const Token* token = GetToken();
        IncrementIndex(num);

        return token;
    }

    std::vector<ASTNode*> moduleFunctionNodes;
    std::vector<ASTNode*> moduleStructNodes;
    std::vector<ASTNode*> modueEnumNodes;

    inline size_t& GetIndex() { return _parseIndex; }
    inline void IncrementIndex(size_t num = 1) { _parseIndex += num; }
    inline void ResetIndex() { _parseIndex = 0; }
private:
    inline const Token* _GetToken(size_t index)
    {
        if (index >= _lexerFile.tokens.size())
            return nullptr;

        return &_lexerFile.tokens[index];
    }

    LexerFile& _lexerFile;

    std::stack<TokenType> _stack;
    size_t _parseIndex = 0;
};


struct ASTNode;
struct ASTFunctionDecl;
struct ASTFunctionCall;
struct ASTStruct;
struct ASTEnum;
struct ASTVariable;
class Parser
{
public:
    void Init();
    void Process(ModuleParser& file);

private:
    bool CheckSyntax(ModuleParser& file);

    void BuildAST(ModuleParser& file);

    ASTFunctionDecl* ParseFunction(ModuleParser& parser);
    ASTFunctionCall* ParseFunctionCall(ModuleParser& parser);
    ASTStruct* ParseStruct(ModuleParser& parser);
    ASTEnum* ParseEnum(ModuleParser& parser);
    ASTVariable* ParseVariable(ModuleParser& parser);

    ASTNode* ParseExpression(ModuleParser& parser);

    ASTNode* ParseKeywordWhile(ModuleParser& parser);
    ASTNode* ParseKeywordIf(ModuleParser& parser);
    ASTNode* ParseKeywordFor(ModuleParser& parser);
    ASTNode* ParseKeywordReturn(ModuleParser& parser);

    void VisitAST(const std::vector<ASTNode*>& ast);
    void VisitNode(const ASTNode* node);

    template<typename... Args>
    void ReportError(int errorCode, std::string str, Args... args)
    {
        std::stringstream ss;
        ss << "Parsing Error 2" << errorCode << ": " << str;

        printf_s(ss.str().c_str(), args...);
    }

    robin_hood::unordered_map<TokenType, robin_hood::unordered_map<TokenType, int>> _rules;

    std::string defaultReturnTypeName = "void";
    std::string inferTypeName = "auto";
    Token* defaultReturnTypeToken = new Token();
    Token* inferTypeToken = new Token();
};