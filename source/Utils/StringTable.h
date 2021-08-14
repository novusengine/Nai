#pragma once
#include <pch/Build.h>
#include <vector>
#include <robin_hood.h>
#include <shared_mutex>

class StringTable
{
public:
    StringTable() { }
    StringTable(size_t numToReserve) 
    {
        _strings.reserve(numToReserve);
        _hashes.reserve(numToReserve);
    }

    // Add string, return index into table
    u32 AddString(const std::string& string);

    const std::string& GetString(u32 index);
    u32 GetStringHash(u32 index);

    size_t GetNumStrings() const { return _strings.size(); }

    void CopyFrom(StringTable& other);

    void Clear();

    bool TryFindHashedString(u32 hash, u32& index) const;

private:
    std::vector<std::string> _strings;
    std::vector<u32> _hashes;

    std::shared_mutex _mutex;
};