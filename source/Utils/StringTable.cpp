#include <pch/Build.h>
#include "StringTable.h"
#include "../Utils/StringUtils.h"

u32 StringTable::AddString(const std::string& string)
{
    std::unique_lock lock(_mutex);

    // We need the hash of the string
    u32 stringHash = StringUtils::fnv1a_32(string.c_str(), string.length());
    
    // Check if the string already exists in this table, if so return that index
    u32 index = 0;
    if (TryFindHashedString(stringHash, index))
    {
        return index;
    }

    // Add the string to the table
    index = static_cast<u32>(_strings.size());

    _strings.push_back(string);
    _hashes.push_back(stringHash);

    return index;
}

const std::string& StringTable::GetString(u32 index)
{
    std::shared_lock lock(_mutex);

    assert(index < _strings.size());

    return _strings[index];
}

u32 StringTable::GetStringHash(u32 index)
{
    std::shared_lock lock(_mutex);

    assert(index < _hashes.size());

    return _hashes[index];
}

void StringTable::CopyFrom(StringTable& other)
{
    std::unique_lock ourLock(_mutex);
    std::shared_lock theirLock(other._mutex);

    _strings = other._strings;
    _hashes = other._hashes;
}

bool StringTable::TryFindHashedString(u32 hash, u32& index) const
{
    for (size_t i = 0; i < _hashes.size(); i++)
    {
        if (hash == _hashes[i])
        {
            index = static_cast<u32>(i);
            return true;
        }
    }

    return false;
}

void StringTable::Clear()
{
    std::unique_lock ourLock(_mutex);

    _strings.clear();
    _hashes.clear();
}