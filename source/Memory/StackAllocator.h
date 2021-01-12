#pragma once
#include <pch/Build.h>
#include "Memory/MemoryBlock.h"

namespace Memory
{
#pragma pack(push, 1)
    class StackAllocator
    {
    public:
        // Defaults stackSize to 1 MB
        StackAllocator(const size_t stackSize = 1 * 1024 * 1024) : _stackSize(stackSize)
        {
            assert(stackSize > 0);
            InitStack();
        }
        ~StackAllocator()
        {
            // Free Stack
            delete _stack;
        }

        void Reset()
        {
            _stack->offset = 0;
        }

        size_t GetStackSize() const { return _stackSize; }
        MemoryBlock* GetStack() const { return _stack; }

    protected:
        // This will update _stack
        void InitStack()
        {
            _stack = new MemoryBlock();

            void* stackData = new uint8_t[_stackSize];
            if (!stackData)
            {
                printf("Failed to allocate %zu bytes in InitStack\n", _stackSize);
            }

            _stack->data = static_cast<uint8_t*>(stackData);
            memset(_stack->data, 0, _stackSize);
        }

        size_t _stackSize;
        MemoryBlock* _stack = nullptr;
    };
#pragma pack(pop)
}