#pragma once
#include <pch/Build.h>

enum class ContextRegister : uint8_t
{
    INPUT_1,
    INPUT_2,
    OUTPUT,
    COUNT
};

enum class ByteOpcode : uint8_t
{
    INVALID,

    // Move
    MOVE_S8_TO_REG,         // Value, Register
    MOVE_S16_TO_REG,        // Value, Register
    MOVE_S32_TO_REG,        // Value, Register
    MOVE_S64_TO_REG,        // Value, Register
    MOVE_U8_TO_REG,         // Value, Register
    MOVE_U16_TO_REG,        // Value, Register
    MOVE_U32_TO_REG,        // Value, Register
    MOVE_U64_TO_REG,        // Value, Register
    MOVE_F32_TO_REG,        // Value, Register
    MOVE_F64_TO_REG,        // Value, Register

    // Arithmetic
    ARITHMETIC_S8_S8,       // Register, Register, Operation ID
    ARITHMETIC_S16_S16,     // Register, Register, Operation ID
    ARITHMETIC_S32_S32,     // Register, Register, Operation ID
    ARITHMETIC_S64_S64,     // Register, Register, Operation ID
    ARITHMETIC_U8_U8,       // Register, Register, Operation ID
    ARITHMETIC_U16_U16,     // Register, Register, Operation ID
    ARITHMETIC_U32_U32,     // Register, Register, Operation ID
    ARITHMETIC_U64_U64,     // Register, Register, Operation ID
    ARITHMETIC_F32_F32,     // Register, Register, Operation ID
    ARITHMETIC_F64_F64,     // Register, Register, Operation ID
    INCREMENT,              // (Register/Ptr)
    DECREMENT,              // (Register/Ptr)

    // Bitwise
    BITWISE_S8_S8,          // Register, Register, Operation ID
    BITWISE_S16_S16,        // Register, Register, Operation ID
    BITWISE_S32_S32,        // Register, Register, Operation ID
    BITWISE_S64_S64,        // Register, Register, Operation ID
    BITWISE_U8_U8,          // Register, Register, Operation ID
    BITWISE_U16_U16,        // Register, Register, Operation ID
    BITWISE_U32_U32,        // Register, Register, Operation ID
    BITWISE_U64_U64,        // Register, Register, Operation ID


    BITWISE_AND,            // (Value/Register/Pointer), (Register/Pointer)
    BITWISE_OR,             // (Value/Register/Pointer), (Register/Pointer)
    BITSHIFT_LEFT,          // (Value/Register/Pointer), (Register/Pointer)
    BITSHIFT_RIGHT,         // (Value/Register/Pointer), (Register/Pointer)

    COMPARE,                // (Value/Register/Pointer), (Register/Pointer)
    COMPARE_NOT_EQUAL,      // (Value/Register/Pointer), (Register/Pointer)
    COMPARE_LESS,           // (Value/Register/Pointer), (Register/Pointer)
    COMPARE_LESS_EQUAL,     // (Value/Register/Pointer), (Register/Pointer)
    COMPARE_GREATER,        // (Value/Register/Pointer), (Register/Pointer)
    COMPARE_GREATER_EQUAL,  // (Value/Register/Pointer), (Register/Pointer)
    COMPARE_AND,            // (Value/Register/Pointer), (Register/Pointer)
    COMPARE_OR,             // (Value/Register/Pointer), (Register/Pointer)
    
    PUSH,                   // (Register/Pointer)
    POP,                    // (Register/Pointer)
    MALLOC,                 // StackOffset, Size
    PUSH_STACK_FRAME,       // No Input
    POP_STACK_FRAME,        // No Input
    ADD_STACK_OFFSET,       // Offset Value
    SUBTRACT_STACK_OFFSET,  // Offset Value

    JMP,                    // Instruction Index
    JMP_CONDITIONAL,        // Instruction Index, Compare Value
    CALL,                   // Function Name Hash
    RETURN,                 // No Input
};

enum class OpcodeFlag : uint8_t
{
    NONE = 0,
    SRC_IS_VAL = 1 << 0,
    SRC_IS_REG = 1 << 1,
    SRC_IS_PTR = 1 << 2,
    DST_IS_VAL = 1 << 3,
    DST_IS_REG = 1 << 3,
    DST_IS_PTR = 1 << 4
};

enum class ArithmeticOpcode : uint8_t
{
    NONE,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
};

struct ByteInstruction
{
    ByteOpcode opcode = ByteOpcode::INVALID;
    OpcodeFlag flags = OpcodeFlag::NONE;

    uint64_t value1 = 0;
    uint64_t value2 = 0;
    uint8_t miscValue = 0;
};