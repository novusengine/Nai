#pragma once
#include "pch/Build.h"
#include "Types.h"
#include "StringUtils.h"

struct NameHash
{
    String name;
    u32 hash;

    void SetNameHash(String inName)
    {
        name = inName;
        hash = StringUtils::hash_djb2(name.c_str(), static_cast<i32>(name.length()));
    }

    void SetNameHash(char* ptr, size_t size)
    {
        name = String(ptr, size);
        hash = StringUtils::hash_djb2(name.c_str(), static_cast<i32>(name.length()));
    }
};

struct NameHashView
{
    const char* name;
    u32 length;
    u32 hash;

    void SetNameHash(const char* ptr, size_t size)
    {
        name = ptr;
        length = static_cast<i32>(size);
        hash = StringUtils::hash_djb2(name, static_cast<i32>(size));
    }
};