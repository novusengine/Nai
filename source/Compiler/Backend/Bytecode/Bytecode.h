#pragma once
#include "pch/Build.h"
#include "ByteOpcode.h"

struct Module;
struct ByteOpcode;
struct Scope;
struct Type;
struct Statement;
struct Expression;
struct Conditional;
struct Loop;
struct LoopPath;
struct Declaration;
struct Function;
struct Variable;

class Bytecode
{
public:
    static void Process(Module* module);
    
private:
    template <typename T>
    static void Emit(Module* module, T opcode, String comment = "")
    {
        size_t size = module->bytecodeInfo.opcodes.size();
        size_t newSize = size + sizeof(T);
        if (newSize > size)
        {
            module->bytecodeInfo.opcodes.resize(newSize);
        }

        // Trick to keep Release from giving a warning about comment being unused
        u32 commentLength = static_cast<u32>(comment.length());
        assert(commentLength < 32);

#if NAI_DEBUG
        if (commentLength > 0)
        {
            memcpy(&opcode.comment[0], comment.data(), commentLength);
        }
#endif // NAI_DEBUG

        memcpy(&module->bytecodeInfo.opcodes[size], &opcode, sizeof(T));
    }
    
    template <typename T>
    static size_t EmitData(Module* module, u8* data, size_t length)
    {
        size_t size = module->bytecodeInfo.data.size();
        size_t newSize = size + length;
        if (newSize > size)
        {
            module->bytecodeInfo.data.resize(newSize);
        }

        memcpy(&module->bytecodeInfo.data[size], data, length);
        return size;
    }

    static void EnterLoop(Module* module, Loop* loop);
    static void ExitLoop(Module* module);

    static void GenerateLoadFrom(Module* module, Type* type, Register sourceRegister, Register destinationRegister, bool isSourceAddress, bool isDestinationAddress);
    static void GenerateAddressInto(Module* module, Expression* expression, Register reg, bool loadAddress);

    static void GenerateScope(Module* module, Scope* scope);
    static void GenerateFunction(Module* module, Function* function);
    static void GenerateStatement(Module* module, Statement* statement);
    static void GenerateExpression(Module* module, Expression* expression);
    static void GenerateExpressionPrimary(Module* module, Expression* expression);
    static void GenerateExpressionUnary(Module* module, Expression* expression);
    static void GenerateExpressionBinary(Module* module, Expression* expression);
    static void GenerateExpressionCall(Module* module, Expression* expression);
    static void GenerateExpressionMemoryNew(Module* module, Expression* expression, size_t typeSize);
    static void GenerateExpressionMemoryFree(Module* module, Expression* expression);
    static void GenerateExpressionDot(Module* module, Expression* expression);
    static void GenerateConditional(Module* module, Conditional* conditional);
    static void GenerateLoop(Module* module, Loop* loop);
    static void GenerateLoopPath(Module* module, LoopPath* loopPath);

    static u64 GetAllocationSize(Type* type);
    static u64 GetAllocationAlignment(u64 number, u64 alignment);
    static i64 GenerateFunctionVariableOffsets(Module* module, Scope* scope);
    static Register GetParameterRegister(Module* module, Declaration* declaration);
    static void GenerateScopeVariableOffsets(Module* module, Scope* scope, i64& offset);
};