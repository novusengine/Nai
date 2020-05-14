#pragma once
#include <pch/Build.h>
#include <vector>
#include "Lexer/Token.h"

class SyntaxAnalyser
{
public:
    void Init(std::vector<Token> tokens);
    void Process();

private:
    std::vector<Token> _tokens;
};