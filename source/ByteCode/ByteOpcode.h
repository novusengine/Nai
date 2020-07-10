#pragma once
#include <pch/Build.h>
#include <functional>

enum class ByteOpcodeId : uint16_t
{
    INVALID,
    MOVE_VAl_TO_REGISTER_A,
    COUNT
};

struct BCVMContext;
struct ByteOpcode
{
    ByteOpcodeId id = ByteOpcodeId::INVALID;

    std::function<void(BCVMContext*, void*)> callback = nullptr;
    BCVMContext* ctx = nullptr;
    void* data = nullptr;
};