#pragma once
#include <pch/Build.h>
#include "Memory/MemoryBlock.h"
#include "Memory/Allocator.h"

namespace Memory
{
#pragma pack(push, 1)
    class BlockAllocator : public Allocator
    {
    public:
        // Defaults blockSize to 16 MB
        BlockAllocator(const size_t blockSize = 16 * 1024 * 1024) : Allocator(blockSize) { }

        template <typename T, typename... Args>
        T* New(Args... args)
        {
            ZoneScopedNC("BlockAllocator::New", tracy::Color::Yellow)

            size_t size = sizeof(T);
            assert(size <= _blockSize && "size must be less or equal to _blockSize");

            if (_currentBlock->offset + size > _blockSize)
                AddBlock();

            // Allocate
            return _currentBlock->Allocate<T>(size, args...);
        }
    };
#pragma pack(pop)
}