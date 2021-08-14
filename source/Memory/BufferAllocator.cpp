#include "pch/Build.h"
#include "BufferAllocator.h"

BufferAllocator::BufferAllocator()
{
    _frames.reserve(64);
}

void BufferAllocator::Init(size_t beginOffset, size_t bufferSize)
{
    SetBufferSize(bufferSize);
    _frames.clear();
    _addressToSize.clear();

    BufferAllocatorFrame& frame = _frames.emplace_back();
    frame.address = beginOffset;
    frame.size = bufferSize;
}

bool BufferAllocator::New(size_t size, size_t& address)
{
    address = 0;

    size_t index = std::numeric_limits<size_t>().max();

    for (size_t i = 0; i < _frames.size(); i++)
    {
        BufferAllocatorFrame& frame = _frames[i];
        if (frame.size >= size)
        {
            index = i;
            break;
        }
    }

    bool foundFrame = index != std::numeric_limits<size_t>().max();
    if (foundFrame)
    {
        BufferAllocatorFrame& frame = _frames[index];
        address = frame.address;

        frame.address += size;
        frame.size -= size;

        if (frame.size == 0)
            _frames.erase(_frames.begin() + index);

        _addressToSize[address] = size;
    }

    return foundFrame;
}

bool BufferAllocator::Free(size_t address)
{
    auto itr = _addressToSize.find(address);
    if (itr == _addressToSize.end())
        return false;

    size_t index = std::numeric_limits<size_t>().max();
    bool addressIsBehind = false;

    for (size_t i = 0; i < _frames.size(); i++)
    {
        BufferAllocatorFrame& frame = _frames[i];
        
        if ((frame.address - itr->second) == address)
        {
            index = i;
            addressIsBehind = true;
            break;
        }

        if ((frame.address + itr->second) == address)
        {
            index = i;
            break;
        }
    }

    bool canMerge = index != std::numeric_limits<size_t>().max();
    if (canMerge)
    {
        if (addressIsBehind)
        {
            BufferAllocatorFrame& frame = _frames[index];
            frame.address -= itr->second;
            frame.size += itr->second;
        }
        else
        {
            BufferAllocatorFrame& frame = _frames[index];
            frame.size += itr->second;
        }
    }
    else
    {
        BufferAllocatorFrame& frame = _frames.emplace_back();
        frame.address = address;
        frame.size = itr->second;
    }

    _addressToSize.erase(address);
    return true;
}
