#pragma once
#include <pch/Build.h>
#include "Memory/MemoryBlock.h"
#include "Memory/BaseBlockAllocator.h"

namespace Memory
{
#pragma pack(push, 1)
    class BlockAllocator : public BaseBlockAllocator
    {
    public:
        // Defaults blockSize to 16 MB
        BlockAllocator(const size_t blockSize = 16 * 1024 * 1024) : BaseBlockAllocator(blockSize) { }

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