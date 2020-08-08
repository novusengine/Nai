#pragma once
#include <pch/Build.h>
#include "ByteOpcode.h"

struct BytecodeContext
{
public:
    void Init(uint16_t registryCount)
    {
        if (_registries)
            delete[] _registries;

        _registries = new uint64_t[registryCount];
    }
    void Destruct()
    {
        delete[] _registries;
    }

    bool Prepare()
    {
        _instructionPointer = 0;
        _flagRegister = 0;
        return true;
    }

    bool RunInstructions(std::vector<ByteInstruction*>& instructions)
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
                    if (_flagRegister == instruction->val2)
                    {
                        i = instruction->val1;
                        _flagRegister = 0;
                        continue;
                    }

                    break;
                }
                case ByteOpcode::RETURN:
                {
                    // TODO: Handle Nested Function calls here
                    break;
                }

                default:
                    return false;
            }

            i++;
        }

        return true;
    }

    // Takes a relative Index
    uint64_t* GetRegistry(size_t index)
    {
        return &_registries[_instructionPointer + index];
    }

private:
    size_t _instructionPointer = 0;
    uint8_t _flagRegister = 0;
    uint64_t* _registries = nullptr;
};