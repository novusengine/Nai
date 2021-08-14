#pragma once
#include "pch/Build.h"
#include <shared_mutex>

template <typename T>
struct BucketArray
{
public:
    BucketArray(size_t bucketSize = 1 * 1024)
    {
        _elementSize = sizeof(T);

        // Ensure the _bucketSize is padded to contain exactly a whole number of elements of type T
        _bucketSize = _elementSize * ((bucketSize + (_elementSize - 1)) / _elementSize);

        CreateBucket();
    }

    void Push(T& element)
    {
        std::unique_lock lock(_mutex);

        if (IsBucketFull())
            CreateBucket();

        _bucketCurrent->Store(element);
    }

    template <typename... Args>
    T* Emplace(Args... args)
    {
        std::unique_lock lock(_mutex);

        if (IsBucketFull())
            CreateBucket();

        return _bucketCurrent->New(args...);
    }

public:
    T* operator[](i32 index)
    {
        Bucket* bucket = _bucketFirst;
        i32 currentMaxElements = 0;

        while (bucket)
        {
            currentMaxElements += bucket->elementNum;

            if (index < currentMaxElements)
                return bucket->Get(index);

            bucket = bucket->next;
        }

        assert(false && "BucketArray: Tried to index into array with unused index");
        return nullptr;
    }

private:
    struct Bucket
    {
    public:
        u8* data = nullptr;

        size_t elementSize = sizeof(T);
        size_t elementNum = 0;
        size_t elementMax = 0;

        Bucket* next = nullptr;
        Bucket* prev = nullptr;

    public:
        void Store(T& element)
        {
            if (elementNum == elementMax)
                return;

            memcpy(&data[elementNum * elementSize], &element, elementSize);
            elementNum++;
        }

        template <typename... Args>
        T* New(Args... args)
        {
            if (elementNum == elementMax)
                return nullptr;

            T* result = new (&data[elementNum * elementSize]) T(args...);
            elementNum++;

            return result;
        }

        T* Get(size_t index)
        {
            if (index >= elementNum)
                return nullptr;

            return reinterpret_cast<T*>(&data[index * elementSize]);
        }
    };

    bool IsBucketFull()
    {
        return _bucketCurrent->elementNum == _bucketCurrent->elementMax;
    }

    void CreateBucket()
    {
        Bucket* bucket = new Bucket();
        bucket->data = new u8[_bucketSize];
        bucket->elementSize = _elementSize;
        bucket->elementMax = _bucketSize / _elementSize;
        bucket->prev = _bucketCurrent;

        if (_bucketFirst == nullptr)
        {
            _bucketFirst = bucket;
            _bucketCurrent = bucket;
        }
        else
        {
            _bucketCurrent->next = bucket;
            _bucketCurrent = bucket;
        }
    }

    Bucket* _bucketFirst = nullptr;
    Bucket* _bucketCurrent = nullptr;

    size_t _bucketSize;
    size_t _elementSize;

    std::shared_mutex _mutex;
};