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

    bool RunInstructions(std::vector<ByteInstruction*>* instructions)
    {
        //uint16_t registryOffset = 0; // Needed for Nested Functions

        for (ByteInstruction* instruction : *instructions)
        {
            switch (instruction->opcode)
            {
                case ByteOpcode::MOVE_TO_REG:
                {
                    _registries[instruction->val2] = instruction->val1;
                    break;
                }
                case ByteOpcode::ADD_TO_REG:
                {
                    _registries[instruction->val2] += instruction->val1;
                    break;
                }
                case ByteOpcode::SUBTRACT_TO_REG:
                {
                    _registries[instruction->val2] -= instruction->val1;
                    break;
                }
                case ByteOpcode::MULTIPLY_TO_REG:
                {
                    _registries[instruction->val2] *= instruction->val1;
                    break;
                }
                case ByteOpcode::DIVIDE_TO_REG:
                {
                    _registries[instruction->val2] /= instruction->val1;
                    break;
                }
                case ByteOpcode::MOVE_FROM_REG_TO_REG:
                {
                    _registries[instruction->val2] = _registries[instruction->val1];
                    break;
                }
                case ByteOpcode::ADD_FROM_REG_TO_REG:
                {
                    _registries[instruction->val2] += _registries[instruction->val1];
                    break;
                }
                case ByteOpcode::SUBTRACT_FROM_REG_TO_REG:
                {
                    _registries[instruction->val2] -= _registries[instruction->val1];
                    break;
                }
                case ByteOpcode::MULTIPLY_FROM_REG_TO_REG:
                {
                    _registries[instruction->val2] *= _registries[instruction->val1];
                    break;
                }
                case ByteOpcode::DIVIDE_FROM_REG_TO_REG:
                {
                    _registries[instruction->val2] /= _registries[instruction->val1];
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
        }

        return true;
    }

private:
    uint64_t* _registries = nullptr;
};