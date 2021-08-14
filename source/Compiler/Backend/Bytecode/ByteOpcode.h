#pragma once
#include "pch/Build.h"
#include "Utils/LinkedList.h"

enum class Register : u8
{
    None,

    Rax,
    Rbx,
    Rcx,
    Rdx,

    Rsi,
    Rdi,
    Rsp,
    Rbp,

    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,

    Count = R15
};

struct ByteOpcode
{
public:
    enum class Kind : u8
    {
        None,
        PushRegister,
        PushNumber,

        PopRegister,
        PopNoRegister,

        JumpAbsolute,
        JumpRelative,
        JumpFunctionEnd,

        LoadAbsolute,
        LoadRelative,
        LoadRegister,
        LoadAddress,

        MemoryNew,
        MemoryFree,

        CreateStringOnHeap,

        FunctionCall,
        FunctionCallNative,
        Ret,

        MoveNToR, // Move Number To Register
        MoveNToA, // Move Number To Address
        MoveRToR, // Move Register To Register
        MoveRToA, // Move Register To Address
        MoveAToR, // Move Address To Register
        MoveAToA, // Move Address To Address

        AddNToR, // Add Number To Register
        AddNToA, // Add Number To Address
        AddRToR, // Add Register To Register
        AddRToA, // Add Register To Address
        AddAToR, // Add Address To Register
        AddAToA, // Add Address To Address

        SubNToR, // Sub Number To Register
        SubRToR, // Sub Register To Register

        MulRToR, // Mul Register To Register

        DivRToR, // Div Register To Register

        ModuloRToR, // Modulo Register To Register

        CmpE_RToR, // Cmp Equals Register to Register
        CmpNE_RToR, // Cmp Not Equals Register to Register
        CmpL_RToR, // Cmp Less Register to Register
        CmpLE_NToR, // Cmp Less Equals Number to Register
        CmpLE_RToR, // Cmp Less Equals Register to Register
        CmpG_RToR, // Cmp Greater Register to Register
        CmpGE_RToR, // Cmp GreaterEquals Register to Register
    };

    ByteOpcode(Kind inKind) : kind(inKind) { }

    Kind kind = Kind::None;

#if NAI_DEBUG
    char comment[32] = { 0 };
#endif // NAI_DEBUG

public:
    static const char* GetKindName(Kind kind)
    {
        switch (kind)
        {
            case Kind::None: return "None";
            case Kind::PushRegister: return "PushRegister";
            case Kind::PushNumber: return "PushNumber";
            case Kind::PopRegister: return "PopRegister";
            case Kind::PopNoRegister: return "PopNoRegister";
            case Kind::JumpAbsolute: return "JumpAbsolute";
            case Kind::JumpRelative: return "JumpRelative";
            case Kind::JumpFunctionEnd: return "JumpFunctionEnd";
            case Kind::LoadAbsolute: return "LoadAbsolute";
            case Kind::LoadRelative: return "LoadRelative";
            case Kind::LoadRegister: return "LoadRegister";
            case Kind::LoadAddress: return "LoadAddress";
            case Kind::MemoryNew: return "MemoryNew";
            case Kind::MemoryFree: return "MemoryFree";
            case Kind::CreateStringOnHeap: return "CreateStringOnHeap";
            case Kind::FunctionCall: return "FunctionCall";
            case Kind::Ret: return "Ret";
            case Kind::MoveNToR: return "MoveNToR";
            case Kind::MoveNToA: return "MoveNToA";
            case Kind::MoveRToR: return "MoveRToR";
            case Kind::MoveRToA: return "MoveRToA";
            case Kind::MoveAToR: return "MoveAToR";
            case Kind::MoveAToA: return "MoveAToA";
            case Kind::AddNToR: return "AddNToR";
            case Kind::AddNToA: return "AddNToA";
            case Kind::AddRToR: return "AddRToR";
            case Kind::AddRToA: return "AddRToA";
            case Kind::AddAToR: return "AddAToR";
            case Kind::AddAToA: return "AddAToA";
            case Kind::SubNToR: return "SubNToR";
            case Kind::SubRToR: return "SubRToR";
            case Kind::MulRToR: return "MulRToR";
            case Kind::DivRToR: return "DivRToR";
            case Kind::ModuloRToR: return "ModuloRToR";
            case Kind::CmpE_RToR: return "CmpE_RToR";
            case Kind::CmpNE_RToR: return "CmpNE_RToR";
            case Kind::CmpL_RToR: return "CmpL_RToR";
            case Kind::CmpLE_NToR: return "CmpLE_NToR";
            case Kind::CmpLE_RToR: return "CmpLE_RToR";
            case Kind::CmpG_RToR: return "CmpG_RToR";
            case Kind::CmpGE_RToR: return "CmpGE_RToR";

            default: return "Invalid";
        }
    }

