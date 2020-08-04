#pragma once
#include <pch/Build.h>
#include "Memory/MemoryBlock.h"
#include "Memory/Allocator.h"

namespace Memory
{
#pragma pack(push, 1)
    class BlockContainer : public Allocator
    {
    public:
        // Defaults blockSize to 1 MB
        BlockContainer(const size_t blockSize = 1 * 1024 * 1024) : Allocator(blockSize) { }

        template <typename T>
        void Store(T* t)
        {
            ZoneScopedNC("BlockContainer::Store", tracy::Color::Yellow)

            size_t size = sizeof(T*);
            assert(size <= _blockSize && "size must be less or equal to _blockSize");

            if (_currentBlock->offset + size > _blockSize)
                AddBlock();

            // Store
            _currentBlock->Store<T>(t, size);
        }
    };
#pragma pack(pop)
}