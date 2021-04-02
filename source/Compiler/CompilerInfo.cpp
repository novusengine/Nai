#include <pch/Build.h>
#include "CompilerInfo.h"

#include <Utils/StringUtils.h>
#define REGISTER_TYPE(name, type, size) { StringUtils::hash_djb2(name, sizeof(name) - 1), TypeInfo(0, name, type, size) }

std::shared_mutex CompilerInfo::_mutex;
robin_hood::unordered_map<size_t, TypeInfo> CompilerInfo::_typeInfoTable =
{
    REGISTER_TYPE("bool",   NaiType::BOOL, 1),
    REGISTER_TYPE("i8",     NaiType::I8, 1),
    REGISTER_TYPE("i16",    NaiType::I16, 2),
    REGISTER_TYPE("i32",    NaiType::I32, 4),
    REGISTER_TYPE("i64",    NaiType::I64, 8),
    REGISTER_TYPE("u8",     NaiType::U8, 1),
    REGISTER_TYPE("u16",    NaiType::U16, 2),
    REGISTER_TYPE("u32",    NaiType::U32, 4),
    REGISTER_TYPE("u64",    NaiType::U64, 8),
    REGISTER_TYPE("f32",    NaiType::F32, 4),
    REGISTER_TYPE("f64",    NaiType::F64, 8)
};