    static const char* GetRegisterName(Register reg)
    {
        switch (reg)
        {
            case Register::None: return "None";
            case Register::Rax: return "Rax";
            case Register::Rbx: return "Rbx";
            case Register::Rcx: return "Rcx";
            case Register::Rdx: return "Rdx";
            case Register::Rsi: return "Rsi";
            case Register::Rdi: return "Rdi";
            case Register::Rsp: return "Rsp";
            case Register::Rbp: return "Rbp";
            case Register::R8: return "R8";
            case Register::R9: return "R9";
            case Register::R10: return "R10";
            case Register::R11: return "R11";
            case Register::R12: return "R12";
            case Register::R13: return "R13";
            case Register::R14: return "R14";
            case Register::R15: return "R15";

            default: return "Invalid";
        }
    }
};

struct PushRegister : public ByteOpcode
{
public:
    PushRegister(Register inReg) : ByteOpcode(Kind::PushRegister)
    {
        reg = inReg;
    }

    Register reg;
};
struct PushNumber : public ByteOpcode
{
public:
    PushNumber(u64 inNumber, u8 inSize) : ByteOpcode(Kind::PushNumber)
    {
        assert(inSize >= 0 && inSize <= 8);

        number = inNumber;
        size = inSize;
    }

    u64 number;
    u8 size;
};

struct PopRegister : public ByteOpcode
{
public:
    PopRegister(Register inReg) : ByteOpcode(Kind::PopRegister)
    {
        reg = inReg;
    }

    Register reg;
};
struct PopNoRegister : public ByteOpcode
{
public:
    PopNoRegister(u8 inSize) : ByteOpcode(Kind::PopNoRegister)
    {
        size = inSize;
    }

    u8 size;
};

struct JumpAbsolute : public ByteOpcode
{
public:
    JumpAbsolute(i32 inAddress, bool isConditional = false, bool condition = false) : ByteOpcode(Kind::JumpAbsolute)
    {
        flags.isConditional = isConditional;
        flags.condition = condition;
        address = inAddress;
    }

    struct
    {
        u8 isConditional : 1;
        u8 condition : 1;
    } flags;

    i32 address;
};
struct JumpRelative : public ByteOpcode
{
public:
    JumpRelative(i32 inAddress, bool isConditional = false, bool condition = false) : ByteOpcode(Kind::JumpRelative)
    {
        flags.isConditional = isConditional;
        flags.condition = condition;
        address = inAddress;
    }

    struct
    {
        u8 isConditional : 1;
        u8 condition : 1;
    } flags;

    i32 address;
};
struct JumpFunctionEnd : public ByteOpcode
{
public:
    JumpFunctionEnd(bool isConditional = false, bool condition = false) : ByteOpcode(Kind::JumpFunctionEnd)
    {
        flags.isConditional = isConditional;
        flags.condition = condition;
    }

    struct
    {
        u8 isConditional : 1;
        u8 condition : 1;
    } flags;
};

struct Load : public ByteOpcode
{
public:
    u8 GetSize() { return packedSizeAndDestination & 0xF; }
    Register GetDestination() { return static_cast<Register>(packedSizeAndDestination >> 4); }

    u8 packedSizeAndDestination;

private:
    friend struct LoadAbsolute;
    friend struct LoadRelative;
    friend struct LoadRegister;
    friend struct LoadAddress;

    Load(ByteOpcode::Kind inKind, Register inDestination, u8 inSize) : ByteOpcode(inKind)
    {
        assert(inSize >= 0 && inSize <= 8);
        packedSizeAndDestination = inSize | (static_cast<u8>(inDestination) << 4);
    }
};
struct LoadAbsolute : public Load
{
public:
    LoadAbsolute(i32 address, Register destination, u8 size) : Load(Kind::LoadAbsolute, destination, size)
    {
        source = address;
    }

    i32 source;
};
struct LoadRelative : public Load
{
public:
    LoadRelative(i32 address, Register destination, u8 size) : Load(Kind::LoadRelative, destination, size)
    {
        assert(size > 0 && size <= 8);
        source = address;
    }

    i32 source;
};
struct LoadRegister : public Load
{
public:
    LoadRegister(Register reg, Register destination, u8 size) : Load(Kind::LoadRegister, destination, size)
    {
        source = reg;
    }

