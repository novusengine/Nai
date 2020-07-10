#include <pch/Build.h>
#include "BCGenerator.h"
#include "BCVMContext.h"

void BCGenerator::Generate()
{
    for (ASTNode* node : _parser.moduleFunctionNodes)
    {
        ASTFunctionDecl* fn = reinterpret_cast<ASTFunctionDecl*>(node);
        ParseFunction(fn);
    }
}

void BCGenerator::ParseFunction(ASTFunctionDecl* fn)
{
    ASTSequence* sequence = fn->top;
    while (sequence && (sequence->left || sequence->right))
    {
        ASTNode* node = sequence->left;
        if (node->type == ASTNodeType::VARIABLE)
        {
            ASTVariable* variable = reinterpret_cast<ASTVariable*>(node);
            
            // Check if the variable is being assigned to something
            if (variable->value)
            {
                // Dumbly assume the value is a literal of value uint64_t for now
                ASTExpression* expression = reinterpret_cast<ASTExpression*>(variable->value);
                uint64_t* value =  new uint64_t(StringUtils::ToUInt64(expression->token->value, expression->token->valueSize));

                ByteOpcode& opcode = fn->opcodes.emplace_back();
                opcode.id = ByteOpcodeId::MOVE_VAl_TO_REGISTER_A;
                opcode.data = value;
                opcode.callback = std::bind(&HANDLE_MOVE_VAL_TO_REGISTER_A, std::placeholders::_1, std::placeholders::_2);
            }
        }

        sequence = sequence->right;
    }
}

void HANDLE_MOVE_VAL_TO_REGISTER_A(BCVMContext* context, void* data)
{
    uint64_t val = *reinterpret_cast<uint64_t*>(data);
    context->registerA = val;
}
