#pragma once
#include <pch/Build.h>

enum class ByteOpcode : uint8_t
{
    INVALID,
    MOVE_TO_REG,                // Val, Register
    ADD_TO_REG,                 // Val, Register
    SUBTRACT_TO_REG,            // Val, Register
    MULTIPLY_TO_REG,            // Val, Register
    DIVIDE_TO_REG,              // Val, Register
    MOVE_FROM_REG_TO_REG,       // Register, Register
    ADD_FROM_REG_TO_REG,        // Register, Register
    SUBTRACT_FROM_REG_TO_REG,   // Register, Register
    MULTIPLY_FROM_REG_TO_REG,   // Register, Register
    DIVIDE_FROM_REG_TO_REG,     // Register, Register
    COMPARE_TO_REG,             // Val, Register
    COMPARE_FROM_REG_TO_REG,    // Register, Register
    JMP,                        // Val
    JMP_CONDITIONAL,            // Val
    RETURN,                     // VM Keeps track of the instruction pointer
};

struct ByteInstruction
{
    ByteOpcode opcode = ByteOpcode::INVALID;

    uint64_t val1; // Value or Registry Index
    uint16_t val2; // Registry Index
};