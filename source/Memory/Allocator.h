#pragma once
#include <pch/Build.h>
#include "Memory/MemoryBlock.h"

namespace Memory
{
#pragma pack(push, 1)
    class Allocator
    {
    public:
        Allocator(const size_t blockSize) : _blockSize(blockSize), _blockNum(0)
        {
            assert(blockSize > 0);
            AddBlock();
        }
        ~Allocator()
        {
            // Free Blocks
            MemoryBlock* currentBlock = _startBlock;
            for (size_t i = 0; i < _blockNum; i++)
            {
                MemoryBlock* nextBlock = currentBlock->nextBlock;

                delete currentBlock;
                currentBlock = nextBlock;
            }

            _startBlock = nullptr;
            _currentBlock = nullptr;
        }

        void Reset()
        {
            MemoryBlock* currentBlock = _startBlock;
            for (size_t i = 0; i < _blockNum; i++)
            {
                currentBlock->offset = 0;
                currentBlock = currentBlock->nextBlock;
            }
        }

        size_t GetBlockSize() const { return _blockSize; }
        size_t GetBlocksNum() const { return _blockNum; }
        MemoryBlock* GetFirstBlock() const { return _startBlock; }
        MemoryBlock* GetCurrentBlock() const { return _currentBlock; }

    protected:
        // This will internally update _currentBlock
        void AddBlock()
        {
            if (_currentBlock)
            {
                _currentBlock->nextBlock = new MemoryBlock();
                _currentBlock = _currentBlock->nextBlock;
            }
            else
            {
                _startBlock = new MemoryBlock();
                _currentBlock = _startBlock;
            }

            _blockNum += 1;

            size_t blockSize = sizeof(uint8_t) * _blockSize;
            void* blockData = malloc(blockSize);
            if (!blockData)
            {
                printf("Failed to allocate %zu bytes in AddBlock\n", blockSize);
            }

            _currentBlock->data = static_cast<uint8_t*>(blockData);
        }

        size_t _blockSize;
        size_t _blockNum;

        MemoryBlock* _startBlock = nullptr;
        MemoryBlock* _currentBlock = nullptr;
    };
#pragma pack(pop)
}