    Register source;
};
struct LoadAddress : public Load
{
public:
    LoadAddress(i32 address, Register destination, bool loadRelative, bool isPointer) : Load(Kind::LoadAddress, destination, 2)
    {
        source = address;
        flags.loadRelative = loadRelative;
        flags.isPointer = isPointer;
    }

    i32 source;
    struct
    {
        u8 loadRelative : 1;
        u8 isPointer : 1;
    } flags;
};

struct MemoryNew : public ByteOpcode
{
public:
    MemoryNew() : ByteOpcode(Kind::MemoryNew) { }
};
struct MemoryFree : public ByteOpcode
{
public:
    MemoryFree() : ByteOpcode(Kind::MemoryFree) { }
};

struct CreateStringOnHeap : public ByteOpcode
{
public:
    CreateStringOnHeap(u16 inStrIndex) : ByteOpcode(Kind::CreateStringOnHeap) 
    {
        strIndex = inStrIndex;
    }

    u16 strIndex;
};

struct FunctionCall : public ByteOpcode
{
public:
    FunctionCall(u32 hash) : ByteOpcode(Kind::FunctionCall)
    {
        callhash = hash;
    }

    u32 callhash;
};
struct OpRet : public ByteOpcode
{
    OpRet() : ByteOpcode(Kind::Ret) { }
};

struct Move : public ByteOpcode
{
public:
    u8 size;
    struct
    {
        u8 isPointer : 1;
    } flags;

private:
    friend struct MoveNToR;
    friend struct MoveNToA;
    friend struct MoveRToR;
    friend struct MoveRToA;
    friend struct MoveAToR;
    friend struct MoveAToA;

    Move(Kind kind, u8 inSize, bool isPointer) : ByteOpcode(kind)
    {
        assert(inSize >= 0 && inSize <= 8);
        size = inSize;
        flags.isPointer = isPointer;
    }
};
struct MoveNToR : public Move
{
public:
    MoveNToR(u64 inSource, Register inDestination, u8 size, bool isPointer) : Move(Kind::MoveNToR, size, isPointer)
    {
        source = inSource;
        destination = inDestination;
    }

    u64 source;
    Register destination;
};
struct MoveNToA : public Move
{
public:
    MoveNToA(u64 inSource, Register inDestination, u8 size, bool isPointer) : Move(Kind::MoveNToA, size, isPointer)
    {
        source = inSource;
        destination = inDestination;
    }

