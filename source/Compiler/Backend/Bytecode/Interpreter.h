#pragma once
#include "pch/Build.h"
#include "ByteOpcode.h"
#include "Utils/LinkedList.h"
#include "Memory/BufferAllocator.h"
#include <stack>

struct Module;
struct Declaration;

class Interpreter
{
public:
    Interpreter()
    {
        *GetRegister(Register::Rsp) = StackSize;
        *GetRegister(Register::Rbp) = StackSize;
    }

    void Init();

    void Interpret(Module* module, Declaration* function);

    template<typename T>
    T* GetParameter(u8 index, bool isPointer = false)
    {
        assert(index > 0);

        // Get Value From Registers
        if (index <= 4)
        {
            const static Register lookup[4] = { Register::Rcx, Register::Rdx, Register::R8, Register::R9 };

            Register reg = static_cast<Register>(lookup[index - 1]);

            if (isPointer)
            {
                u64 parameterAddress = *GetRegister(reg);
                T* ptr = reinterpret_cast<T*>(&_memory[parameterAddress]);
                return ptr;
            }
            else
            {
                return reinterpret_cast<T*>(GetRegister(reg));
            }
        }
        // Get Value From Stack
        else
        {
            assert(_moduleStack.size() > 0);
            assert(_callStack.size() > 0);
            Module* currModule = _moduleStack.top();
            Declaration* currFunc = _callStack.top();

            FunctionParamInfo& paramInfo = currModule->bytecodeInfo.functionHashToParamInfo[currFunc->token->nameHash.hash];
            Declaration* param = paramInfo.declarations[index - 1];

            // Native functions don't setup a function frame in Nai because the code runs in a cpp callback, therefore Rsp is "actually" storing our Rbp
            u64* rbp = GetRegister(Register::Rsp);
            size_t stackAddress = (*rbp + paramInfo.extraParameterStackSpace) - param->variable.offset;

            if (isPointer)
            {
                u64 ptrAddress = _memory[stackAddress];
                return reinterpret_cast<T*>(&_memory[ptrAddress]);
            }
            else
            {
                return reinterpret_cast<T*>(&_memory[stackAddress]);
            }
        }
    }
    u64* GetRegister(Register reg)
    {
        assert(reg < Register::Count);
        return &_registers[static_cast<u32>(reg) - 1];
    }

    u8* GetMemory() { return _memory; }
    void AllocateHeap(size_t size, size_t& address);
    void FreeHeap(size_t address);

private:
    static constexpr u32 RegisterCount = static_cast<u32>(Register::Count);
    static constexpr u32 StackSize = 1 * 1024 * 1024;
    static constexpr u32 HeapSize = 16 * 1024 * 1024;

    bool compareFlag = false;
    u64 _registers[RegisterCount];
    u8 _memory[StackSize + HeapSize];
    u32 _heapOffset = 0;

    BufferAllocator _bufferAllocator;
    std::stack<Module*> _moduleStack;
    std::stack<Declaration*> _callStack;
};