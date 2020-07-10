#include <pch/Build.h>
#include "BCVM.h"

void BCVM::Run()
{
    // Find Main (Entry)

    ASTFunctionDecl* mainFn = nullptr;

    for (ASTNode* node : _parser.moduleFunctionNodes)
    {
        if (strncmp(node->token->value, "main", 4) == 0)
        {
            mainFn = reinterpret_cast<ASTFunctionDecl*>(node);
            break;
        }
    }

    if (!mainFn)
        return;

    for (const ByteOpcode& opcode : mainFn->opcodes)
    {
        opcode.callback(context, opcode.data);
    }
}