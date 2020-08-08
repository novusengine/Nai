#pragma once
#include <pch/Build.h>

enum class ByteOpcode : uint8_t
{
    INVALID,
    MOVE_TO_REG,                        // Val, Register
    ADD_TO_REG,                         // Val, Register
    SUBTRACT_TO_REG,                    // Val, Register
    MULTIPLY_TO_REG,                    // Val, Register
    DIVIDE_TO_REG,                      // Val, Register
    COMPARE_TO_REG,                     // Val, Register
    COMPARE_LESS_TO_REG,                // Val, Register
    COMPARE_GREATER_TO_REG,             // Val, Register
    COMPARE_LESS_EQUAL_TO_REG,          // Val, Register
    COMPARE_GREATER_EQUAL_TO_REG,       // Val, Register
    COMPARE_NOT_EQUAL_TO_REG,           // Val, Register
    COUNT_SINGLE_TO_REG = COMPARE_NOT_EQUAL_TO_REG, // This is used to know what we need to offset to translate between (val, reg) opcodes and (reg, reg) opcodes
    MOVE_REG_TO_REG,                    // Register, Register
    ADD_REG_TO_REG,                     // Register, Register
    SUBTRACT_REG_TO_REG,                // Register, Register
    MULTIPLY_REG_TO_REG,                // Register, Register
    DIVIDE_REG_TO_REG,                  // Register, Register
    COMPARE_REG_TO_REG,                 // Register, Register
    COMPARE_LESS_REG_TO_REG,            // Register, Register
    COMPARE_GREATER_REG_TO_REG,         // Register, Register
    COMPARE_LESS_EQUAL_REG_TO_REG,      // Register, Register
    COMPARE_GREATER_EQUAL_REG_TO_REG,   // Register, Register
    COMPARE_NOT_EQUAL_REG_TO_REG,       // Register, Register
    JMP,                                // Val
    JMP_CONDITIONAL,                    // Val, Val
    RETURN,                             // VM Keeps track of the instruction pointer
};

struct ByteInstruction
{
    ByteOpcode opcode = ByteOpcode::INVALID;

    // This is ONLY used to specify a "Val, Register" opcode and if needed the func will
    // translate it into a "Register, Register" opcode
    inline void UpdateOpcode(ByteOpcode inOpcode, bool translate)
    {
        opcode = static_cast<ByteOpcode>(static_cast<uint8_t>(inOpcode) + static_cast<uint8_t>(ByteOpcode::COUNT_SINGLE_TO_REG) * translate);
    }

    uint64_t val1; // Value or Registry Index
    uint16_t val2; // Registry Index
};