#pragma once
#include <pch/Build.h>
#include <shared_mutex>
#include <robin_hood.h>

#include <Utils/StringUtils.h>

enum class NaiType : int
{
    NONE,
    BOOL,
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    STRING,
    STRUCT,
    ENUM
};

struct TypeInfo
{
    TypeInfo() { }
    TypeInfo(uint32_t inModuleHash, const char* inTypeName, NaiType inType, int inSize) : moduleHash(inModuleHash), typeName(inTypeName), type(inType), size(inSize)
    {
        typeNameHash = StringUtils::hash_djb2(inTypeName, sizeof(inTypeName) - 1);
    }
    TypeInfo(uint32_t inModuleHash, const std::string_view& inTypeName, NaiType inType, int inSize) : moduleHash(inModuleHash), typeName(inTypeName), type(inType), size(inSize)
    {
        typeNameHash = StringUtils::hash_djb2(inTypeName.data(), inTypeName.size());
    }

    uint32_t moduleHash = 0;

    std::string_view typeName;
    uint32_t typeNameHash = 0;
    
    NaiType type = NaiType::NONE;
    int size = 0;
};

class CompilerInfo
{
public:
    static bool GetTypeInfo(unsigned int moduleHash, unsigned int typeNameHash, TypeInfo& typeInfo)
    {
        std::shared_lock sharedLock(_mutex);

        // TODO: We need to find a way to properly handle checking all imported modules

        // First we check our own module, otherwise check the global namespace
        size_t hash = static_cast<size_t>(moduleHash) << 32 | typeNameHash;
        auto itr = _typeInfoTable.find(hash);
        if (itr == _typeInfoTable.end())
        {
            itr = _typeInfoTable.find(typeNameHash);
            if (itr == _typeInfoTable.end())
                return false;
        }

        typeInfo = itr->second;
        return true;
    }

    static bool AddTypeInfo(TypeInfo& typeInfo)
    {
        std::unique_lock uniqueLock(_mutex);

        auto itr = _typeInfoTable.find(typeInfo.typeNameHash);
        if (itr != _typeInfoTable.end())
            return false;

        size_t hash = static_cast<size_t>(typeInfo.moduleHash) << 32 | typeInfo.typeNameHash;
        _typeInfoTable[hash] = typeInfo;
        return true;
    }

private:
    static std::shared_mutex _mutex;
    static robin_hood::unordered_map<size_t, TypeInfo> _typeInfoTable;
};