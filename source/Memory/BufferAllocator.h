#include "pch/Build.h"
#include <vector>
#include "robin_hood.h"

struct BufferAllocatorFrame
{
    size_t address;
    size_t size;
};

class BufferAllocator
{
public:
    BufferAllocator();

    void Init(size_t beginOffset, size_t bufferSize);
    bool New(size_t size, size_t& address);
    bool Free(size_t address);

public:
    size_t GetBufferSize() { return _bufferSize; }
    void SetBufferSize(size_t bufferSize) { _bufferSize = bufferSize; }

private:
    size_t _bufferSize;

    std::vector<BufferAllocatorFrame> _frames;
    robin_hood::unordered_map<size_t, size_t> _addressToSize;
};