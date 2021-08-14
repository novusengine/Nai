#include "pch/Build.h"
#include "Interpreter.h"
#include <chrono>

#include "../../Module.h"

#include "Bytecode.h"
#include "ByteOpcode.h"

void Interpreter::Init()
{
    _bufferAllocator.Init(StackSize, HeapSize);
}

void Interpreter::Interpret(Module* module, Declaration* declaration)
{
    ZoneScopedNC("Interpret", tracy::Color::AliceBlue);
    assert(declaration->kind == Declaration::Kind::Function);

    _moduleStack.push(module);
    _callStack.push(declaration);
    FunctionMemoryInfo& memoryInfo = module->bytecodeInfo.functionHashToMemoryInfo[declaration->token->nameHash.hash];

    u8* ip = memoryInfo.instructions;
    u8* memoryEnd = memoryInfo.instructions + memoryInfo.numInstructionBytes;

    while (ip < memoryEnd)
    {
        ByteOpcode* opcode = reinterpret_cast<ByteOpcode*>(ip);
        ByteOpcode::Kind kind = opcode->kind;

#if NAI_DEBUG
       //DebugHandler::PrintSuccess("Opcode: %s -- %s", ByteOpcode::GetKindName(opcode->kind), &opcode->comment[0]);
#endif // NAI_DEBUG

        switch (kind)
        {
            case ByteOpcode::Kind::PushRegister:
            {
                ZoneScopedNC("Push Register", tracy::Color::Green);
                PushRegister* pushCmd = reinterpret_cast<PushRegister*>(ip);
                u64* rsp = GetRegister(Register::Rsp);

                *rsp -= 8;
                memcpy(&_memory[*rsp], GetRegister(pushCmd->reg), 8);

                if (*rsp > StackSize)
                {
                    DebugHandler::PrintError("Interpreter : Stack overflow");
                    exit(1);
                }

                ip += sizeof(PushRegister);
                break;
            }
            case ByteOpcode::Kind::PushNumber:
            {
                ZoneScopedNC("Push Number", tracy::Color::Green);
                PushNumber* pushCmd = reinterpret_cast<PushNumber*>(ip);
                u64* rsp = GetRegister(Register::Rsp);

                *rsp -= pushCmd->size;
                memcpy(&_memory[*rsp], &pushCmd->number, pushCmd->size);

                if (*rsp > StackSize)
                {
                    DebugHandler::PrintError("Interpreter : Stack overflow");
                    exit(1);
                }

                ip += sizeof(PushNumber);
                break;
            }
            case ByteOpcode::Kind::PopRegister:
            {
                ZoneScopedNC("Pop Register", tracy::Color::Turquoise);
                PopRegister* popCmd = reinterpret_cast<PopRegister*>(ip);
                u64* rsp = GetRegister(Register::Rsp);

                memcpy(GetRegister(popCmd->reg), &_memory[*rsp], 8);
                *rsp += 8;

                if (*rsp > StackSize)
                {
                    DebugHandler::PrintError("Interpreter : Stack underflow");
                    exit(1);
                }

                ip += sizeof(PopRegister);
                break;
            }
            case ByteOpcode::Kind::PopNoRegister:
            {
                ZoneScopedNC("Pop No Register", tracy::Color::Turquoise);
                PopNoRegister* popCmd = reinterpret_cast<PopNoRegister*>(ip);
                u64* rsp = GetRegister(Register::Rsp);

                *rsp += popCmd->size;

                if (*rsp > StackSize)
                {
                    DebugHandler::PrintError("Interpreter : Stack underflow");
                    exit(1);
                }

                ip += sizeof(PopNoRegister);
                break;
            }

            case ByteOpcode::Kind::JumpAbsolute:
            {
                ZoneScopedNC("JumpAbsolute", tracy::Color::Purple);
                JumpAbsolute* jumpCmd = reinterpret_cast<JumpAbsolute*>(ip);

                bool shouldJmp = !jumpCmd->flags.isConditional || compareFlag == jumpCmd->flags.condition;
                if (shouldJmp)
                {
                    ip = memoryInfo.instructions + jumpCmd->address;
                    break;
                }

                ip += sizeof(JumpAbsolute);
                break;
            }
            case ByteOpcode::Kind::JumpRelative:
            {
                ZoneScopedNC("JumpRelative", tracy::Color::Purple);
                JumpRelative* jumpCmd = reinterpret_cast<JumpRelative*>(ip);

                bool shouldJmp = !jumpCmd->flags.isConditional || compareFlag == jumpCmd->flags.condition;
                if (shouldJmp)
                {
                    ip += jumpCmd->address;
                    break;
                }

                ip += sizeof(JumpRelative);
                break;
            }
            case ByteOpcode::Kind::JumpFunctionEnd:
            {
                ZoneScopedNC("JumpFunctionEnd", tracy::Color::Purple);
                JumpFunctionEnd* jumpCmd = reinterpret_cast<JumpFunctionEnd*>(ip);

                bool shouldJmp = !jumpCmd->flags.isConditional || compareFlag == jumpCmd->flags.condition;
                if (shouldJmp)
                {
                    ip = memoryInfo.instructions + memoryInfo.cleanupAddress;
                    break;
                }

                ip += sizeof(JumpFunctionEnd);
                break;
            }

            case ByteOpcode::Kind::LoadAbsolute:
            {
                ZoneScopedNC("LoadAbsolute", tracy::Color::PeachPuff);
                LoadAbsolute* loadCmd = reinterpret_cast<LoadAbsolute*>(ip);

                u64* destRegister = GetRegister(loadCmd->GetDestination());
                u8 size = loadCmd->GetSize();

                memcpy(destRegister, &_memory[loadCmd->source], size);

                ip += sizeof(LoadAbsolute);
                break;
            }
            case ByteOpcode::Kind::LoadRelative:
            {
                ZoneScopedNC("LoadRelative", tracy::Color::PeachPuff);
                LoadRelative* loadCmd = reinterpret_cast<LoadRelative*>(ip);

                u64* destination = GetRegister(loadCmd->GetDestination());
                u64 basePointerAddress = *GetRegister(Register::Rbp);
                u8 size = loadCmd->GetSize();

                memcpy(destination, &_memory[basePointerAddress - loadCmd->source], size);

                ip += sizeof(LoadRelative);
                break;
            }
            case ByteOpcode::Kind::LoadRegister:
            {
                ZoneScopedNC("LoadRegister", tracy::Color::PeachPuff);
                LoadRegister* loadCmd = reinterpret_cast<LoadRegister*>(ip);

                u64* source = GetRegister(loadCmd->source);
                u64* destination = GetRegister(loadCmd->GetDestination());
                u8 size = loadCmd->GetSize();

                memcpy(destination, &_memory[*source], size);

                ip += sizeof(LoadRegister);
                break;
            }
            case ByteOpcode::Kind::LoadAddress:
            {
                ZoneScopedNC("LoadAddress", tracy::Color::PeachPuff);
                LoadAddress* loadCmd = reinterpret_cast<LoadAddress*>(ip);

                u64* destination = GetRegister(loadCmd->GetDestination());

                if (loadCmd->flags.isPointer)
                {
                    if (loadCmd->flags.loadRelative)
                    {
                        u64 basePointerAddress = *GetRegister(Register::Rbp);
                        u64 ptrAddress = basePointerAddress - loadCmd->source;
                        *destination = *reinterpret_cast<u64*>(&_memory[ptrAddress]);
                    }
                    else
                    {
                        u64 ptrAddress = loadCmd->source;
                        *destination = *reinterpret_cast<u64*>(&_memory[ptrAddress]);
                    }
                }
                else
                {
                    if (loadCmd->flags.loadRelative)
                    {
                        u64 basePointerAddress = *GetRegister(Register::Rbp);
                        *destination = basePointerAddress - loadCmd->source;
                    }
                    else
                    {
                        *destination = loadCmd->source;
                    }
                }

                ip += sizeof(LoadAddress);
                break;
            }

            case ByteOpcode::Kind::MemoryNew:
            {
                ZoneScopedNC("MemoryNew", tracy::Color::Brown);

                u64* rax = GetRegister(Register::Rax);
                size_t address = 0;
                {
                    AllocateHeap(*rax, address);
                    *rax = address;
                }

                ip += sizeof(MemoryNew);
                break;
            }

            case ByteOpcode::Kind::MemoryFree:
            {
                ZoneScopedNC("MemoryFree", tracy::Color::RosyBrown);

                u64 address = *GetRegister(Register::Rax);
                FreeHeap(address);

                ip += sizeof(MemoryFree);
                break;
            }

            case ByteOpcode::Kind::CreateStringOnHeap:
            {
                ZoneScopedNC("CreateStringOnHeap", tracy::Color::SandyBrown);

                CreateStringOnHeap* createStringCmd = reinterpret_cast<CreateStringOnHeap*>(ip);

                size_t size = 0;
                size_t address = 0;

                auto itr = module->bytecodeInfo.stringIndexToHeapAddress.find(createStringCmd->strIndex);
                if (itr == module->bytecodeInfo.stringIndexToHeapAddress.end())
                {
                    const std::string& str = module->bytecodeInfo.stringTable.GetString(createStringCmd->strIndex);
                    size = str.length() + 1;

                    AllocateHeap(size, address);
                    memcpy(&_memory[address], str.c_str(), size);

                    module->bytecodeInfo.stringIndexToHeapAddress[createStringCmd->strIndex] = address;
                }
                else
                {
                    address = itr->second;
                }

                *GetRegister(Register::Rax) = address;
                ip += sizeof(CreateStringOnHeap);
                break;
            }

            case ByteOpcode::Kind::FunctionCall:
            {
                ZoneScopedNC("Function Call", tracy::Color::Violet);
                FunctionCall* callCmd = reinterpret_cast<FunctionCall*>(ip);
                Declaration* funcDeclaration = module->bytecodeInfo.functionHashToDeclaration[callCmd->callhash];

                if (funcDeclaration->function.flags.nativeCall)
                {
                    _callStack.push(funcDeclaration);
                    module->_nativeFunctionHashToCallback[callCmd->callhash](this);
                    _callStack.pop();
                }
                else
                {
                    Interpret(module, funcDeclaration);
                }

                ip += sizeof(FunctionCall);
                break;
            }
            case ByteOpcode::Kind::Ret:
            {
                ZoneScopedNC("Ret", tracy::Color::VioletRed);
                ip = memoryEnd;
                break;
            }

            case ByteOpcode::Kind::MoveNToR:
            {
                ZoneScopedNC("MoveNToR", tracy::Color::Red);
                MoveNToR* moveCmd = reinterpret_cast<MoveNToR*>(ip);

                memcpy(GetRegister(moveCmd->destination), &moveCmd->source, moveCmd->size);

                ip += sizeof(MoveNToR);
                break;
            }
            case ByteOpcode::Kind::MoveNToA:
            {
                ZoneScopedNC("MoveNToA", tracy::Color::Red);
                MoveNToA* moveCmd = reinterpret_cast<MoveNToA*>(ip);

                i32 address = *GetRegister(moveCmd->destination) & 0xFFFFFFFF;
                memcpy(&_memory[address], &moveCmd->source, moveCmd->size);

                ip += sizeof(MoveNToA);
                break;
            }
            case ByteOpcode::Kind::MoveRToR:
            {
                ZoneScopedNC("MoveRToR", tracy::Color::Red);
                MoveRToR* moveCmd = reinterpret_cast<MoveRToR*>(ip);

                memcpy(GetRegister(moveCmd->destination), GetRegister(moveCmd->source), moveCmd->size);

                ip += sizeof(MoveRToR);
                break;
            }
            case ByteOpcode::Kind::MoveRToA:
            {
                ZoneScopedNC("MoveRToA", tracy::Color::Red);
                MoveRToA* moveCmd = reinterpret_cast<MoveRToA*>(ip);

                i32 address = *reinterpret_cast<i32*>(GetRegister(moveCmd->destination));
                memcpy(&_memory[address], GetRegister(moveCmd->source), moveCmd->size);

                ip += sizeof(MoveRToA);
                break;
            }
            case ByteOpcode::Kind::MoveAToR:
            {
                ZoneScopedNC("MoveAToR", tracy::Color::Red);
                MoveAToR* moveCmd = reinterpret_cast<MoveAToR*>(ip);

                i32 address = *reinterpret_cast<i32*>(GetRegister(moveCmd->source));
                memcpy(GetRegister(moveCmd->destination), &_memory[address], moveCmd->size);

                ip += sizeof(MoveAToR);
                break;
            }
            case ByteOpcode::Kind::MoveAToA:
            {
                ZoneScopedNC("MoveAToA", tracy::Color::Red);
                MoveAToA* moveCmd = reinterpret_cast<MoveAToA*>(ip);

                i32 sourceAddress = *reinterpret_cast<i32*>(GetRegister(moveCmd->source));
                i32 destinationAddress = *reinterpret_cast<i32*>(GetRegister(moveCmd->destination));
                memcpy(&_memory[destinationAddress], &_memory[sourceAddress], moveCmd->size);

                ip += sizeof(MoveAToA);
                break;
            }

            case ByteOpcode::Kind::AddNToR:
            {
                ZoneScopedNC("AddNToR", tracy::Color::OrangeRed);
                AddNToR* addCmd = reinterpret_cast<AddNToR*>(ip);

                u64* destination = GetRegister(addCmd->destination);
                u64 result = *destination + addCmd->source;

                memcpy(destination, &result, addCmd->size);

                ip += sizeof(AddNToR);
                break;
            }
            case ByteOpcode::Kind::AddNToA:
            {
                ZoneScopedNC("AddNToA", tracy::Color::OrangeRed);
                AddNToA* addCmd = reinterpret_cast<AddNToA*>(ip);

                u64* destination = reinterpret_cast<u64*>(&_memory[*GetRegister(addCmd->destination)]);
                u64 result = *destination + addCmd->source;

                memcpy(destination, &result, addCmd->size);

                ip += sizeof(AddNToA);
                break;
            }
            case ByteOpcode::Kind::AddRToR:
            {
                ZoneScopedNC("AddRToR", tracy::Color::OrangeRed);
                AddRToR* addCmd = reinterpret_cast<AddRToR*>(ip);

                u64* destination = GetRegister(addCmd->destination);
                u64 result = *destination + *GetRegister(addCmd->source);

                memcpy(destination, &result, addCmd->size);

                ip += sizeof(AddRToR);
                break;
            }
            case ByteOpcode::Kind::AddRToA:
            {
                ZoneScopedNC("AddRToA", tracy::Color::OrangeRed);
                AddRToA* addCmd = reinterpret_cast<AddRToA*>(ip);

                u64* destination = reinterpret_cast<u64*>(&_memory[*GetRegister(addCmd->destination)]);
                u64 result = *destination + *GetRegister(addCmd->source);

                memcpy(destination, &result, addCmd->size);

                ip += sizeof(AddRToA);
                break;
            }
            case ByteOpcode::Kind::AddAToR:
            {
                ZoneScopedNC("AddAToR", tracy::Color::OrangeRed);
                AddAToR* addCmd = reinterpret_cast<AddAToR*>(ip);

                u64* destination = GetRegister(addCmd->destination);
                u64 result = *destination + *reinterpret_cast<u64*>(&_memory[*GetRegister(addCmd->source)]);

                memcpy(destination, &result, addCmd->size);

                ip += sizeof(AddAToR);
                break;
            }
            case ByteOpcode::Kind::AddAToA:
            {
                ZoneScopedNC("AddAToA", tracy::Color::OrangeRed);
                AddAToA* addCmd = reinterpret_cast<AddAToA*>(ip);

                u64* destination = reinterpret_cast<u64*>(&_memory[*GetRegister(addCmd->destination)]);
                u64 result = *destination + *reinterpret_cast<u64*>(&_memory[*GetRegister(addCmd->source)]);

                memcpy(destination, &result, addCmd->size);

                ip += sizeof(AddAToA);
                break;
            }

            case ByteOpcode::Kind::SubRToR:
            {
                ZoneScopedNC("SubRToR", tracy::Color::IndianRed);
                SubRToR* subCmd = reinterpret_cast<SubRToR*>(ip);

                u64* destination = GetRegister(subCmd->destination);
                u64 result = *destination - *GetRegister(subCmd->source);

                memcpy(destination, &result, subCmd->size);

                ip += sizeof(SubRToR);
                break;
            }
            case ByteOpcode::Kind::SubNToR:
            {
                ZoneScopedNC("SubNToR", tracy::Color::IndianRed);
                SubNToR* subCmd = reinterpret_cast<SubNToR*>(ip);

                u64* destination = GetRegister(subCmd->destination);
                u64 result = *destination - subCmd->source;

                memcpy(destination, &result, subCmd->size);

                ip += sizeof(SubNToR);
                break;
            }

            case ByteOpcode::Kind::MulRToR:
            {
                ZoneScopedNC("MulRToR", tracy::Color::DarkRed);
                MulRToR* mulCmd = reinterpret_cast<MulRToR*>(ip);

                u64* destination = GetRegister(mulCmd->destination);
                u64 result = *destination * *GetRegister(mulCmd->source);

                memcpy(destination, &result, mulCmd->size);

                ip += sizeof(MulRToR);
                break;
            }

            case ByteOpcode::Kind::DivRToR:
            {
                ZoneScopedNC("DivRToR", tracy::Color::MediumVioletRed);
                DivRToR* divCmd = reinterpret_cast<DivRToR*>(ip);

                u64* destination = GetRegister(divCmd->destination);
                u64 result = *destination / *GetRegister(divCmd->source);

                memcpy(destination, &result, divCmd->size);

                ip += sizeof(DivRToR);
                break;
            }

            case ByteOpcode::Kind::ModuloRToR:
            {
                ZoneScopedNC("ModuloRToR", tracy::Color::PaleVioletRed);
                ModuloRToR* moduloCmd = reinterpret_cast<ModuloRToR*>(ip);

                if (moduloCmd->size == 1)
                {
                    u8* destination = reinterpret_cast<u8*>(GetRegister(moduloCmd->destination));
                    u8 result = *destination % *reinterpret_cast<u8*>(GetRegister(moduloCmd->source));

                    memcpy(destination, &result, moduloCmd->size);
                }
                else if (moduloCmd->size == 2)
                {
                    u16* destination = reinterpret_cast<u16*>(GetRegister(moduloCmd->destination));
                    u16 result = *destination % *reinterpret_cast<u16*>(GetRegister(moduloCmd->source));

                    memcpy(destination, &result, moduloCmd->size);
                }
                else if (moduloCmd->size == 4)
                {
                    u32* destination = reinterpret_cast<u32*>(GetRegister(moduloCmd->destination));
                    u32 result = *destination % *reinterpret_cast<u32*>(GetRegister(moduloCmd->source));

                    memcpy(destination, &result, moduloCmd->size);
                }
                else if (moduloCmd->size == 8)
                {
                    u64* destination = GetRegister(moduloCmd->destination);
                    u64 result = *destination % *GetRegister(moduloCmd->source);

                    memcpy(destination, &result, moduloCmd->size);
                }

                ip += sizeof(ModuloRToR);
                break;
            }

            case ByteOpcode::Kind::CmpE_RToR:
            {
                ZoneScopedNC("CmpE_RToR", tracy::Color::Azure);
                CmpE_RToR* cmpCmd = reinterpret_cast<CmpE_RToR*>(ip);

                if (cmpCmd->size == 1)
                {
                    u8* destination = reinterpret_cast<u8*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination == *reinterpret_cast<u8*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 2)
                {
                    u16* destination = reinterpret_cast<u16*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination == *reinterpret_cast<u16*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 4)
                {
                    u32* destination = reinterpret_cast<u32*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination == *reinterpret_cast<u32*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 8)
                {
                    u64* destination = GetRegister(cmpCmd->destination);
                    compareFlag = *destination == *GetRegister(cmpCmd->source);
                    *destination = compareFlag;
                }

                ip += sizeof(CmpE_RToR);
                break;
            }

            case ByteOpcode::Kind::CmpNE_RToR:
            {
                ZoneScopedNC("CmpNE_RToR", tracy::Color::Azure);
                CmpNE_RToR* cmpCmd = reinterpret_cast<CmpNE_RToR*>(ip);

                if (cmpCmd->size == 1)
                {
                    u8* destination = reinterpret_cast<u8*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination != *reinterpret_cast<u8*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 2)
                {
                    u16* destination = reinterpret_cast<u16*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination != *reinterpret_cast<u16*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 4)
                {
                    u32* destination = reinterpret_cast<u32*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination != *reinterpret_cast<u32*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 8)
                {
                    u64* destination = GetRegister(cmpCmd->destination);
                    compareFlag = *destination != *GetRegister(cmpCmd->source);
                    *destination = compareFlag;
                }

                ip += sizeof(CmpNE_RToR);
                break;
            }

            case ByteOpcode::Kind::CmpL_RToR:
            {
                ZoneScopedNC("CmpL_RToR", tracy::Color::Azure);
                CmpL_RToR* cmpCmd = reinterpret_cast<CmpL_RToR*>(ip);

                if (cmpCmd->size == 1)
                {
                    u8* destination = reinterpret_cast<u8*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination < *reinterpret_cast<u8*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 2)
                {
                    u16* destination = reinterpret_cast<u16*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination < *reinterpret_cast<u16*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 4)
                {
                    u32* destination = reinterpret_cast<u32*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination < *reinterpret_cast<u32*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 8)
                {
                    u64* destination = GetRegister(cmpCmd->destination);
                    compareFlag = *destination < *GetRegister(cmpCmd->source);
                    *destination = compareFlag;
                }

                ip += sizeof(CmpL_RToR);
                break;
            }

            case ByteOpcode::Kind::CmpLE_NToR:
            {
                ZoneScopedNC("CmpLE_NToR", tracy::Color::Azure);
                CmpLE_NToR* cmpCmd = reinterpret_cast<CmpLE_NToR*>(ip);

                if (cmpCmd->size == 1)
                {
                    u8* destination = reinterpret_cast<u8*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination <= static_cast<u8>(cmpCmd->source);
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 2)
                {
                    u16* destination = reinterpret_cast<u16*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination <= static_cast<u16>(cmpCmd->source);
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 4)
                {
                    u32* destination = reinterpret_cast<u32*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination <= static_cast<u32>(cmpCmd->source);
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 8)
                {
                    u64* destination = GetRegister(cmpCmd->destination);
                    compareFlag = *destination <= cmpCmd->source;
                    *destination = compareFlag;
                }

                ip += sizeof(CmpLE_NToR);
                break;
            }
            case ByteOpcode::Kind::CmpLE_RToR:
            {
                ZoneScopedNC("CmpLE_RToR", tracy::Color::Azure);
                CmpLE_RToR* cmpCmd = reinterpret_cast<CmpLE_RToR*>(ip);

                if (cmpCmd->size == 1)
                {
                    u8* destination = reinterpret_cast<u8*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination <= *reinterpret_cast<u8*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 2)
                {
                    u16* destination = reinterpret_cast<u16*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination <= *reinterpret_cast<u16*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 4)
                {
                    u32* destination = reinterpret_cast<u32*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination <= *reinterpret_cast<u32*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 8)
                {
                    u64* destination = GetRegister(cmpCmd->destination);
                    compareFlag = *destination <= *GetRegister(cmpCmd->source);
                    *destination = compareFlag;
                }

                ip += sizeof(CmpLE_RToR);
                break;
            }

            case ByteOpcode::Kind::CmpG_RToR:
            {
                ZoneScopedNC("CmpG_RToR", tracy::Color::Azure);
                CmpG_RToR* cmpCmd = reinterpret_cast<CmpG_RToR*>(ip);

                if (cmpCmd->size == 1)
                {
                    u8* destination = reinterpret_cast<u8*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination > *reinterpret_cast<u8*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 2)
                {
                    u16* destination = reinterpret_cast<u16*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination > *reinterpret_cast<u16*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 4)
                {
                    u32* destination = reinterpret_cast<u32*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination > *reinterpret_cast<u32*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 8)
                {
                    u64* destination = GetRegister(cmpCmd->destination);
                    compareFlag = *destination > *GetRegister(cmpCmd->source);
                    *destination = compareFlag;
                }

                ip += sizeof(CmpG_RToR);
                break;
            }
            case ByteOpcode::Kind::CmpGE_RToR:
            {
                ZoneScopedNC("CmpGE_RToR", tracy::Color::Azure);
                CmpGE_RToR* cmpCmd = reinterpret_cast<CmpGE_RToR*>(ip);

                if (cmpCmd->size == 1)
                {
                    u8* destination = reinterpret_cast<u8*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination >= *reinterpret_cast<u8*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 2)
                {
                    u16* destination = reinterpret_cast<u16*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination >= *reinterpret_cast<u16*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 4)
                {
                    u32* destination = reinterpret_cast<u32*>(GetRegister(cmpCmd->destination));
                    compareFlag = *destination >= *reinterpret_cast<u32*>(GetRegister(cmpCmd->source));
                    *destination = compareFlag;
                }
                else if (cmpCmd->size == 8)
                {
                    u64* destination = GetRegister(cmpCmd->destination);
                    compareFlag = *destination >= *GetRegister(cmpCmd->source);
                    *destination = compareFlag;
                }

                ip += sizeof(CmpGE_RToR);
                break;
            }

            default:
            {
                DebugHandler::PrintError("Interpreter : Unhandled Opcode::Kind(%s)", ByteOpcode::GetKindName(kind));
                exit(1);
            }
        }
    }

#if NAI_DEBUG
    DebugHandler::PrintSuccess("Interpreter : Function(%.*s) Returned (%u)", declaration->token->nameHash.length, declaration->token->nameHash.name, *GetRegister(Register::Rax));
#endif // NAI_DEBUG

    _callStack.pop();
    _moduleStack.pop();
}

void Interpreter::AllocateHeap(size_t size, size_t& address)
{
    if (!_bufferAllocator.New(size, address))
    {
        DebugHandler::PrintError("Interpreter : Heap full");
        exit(1);
    }
}

void Interpreter::FreeHeap(size_t address)
{
    if (!_bufferAllocator.Free(address))
    {
        DebugHandler::PrintError("Interpreter : Attempted to Free Memory that has not been allocated (Address: %u)", address);
        exit(1);
    }
}
