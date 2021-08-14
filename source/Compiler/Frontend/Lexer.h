#pragma once
#include "pch/Build.h"
#include "robin_hood.h"
#include "../Token.h"

struct Module;
struct LexerInfo;
class Lexer
{
public:
    static void Process(Module* module);

private:
    static void GetNextToken(LexerInfo& lexerInfo, Token& token);
    static void Advance(LexerInfo& lexerInfo, u32 count = 1);
    static void SkipWhitespaces(LexerInfo& lexerInfo);

    static bool IsWhitespace(char c);
    static bool IsCharacter(char c);
    static bool IsNumber(char c);
    static bool IsSingleQuote(char c);
    static bool IsString(char c);
    static bool IsCommentSL(LexerInfo& lexerInfo);
    static bool IsCommentML_Start(LexerInfo& lexerInfo);
    static bool IsCommentML_End(LexerInfo& lexerInfo);

    static bool CharToNumber(char c, u8& number);

    static void ParseIdentifier(LexerInfo& lexerInfo, Token& token);
    static void ParseNumber(LexerInfo& lexerInfo, Token& token);
    static void ParseSingleQuote(LexerInfo& lexerInfo, Token& token);
    static void ParseString(LexerInfo& lexerInfo, Token& token);
    static void ParsePunctuation(LexerInfo& lexerInfo, Token& token);
    static void SkipPunctuation(LexerInfo& lexerInfo, Token& token, Token::Kind kind, u32 count);
    static void ParseCommentSL(LexerInfo& lexerInfo, Token& token);
    static void ParseCommentML(LexerInfo& lexerInfo, Token& token);

    static robin_hood::unordered_map<u32, Token::Kind> keywords;
};