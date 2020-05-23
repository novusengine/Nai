#pragma once
#include <pch/Build.h>
#include <vector>
#include <stack>
#include "Lexer/Token.h"
#include "Lexer/Lexer.h"
#include "../Utils/RobingHood.h"

struct ParserFile
{
    ParserFile(LexerFile& inFile) : lexerOutput(inFile) { }

    LexerFile& lexerOutput;
    std::stack<TokenType> stack;
};

class Parser
{
public:
    void Init();
    void Process(ParserFile& file);

private:
    bool CheckSyntax(ParserFile& file);
    void BuildAST(ParserFile& file);

    template<typename... Args>
    void ReportError(int errorCode, std::string str, Args... args)
    {
        std::stringstream ss;
        ss << "Parsing Error 2" << errorCode << ": " << str;

        printf_s(ss.str().c_str(), args...);
    }

    robin_hood::unordered_map<TokenType, robin_hood::unordered_map<TokenType, int>> _rules;
};