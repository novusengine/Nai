#pragma once
#include <pch/Build.h>
#include "ByteOpcode.h"

struct BytecodeContext
{
public:
    void Init(uint32_t registryCount)
    {
        if (_registries)
            delete[] _registries;
        
        _registries = new uint64_t[registryCount];
        memset(&_registries[0], 0, sizeof(uint64_t) * registryCount);
    }
    void Destruct()
    {
        delete[] _registries;
    }

    bool Prepare()
    {
        _registerOffset = 0;
        _flagRegister = 0;
        return true;
    }

    bool RunInstructions(ModuleInfo& moduleInfo, std::vector<ByteInstruction*>& instructions)
    {
        for (size_t i = 0; i < instructions.size();)
        {
            ByteInstruction* instruction = instructions[i];

            switch (instruction->opcode)
            {
                case ByteOpcode::MOVE_TO_REG:
                {
                    *GetRegistry(instruction->val2) = instruction->val1;
                    break;
                }
                case ByteOpcode::ADD_TO_REG:
                {
                    *GetRegistry(instruction->val2) += instruction->val1;
                    break;
                }
                case ByteOpcode::SUBTRACT_TO_REG:
                {
                    *GetRegistry(instruction->val2) -= instruction->val1;
                    break;
                }
                case ByteOpcode::MULTIPLY_TO_REG:
                {
                    *GetRegistry(instruction->val2) *= instruction->val1;
                    break;
                }
                case ByteOpcode::DIVIDE_TO_REG:
                {
                    *GetRegistry(instruction->val2) /= instruction->val1;
                    break;
                }
                case ByteOpcode::COMPARE_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) == instruction->val1;
                    break;
                }
                case ByteOpcode::COMPARE_LESS_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) < instruction->val1;
                    break;
                }
                case ByteOpcode::COMPARE_GREATER_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) > instruction->val1;
                    break;
                }
                case ByteOpcode::COMPARE_LESS_EQUAL_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) <= instruction->val1;
                    break;
                }
                case ByteOpcode::COMPARE_GREATER_EQUAL_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) >= instruction->val1;
                    break;
                }
                case ByteOpcode::COMPARE_NOT_EQUAL_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) != instruction->val1;
                    break;
                }
                case ByteOpcode::MOVE_REG_TO_REG:
                {
                    *GetRegistry(instruction->val2) = *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::ADD_REG_TO_REG:
                {
                    *GetRegistry(instruction->val2) += *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::SUBTRACT_REG_TO_REG:
                {
                    *GetRegistry(instruction->val2) -= *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::MULTIPLY_REG_TO_REG:
                {
                    *GetRegistry(instruction->val2) *= *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::DIVIDE_REG_TO_REG:
                {
                    *GetRegistry(instruction->val2) /= *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::COMPARE_REG_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) == *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::COMPARE_LESS_REG_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) < *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::COMPARE_GREATER_REG_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) > *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::COMPARE_LESS_EQUAL_REG_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) <= *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::COMPARE_GREATER_EQUAL_REG_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) >= *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::COMPARE_NOT_EQUAL_REG_TO_REG:
                {
                    _flagRegister = *GetRegistry(instruction->val2) != *GetRegistry(instruction->val1);
                    break;
                }
                case ByteOpcode::JMP:
                {
                    i = instruction->val1;
                    continue;
                }
                case ByteOpcode::JMP_CONDITIONAL:
                {
                    // JMP_CONDITIONAL uses val2 to determine if we should jmp if the last comparison was true/false
                    if (GetFlags() == instruction->val2)
                    {
                        i = instruction->val1;
                        continue;
                    }

                    break;
                }
                case ByteOpcode::ADD_REGISTER_OFFSET:
                {
                    _registerOffset += instruction->val1;
                    break;
                }
                case ByteOpcode::SUBTRACT_REGISTER_OFFSET:
                {
                    _registerOffset -= instruction->val1;
                    break;
                }
                case ByteOpcode::CALL:
                {
                    ASTFunctionDecl* fnDecl = moduleInfo.GetFunctionByNameHash(instruction->val1);
                    RunInstructions(moduleInfo, fnDecl->GetInstructions());
                    break;
                }
                case ByteOpcode::RETURN:
                {
                    return true;
                }

                default:
                    return false;
            }

            i++;
        }

        return true;
    }

    // Read & Reset Flags
    uint8_t GetFlags()
    {
        // TODO: Later when we add more flags, we want to specifically request a certain bit in the flag and just reset that one
        
        uint8_t val = _flagRegister;
        _flagRegister = 0;

        return val;
    }

    // Takes a relative Index
    uint64_t* GetRegistry(size_t index)
    {
        return &_registries[_registerOffset + index];
    }

private:
    size_t _registerOffset = 0;
    uint8_t _flagRegister = 0;
    uint64_t* _registries = nullptr;
};