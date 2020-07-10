#pragma once
#include <pch/Build.h>
#include "../Parser/Parser.h"
#include "../Parser/AST.h"
#include "ByteOpcode.h"

struct BCVMContext
{
public:
    BCVMContext() { }

    uint64_t registerA;
};