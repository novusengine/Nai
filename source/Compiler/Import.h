#pragma once
#include "pch/Build.h"

struct Token;
struct Import
{
public:
    NameHash nameHash;
    u32 moduleIndex;

    std::vector<Token*> tokens;
};

struct ImportAlias
{
public:
    String alias;
    u32 importIndex;
};