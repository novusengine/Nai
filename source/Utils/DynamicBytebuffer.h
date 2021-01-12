#pragma once
#include <pch/Build.h>
#include <cassert>
#include <cstring>

class DynamicBytebuffer
{
public:
    DynamicBytebuffer(size_t inSize)
    {
        _data = new uint8_t[inSize];
        size = inSize;
    }
    ~DynamicBytebuffer()
    {
        delete[] _data;
        _data = nullptr;
    }

    template <typename T>
    inline bool Get(T& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<T const*>(&_data[readData]);
        readData += sizeof(T);
        return true;
    }
    template <typename T>
    inline bool Get(T& val, size_t offset)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<T const*>(&_data[offset]);
        return true;
    }
    inline bool GetI8(int8_t& val)
    {
        assert(_data != nullptr);

        val = _data[readData];
        readData += sizeof(int8_t);
        return true;
    }
    inline bool GetU8(uint8_t& val)
    {
        assert(_data != nullptr);

        val = _data[readData];
        readData += sizeof(uint8_t);
        return true;
    }
    inline bool GetBytes(uint8_t* dest, size_t size)
    {
        assert(_data != nullptr);

        std::memcpy(dest, &_data[readData], size);
        readData += size;
        return true;
    }
    inline bool GetBytes(uint8_t* dest, size_t size, size_t offset)
    {
        assert(_data != nullptr);

        std::memcpy(dest, &_data[offset], size);
        return true;
    }
    inline bool GetI16(int16_t& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<int16_t*>(&_data[readData]);
        readData += sizeof(int16_t);
        return true;
    }
    inline bool GetU16(uint16_t& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<uint16_t*>(&_data[readData]);
        readData += sizeof(uint16_t);
        return true;
    }
    inline bool GetI32(int32_t& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<int32_t*>(&_data[readData]);
        readData += sizeof(int32_t);
        return true;
    }
    inline bool GetU32(uint32_t& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<uint32_t*>(&_data[readData]);
        readData += sizeof(uint32_t);
        return true;
    }
    inline bool GetF32(float& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<float*>(&_data[readData]);
        readData += sizeof(float);
        return true;
    }
    inline bool GetI64(int64_t& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<int64_t*>(&_data[readData]);
        readData += sizeof(int64_t);
        return true;
    }
    inline bool GetU64(uint64_t& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<uint64_t*>(&_data[readData]);
        readData += sizeof(uint64_t);
        return true;
    }
    inline bool GetF64(double& val)
    {
        assert(_data != nullptr);

        val = *reinterpret_cast<double*>(_data[readData]);
        readData += sizeof(double);
        return true;
    }
    inline void GetString(std::string& val)
    {
        assert(_data != nullptr);

        val.clear();
        while (readData < size)
        {
            char c = _data[readData++];
            if (c == 0)
                break;

            val += c;
        }
    }
    inline void GetString(std::string& val, int32_t size)
    {
        assert(_data != nullptr);

        val.clear();
        for (int32_t i = 0; i < size; i++)
        {
            val += _data[readData++];
        }
    }
    inline void GetStringByOffset(std::string& val, size_t offset)
    {
        assert(_data != nullptr);

        val.clear();
        while (offset < size)
        {
            char c = _data[offset++];
            if (c == 0)
                break;

            val += c;
        }
    }
    inline void GetStringByOffset(std::string& val, int32_t size, size_t offset)
    {
        assert(_data != nullptr);

        val.clear();
        for (int32_t i = 0; i < size; i++)
        {
            val += _data[offset++];
        }
    }

    template <typename T>
    inline bool Put(T val)
    {
        assert(_data != nullptr);

        size_t writeSize = sizeof(T);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], &val, writeSize);
        writtenData += writeSize;
        return true;
    }
    template <typename T>
    inline bool Put(T val, size_t offset)
    {
        assert(_data != nullptr);

        size_t writeSize = sizeof(T);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[offset], &val, writeSize);
        return true;
    }
    inline bool PutI8(int8_t val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(int8_t);
        CheckStorageSpace(writeSize);

        _data[writtenData] = val;
        writtenData += writeSize;
        return true;
    }
    inline bool PutU8(uint8_t val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(uint8_t);
        CheckStorageSpace(writeSize);

        _data[writtenData] = val;
        writtenData += writeSize;
        return true;
    }
    inline bool PutBytes(const uint8_t* val, size_t size)
    {
        assert(_data != nullptr);

        CheckStorageSpace(size);

        std::memcpy(&_data[writtenData], val, size);
        writtenData += size;
        return true;
    }
    inline bool PutI16(int16_t val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(int16_t);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], reinterpret_cast<const uint8_t*>(&val), writeSize);
        writtenData += writeSize;
        return true;
    }
    inline bool PutU16(uint16_t val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(uint16_t);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], reinterpret_cast<const uint8_t*>(&val), writeSize);
        writtenData += writeSize;
        return true;
    }
    inline bool PutI32(int32_t val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(int32_t);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], reinterpret_cast<const uint8_t*>(&val), writeSize);
        writtenData += writeSize;
        return true;
    }
    inline bool PutU32(uint32_t val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(uint32_t);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], reinterpret_cast<const uint8_t*>(&val), writeSize);
        writtenData += writeSize;
        return true;
    }
    inline bool PutF32(float val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(float);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], reinterpret_cast<const uint8_t*>(&val), writeSize);
        writtenData += writeSize;
        return true;
    }
    inline bool PutI64(int64_t val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(int64_t);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], reinterpret_cast<const uint8_t*>(&val), writeSize);
        writtenData += writeSize;
        return true;
    }
    inline bool PutU64(uint64_t val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(uint64_t);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], reinterpret_cast<const uint8_t*>(&val), writeSize);
        writtenData += writeSize;
        return true;
    }
    inline bool PutF64(double val)
    {
        assert(_data != nullptr);

        const size_t writeSize = sizeof(double);
        CheckStorageSpace(writeSize);

        std::memcpy(&_data[writtenData], reinterpret_cast<const uint8_t*>(&val), writeSize);
        writtenData += writeSize;
        return true;
    }
    inline size_t PutString(const std::string_view val)
    {
        assert(_data != nullptr);

        size_t writeSize = val.length();
        size_t writeSizeTotal = writeSize + 1;
        CheckStorageSpace(writeSizeTotal);

        std::memcpy(&_data[writtenData], val.data(), writeSize);
        writtenData += writeSize;
        _data[writtenData++] = 0;
        return writeSizeTotal;
    }

    inline void CheckStorageSpace(size_t inSize)
    {
        // Reallocate if true
        if (writtenData + inSize > size)
        {
            uint8_t* newArr = new uint8_t[size * 2];

            if (writtenData > 0)
            {
                memcpy(newArr, _data, writtenData);
            }

            delete[] _data;
            _data = newArr;
        }
    }

    inline void SetOwnership(bool hasOwnership) { _hasOwnership = hasOwnership; }
    inline bool HasOwnership() { return _hasOwnership; }

    inline bool SkipRead(size_t bytes) 
    {
        if (readData + bytes > size)
            return false;

        readData += bytes;
        return true;
    }
    inline bool SkipWrite(size_t bytes) 
    {
        if (writtenData + bytes > size)
            return false;

        writtenData += bytes;
        return true;
    }
    inline void Reset()
    {
        writtenData = 0;
        readData = 0;
    }

    inline bool IsEmpty() { return writtenData == 0; }
    inline bool IsFull() { return writtenData == size; }
    inline size_t GetSpace() { return size - writtenData; }
    inline size_t GetReadSpace() { return size - readData; }
    inline size_t GetActiveSize() { return writtenData - readData; }

    size_t writtenData = 0;
    size_t readData = 0;
    size_t size = 0;

    uint8_t* GetDataPointer() { return _data; }
    uint8_t* GetReadPointer() { return _data + readData; }
    uint8_t* GetWritePointer() { return _data + writtenData; }

private:
    uint8_t* _data = nullptr;
    bool _hasOwnership = false;
};
