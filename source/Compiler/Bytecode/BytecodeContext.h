#pragma once
#include <pch/Build.h>
#include "NaiType.h"
#include "ByteOpcode.h"
#include "../../Utils/DynamicBytebuffer.h"

#include <vector>

struct StackFrame
{
    StackFrame() { }
    StackFrame(uint64_t inStackOffset, uint64_t inHeapOffset) : stackOffset(inStackOffset), heapOffset(inHeapOffset) { }

    uint64_t stackOffset = 0;
    uint64_t heapOffset = 0;
};

constexpr size_t STACK_SIZE = 1 * 1024 * 1024; // 1 MB
constexpr size_t HEAP_SIZE = 16 * 1024 * 1024; // 16 MB

struct BytecodeContext
{
public:
    void Init()
    {
    }

    void Destruct()
    {
    }

    __forceinline void Prepare()
    {
    }

    bool RunInstructions(ModuleInfo& /*moduleInfo*/)
    {
        return true;
    }
};