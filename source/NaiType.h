#pragma once
#include <pch/Build.h>

enum class NaiType : uint32_t
{
    INVALID,
    AUTO,
    NAI_VOID,
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    
    CUSTOM = 64
};