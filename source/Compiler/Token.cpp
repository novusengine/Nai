#include "pch/Build.h"
#include "Token.h"

const char* TokenNames[] =
{
    "None",
    "End of File",
    "Parenthesis Open",
    "Parenthesis Close",
    "Bracket Open",
    "Bracket Close",
    "Curley Bracket Open",
    "Curley Bracket Close",
    "Semicolon",
    "Colon",
    "Double Colon",
    "Comma",
    "Dot",
    "Arrow",
    "Hashtag",

    "Identifier",
    "String",
    "Number",
    "Comment",

    "Add",
    "Subtract",
    "Multiply",
    "Division",
    "Modulus",
    "Assign",
    "Less Than",
    "Less Equal",
    "Greater Than",
    "Greater Equal",
    "Equal",
    "Not Equal",
    "Bitwise And",
    "And",
    "Bitwise Or",
    "Or",

    "Struct",
    "Enum",
    "Fn",
    "If",
    "Else",
    "Else If",
    "Loop",
    "Continue",
    "Break",
    "Return"
};

const char* Token::GetTokenKindName(Kind kind)
{
    return TokenNames[static_cast<u32>(kind)];
}
