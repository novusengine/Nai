#pragma once
#include <pch/Build.h>
#include "../Parser/Parser.h"
#include "../Parser/AST.h"
#include "ByteOpcode.h"
#include "BCVMContext.h"

class BCVM
{
public:
    BCVM(ModuleParser& parser) : _parser(parser) { }

    void Run();

private:
     ModuleParser& _parser;
     BCVMContext* context = new BCVMContext();
};