    u64 source;
    Register destination;
};
struct MoveRToR : public Move
{
public:
    MoveRToR(Register inSource, Register inDestination, u8 size, bool isPointer) : Move(Kind::MoveRToR, size, isPointer)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct MoveRToA : public Move
{
public:
    MoveRToA(Register inSource, Register inDestination, u8 size, bool isPointer) : Move(Kind::MoveRToA, size, isPointer)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct MoveAToR : public Move
{
public:
    MoveAToR(Register inSource, Register inDestination, u8 size, bool isPointer) : Move(Kind::MoveAToR, size, isPointer)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct MoveAToA : public Move
{
public:
    MoveAToA(Register inSource, Register inDestination, u8 size, bool isPointer) : Move(Kind::MoveAToA, size, isPointer)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};

struct Add : public ByteOpcode
{
public:
    u8 size;

private:
    friend struct AddNToR;
    friend struct AddNToA;
    friend struct AddRToR;
    friend struct AddRToA;
    friend struct AddAToR;
    friend struct AddAToA;

    Add(Kind kind, u8 inSize) : ByteOpcode(kind)
    {
        assert(inSize >= 0 && inSize <= 8);
        size = inSize;
    }
};
struct AddNToR : public Add
{
public:
    AddNToR(u64 inSource, Register inDestination, u8 size) : Add(Kind::AddNToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    u64 source;
    Register destination;
};
struct AddNToA : public Add
{
public:
    AddNToA(u64 inSource, Register inDestination, u8 size) : Add(Kind::AddNToA, size)
    {
        source = inSource;
        destination = inDestination;
    }

    u64 source;
    Register destination;
};
struct AddRToR : public Add
{
public:
    AddRToR(Register inSource, Register inDestination, u8 size) : Add(Kind::AddRToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct AddRToA : public Add
{
public:
    AddRToA(Register inSource, Register inDestination, u8 size) : Add(Kind::AddRToA, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct AddAToR : public Add
{
public:
    AddAToR(Register inSource, Register inDestination, u8 size) : Add(Kind::AddAToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct AddAToA : public Add
{
public:
    AddAToA(Register inSource, Register inDestination, u8 size) : Add(Kind::AddAToA, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};

struct Sub : public ByteOpcode
{
public:
    u8 size;

private:
    friend struct SubNToR;
    friend struct SubNToA;
    friend struct SubRToR;
    friend struct SubRToA;
    friend struct SubAToR;
    friend struct SubAToA;

    Sub(Kind kind, u8 inSize) : ByteOpcode(kind)
    {
        assert(inSize >= 0 && inSize <= 8);
        size = inSize;
    }
};
struct SubNToR : public Sub
{
public:
    SubNToR(u64 inSource, Register inDestination, u8 size) : Sub(Kind::SubNToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    u64 source;
    Register destination;
};
struct SubRToR : public Sub
{
public:
    SubRToR(Register inSource, Register inDestination, u8 size) : Sub(Kind::SubRToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};

struct Mul : public ByteOpcode
{
public:
    u8 size;

private:
    friend struct MulNToR;
    friend struct MulNToA;
    friend struct MulRToR;
    friend struct MulRToA;
    friend struct MulAToR;
    friend struct MulAToA;

    Mul(Kind kind, u8 inSize) : ByteOpcode(kind)
    {
        assert(inSize >= 0 && inSize <= 8);
        size = inSize;
    }
};
struct MulRToR : public Mul
{
public:
    MulRToR(Register inSource, Register inDestination, u8 size) : Mul(Kind::MulRToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};

struct Div : public ByteOpcode
{
public:
    u8 size;

private:
    friend struct DivNToR;
    friend struct DivNToA;
    friend struct DivRToR;
    friend struct DivRToA;
    friend struct DivAToR;
    friend struct DivAToA;

    Div(Kind kind, u8 inSize) : ByteOpcode(kind)
    {
        assert(inSize >= 0 && inSize <= 8);
        size = inSize;
    }
};
struct DivRToR : public Div
{
public:
    DivRToR(Register inSource, Register inDestination, u8 size) : Div(Kind::DivRToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};

struct Modulo : public ByteOpcode
{
public:
    u8 size;

private:
    friend struct ModuloNToR;
    friend struct ModuloNToA;
    friend struct ModuloRToR;
    friend struct ModuloRToA;
    friend struct ModuloAToR;
    friend struct ModuloAToA;

    Modulo(Kind kind, u8 inSize) : ByteOpcode(kind)
    {
        assert(inSize >= 0 && inSize <= 8);
        size = inSize;
    }
};
struct ModuloRToR : public Modulo
{
public:
    ModuloRToR(Register inSource, Register inDestination, u8 size) : Modulo(Kind::ModuloRToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};

struct Cmp : public ByteOpcode
{
public:
    u8 size;

private:
    friend struct CmpE_RToR;
    friend struct CmpNE_RToR;
    friend struct CmpL_RToR;
    friend struct CmpLE_NToR;
    friend struct CmpLE_RToR;
    friend struct CmpG_RToR;
    friend struct CmpGE_RToR;

    Cmp(Kind kind, u8 inSize) : ByteOpcode(kind)
    {
        assert(inSize >= 0 && inSize <= 8);
        size = inSize;
    }
};
struct CmpE_RToR : public Cmp
{
public:
    CmpE_RToR(Register inSource, Register inDestination, u8 size) : Cmp(Kind::CmpE_RToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct CmpNE_RToR : public Cmp
{
public:
    CmpNE_RToR(Register inSource, Register inDestination, u8 size) : Cmp(Kind::CmpNE_RToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct CmpL_RToR : public Cmp
{
public:
    CmpL_RToR(Register inSource, Register inDestination, u8 size) : Cmp(Kind::CmpL_RToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct CmpLE_NToR : public Cmp
{
public:
    CmpLE_NToR(u64 inSource, Register inDestination, u8 size) : Cmp(Kind::CmpLE_NToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    u64 source;
    Register destination;
};
struct CmpLE_RToR : public Cmp
{
public:
    CmpLE_RToR(Register inSource, Register inDestination, u8 size) : Cmp(Kind::CmpLE_RToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct CmpG_RToR : public Cmp
{
public:
    CmpG_RToR(Register inSource, Register inDestination, u8 size) : Cmp(Kind::CmpG_RToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};
struct CmpGE_RToR : public Cmp
{
public:
    CmpGE_RToR(Register inSource, Register inDestination, u8 size) : Cmp(Kind::CmpGE_RToR, size)
    {
        source = inSource;
        destination = inDestination;
    }

    Register source;
    Register destination;
};