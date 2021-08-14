#pragma once
#include "pch/Build.h"
#include "Utils/StringUtils.h"
#include "Utils/NameHash.h"

struct Token
{
public:
    enum class Kind
    {
        None,
        End_of_File,

        Parenthesis_Open,
        Parenthesis_Close,
        Bracket_Open,
        Bracket_Close,
        Curley_Bracket_Open,
        Curley_Bracket_Close,
        Semicolon,
        Colon,
        DoubleColon,
        Comma,
        Dot,
        Arrow,
        Hashtag,

        Identifier,
        String,
        Number,
        Comment,

        Op_Add,
        Op_Subtract,
        Op_Multiply,
        Op_Divide,
        Op_Modulo,
        Op_Assign,
        Op_LessThan,
        Op_LessEqual,
        Op_GreaterThan,
        Op_GreaterEqual,
        Op_Equal,
        Op_NotEqual,
        Op_Bitwise_And,
        Op_And,
        Op_Bitwise_Or,
        Op_Or,
        Op_At,

        Keyword_Struct,
        Keyword_Union,
        Keyword_Enum,
        Keyword_Fn,
        Keyword_If,
        Keyword_Else,
        Keyword_Loop,
        Keyword_Continue,
        Keyword_Break,
        Keyword_New,
        Keyword_Free,
        Keyword_Return
    };

    Kind kind = Kind::None;

    NameHashView nameHash;

    u32 line = std::numeric_limits<u32>().max();
    u32 column = std::numeric_limits<u32>().max();

    u64 number;

    bool ExternalToken()
    {
        return line == std::numeric_limits<u32>().max() || column == std::numeric_limits<u32>().max();
    }
    static const char* GetTokenKindName(Kind kind);
};