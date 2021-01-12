#pragma once
#include <pch/Build.h>

namespace Memory
{
#pragma pack(push, 1)
    struct MemoryBlock
    {
        ~MemoryBlock()
        {
            if (data)
            {
                delete[] data;
            }
        }

        size_t offset = 0;
        uint8_t* data = nullptr;
        MemoryBlock* nextBlock = nullptr;

        template <typename T, typename... Args>
        T* Allocate(size_t size, Args... args)
        {
            ZoneScopedNC("MemoryBlock::Allocate", tracy::Color::Yellow1);

            // Note to self: This is called a 'new placement' first parentheses specify a memory address to intialize your object at
            T* result = new (&data[offset]) T(args...);
            offset += size;

            return result;
        }

        template <typename T>
        void Store(T* t, size_t size)
        {
            ZoneScopedNC("MemoryBlock::Store", tracy::Color::Yellow1);

            memcpy(&data[offset], &t, size);
            offset += size;
        }

        template <typename T>
        T* Get(size_t offset)
        {
            ZoneScopedNC("MemoryBlock::Get", tracy::Color::Yellow1);

            return reinterpret_cast<T*>(&data[offset]);
        }
    };
#pragma pack(pop)
}