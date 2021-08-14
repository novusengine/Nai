#include "pch/Build.h"
#include "Bytecode.h"
#include "../../Module.h"

#include "ByteOpcode.h"

void Bytecode::Process(Module* module)
{
    module->bytecodeInfo.opcodes.clear();

    Compound* compound = module->parserInfo.block;
    GenerateScope(module, compound->scope);
}

void Bytecode::EnterLoop(Module* module, Loop* loop)
{
    Loop* currentLoop = module->parserInfo.currentLoop;

    loop->parent = currentLoop;
    module->parserInfo.currentLoop = loop;
}

void Bytecode::ExitLoop(Module* module)
{
    assert(module->parserInfo.currentLoop);
    module->parserInfo.currentLoop = module->parserInfo.currentLoop->parent;
}

void Bytecode::GenerateLoadFrom(Module* module, Type* type, Register source, Register destination, bool isSourceAddress, bool isDestinationAddress)
{
    assert(type && (type->kind == Type::Kind::Basic || type->kind == Type::Kind::Pointer));

    if (type->kind == Type::Kind::Pointer && type->pointer.count)
        return;

    if (type->size < 1 || type->size > 8)
    {
        DebugHandler::PrintError("Bytecode : Unsupported Type::size(%u)", type->size);
        exit(1);
    }

    if (isSourceAddress && isDestinationAddress)
    {
        Emit(module, MoveAToA(source, destination, static_cast<u8>(type->size), type->IsPointer()));
    }
    else if (!isSourceAddress && !isDestinationAddress)
    {
        Emit(module, MoveRToR(source, destination, static_cast<u8>(type->size), type->IsPointer()));
    }
    else if (isSourceAddress && !isDestinationAddress)
    {
        Emit(module, MoveAToR(source, destination, static_cast<u8>(type->size), type->IsPointer()));
    }
    else if (!isSourceAddress && isDestinationAddress)
    {
        Emit(module, MoveRToA(source, destination, static_cast<u8>(type->size), type->IsPointer()));
    }
}

void Bytecode::GenerateAddressInto(Module* module, Expression* expression, Register reg, bool loadAddress)
{
    if (expression->kind == Expression::Kind::Primary &&
        expression->primary.kind == Primary::Kind::Identifier)
    {
        if (loadAddress)
        {
            Emit(module, LoadAddress(static_cast<i32>(expression->primary.declaration->variable.offset), reg, true, false));
        }
        else
        {
            Emit(module, LoadRelative(static_cast<i32>(expression->primary.declaration->variable.offset), reg, static_cast<u8>(expression->primary.declaration->type->size)));
        }
    }
    else if (expression->kind == Expression::Kind::Unary &&
            expression->unary.kind == Unary::Kind::Deref)
    {
        GenerateExpression(module, expression->unary.operand);
    }
    else if (expression->kind == Expression::Kind::Dot)
    {
        GenerateAddressInto(module, expression->dot.expression, Register::Rax, true);
        Emit(module, AddNToR(expression->dot.offset, Register::Rax, static_cast<u8>(expression->type->size)));
    }
    else
    {
        DebugHandler::PrintError("Bytecode::GenerateAddress : Only supports identifiers (For now)");
        exit(1);
    }
}

void Bytecode::GenerateScope(Module* module, Scope* scope)
{
    // Nai has no global variables, so we only need to generate functions

    ListNode* node;
    ListIterate(&scope->declarations, node)
    {
        Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);
        if (declaration->kind == Declaration::Kind::Function)
            GenerateFunction(module, &declaration->function);
    }
}

void Bytecode::GenerateFunction(Module* module, Function* function)
{
    assert(function->body);

    Declaration* fnDecl = reinterpret_cast<Declaration*>(function);
    u32 functionHash = fnDecl->token->nameHash.hash;

    module->bytecodeInfo.currentFunction = function;
    module->bytecodeInfo.functionHashToDeclaration[functionHash] = fnDecl;
    FunctionParamInfo& paramInfo = module->bytecodeInfo.functionHashToParamInfo[functionHash];

    // Go over all variables (Including Child Scopes) and generate variable offsets
    u64 variableFrameSize = GenerateFunctionVariableOffsets(module, function->scope);

    ListNode* node;
    u64 numParameters = 0;

    ListIterate(&function->scope->declarations, node)
    {
        Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);
        if (declaration->kind == Declaration::Kind::Variable)
            numParameters++;
    }

    if (numParameters > 4)
    {
        constexpr u32 numParamsPassedToRegisters = 4;
        paramInfo.extraParameterStackSpace = static_cast<u8>(((numParameters - numParamsPassedToRegisters) * 8));
        Emit(module, AddNToR(paramInfo.extraParameterStackSpace + 8, Register::Rsp, 8), "> 4 Parameters");
    }

    Emit(module, PushRegister(Register::Rbp), "Save Rbp");
    Emit(module, MoveRToR(Register::Rsp, Register::Rbp, 8, false), "Set new Rbp");
    if (variableFrameSize > 0)
    {
        Emit(module, SubNToR(variableFrameSize, Register::Rsp, 8), "Move Rsp");
    }

    GenerateStatement(module, function->body);

    u32 cleanupAddress = static_cast<u32>(module->bytecodeInfo.opcodes.size());
    Emit(module, MoveRToR(Register::Rbp, Register::Rsp, 8, false), "Restore Rsp Stack");
    Emit(module, PopRegister(Register::Rbp), "Restore Rbp");
    Emit(module, OpRet(), "Function Return");

    FunctionMemoryInfo& memoryInfo = module->bytecodeInfo.functionHashToMemoryInfo[functionHash];
    memoryInfo.numInstructionBytes = static_cast<u32>(module->bytecodeInfo.opcodes.size());
    memoryInfo.instructions = new u8[memoryInfo.numInstructionBytes];
    memoryInfo.cleanupAddress = cleanupAddress;
    
    memcpy(memoryInfo.instructions, module->bytecodeInfo.opcodes.data(), memoryInfo.numInstructionBytes);
    module->bytecodeInfo.opcodes.clear();
}

void Bytecode::GenerateStatement(Module* module, Statement* statement)
{
    switch (statement->kind)
    {
        case Statement::Kind::Compound:
        {
            Compound* compound = &statement->compound;

            ListNode* node;
            ListIterate(&compound->statements, node)
            {
                Statement* stmt = ListGetStructPtr(node, Statement, listNode);
                GenerateStatement(module, stmt);
            }
            break;
        }
        case Statement::Kind::Comment:
        {
            break;
        }
        case Statement::Kind::Expression:
        {
            GenerateExpression(module, statement->expression);
            break;
        }
        case Statement::Kind::Return:
        {
            Return* ret = &statement->returnStmt;

            if (ret->expression)
            {
                GenerateExpression(module, ret->expression);
            }

            // We need JumpFunctionEnd so we can clean up the stack frame before we exit
            Emit(module, JumpFunctionEnd(), "Return (Clean Stack Frame)");
            break;
        }
        case Statement::Kind::Conditional:
        {
            GenerateConditional(module, &statement->conditional);
            break;
        }
        case Statement::Kind::Loop:
        {
            GenerateLoop(module, &statement->loop);
            break;
        }
        case Statement::Kind::Continue:
        case Statement::Kind::Break:
        {
            LoopPath& loopPath = module->parserInfo.currentLoop->paths->emplace_back();
            loopPath.kind = statement->kind == Statement::Kind::Continue ? LoopPath::Kind::Continue : LoopPath::Kind::Break;
            loopPath.opcodeDataIndex = module->bytecodeInfo.opcodes.size();

            GenerateLoopPath(module, &loopPath);
            break;
        }

        default:
        {
            DebugHandler::PrintError("Bytecode : Unhandled Statement::Kind(%u)", statement->kind);
            exit(1);
        }
    }
}

void Bytecode::GenerateExpression(Module* module, Expression* expression)
{
    switch (expression->kind)
    {
        case Expression::Kind::Primary:
        {
            GenerateExpressionPrimary(module, expression);
            break;
        }
        case Expression::Kind::Unary:
        {
            GenerateExpressionUnary(module, expression);
            break;
        }
        case Expression::Kind::Binary:
        {
            GenerateExpressionBinary(module, expression);
            break;
        }
        case Expression::Kind::Call:
        {
            GenerateExpressionCall(module, expression);
            break;
        }
        case Expression::Kind::MemoryNew:
        {
            GenerateExpressionMemoryNew(module, expression, 1);
            break;
        }
        case Expression::Kind::MemoryFree:
        {
            GenerateExpressionMemoryFree(module, expression);
            break;
        }
        case Expression::Kind::Dot:
        {
            GenerateExpressionDot(module, expression);
            break;
        }

        default:
        {
            DebugHandler::PrintError("Bytecode : Unhandled Expression::Kind(%u)", expression->kind);
            exit(1);
        }
    }
}

void Bytecode::GenerateExpressionPrimary(Module* module, Expression* expression)
{
    Primary* primary = &expression->primary;

    switch (primary->kind)
    {
        case Primary::Kind::Number:
        {
            Emit(module, MoveNToR(primary->number, Register::Rax, 8, false), "Move Number To Rax");
            break;
        }

        case Primary::Kind::Identifier:
        {
            Register reg = GetParameterRegister(module, expression->primary.declaration);
            
            if (reg == Register::None)
            {
                if (expression->type->IsPointer())
                {
                    Emit(module, LoadAddress(static_cast<i32>(expression->primary.declaration->variable.offset), Register::Rax, true, true), "Load Variable Address");
                }
                else
                {
                    Emit(module, LoadRelative(static_cast<i32>(expression->primary.declaration->variable.offset), Register::Rax, static_cast<u8>(expression->primary.declaration->type->size)), "Load Variable");
                }
            }
            else
            {
                Emit(module, MoveRToR(reg, Register::Rax, 8, false), "Move Parameter To Rax");
            }
            break;
        }

        case Primary::Kind::String:
        {
            u16 strIndex = static_cast<u16>(module->bytecodeInfo.stringTable.AddString(*primary->string));
            Emit(module, CreateStringOnHeap(strIndex));

            break;
        }

        default:
        {
            DebugHandler::PrintError("Bytecode : Unhandled Primary::Kind(%u)", primary->kind);
            exit(1);
        }
    }
}

void Bytecode::GenerateExpressionUnary(Module* module, Expression* expression)
{
    Unary* unary = &expression->unary;

    switch (unary->kind)
    {
        case Unary::Kind::Deref:
        {
            GenerateExpression(module, unary->operand);
            GenerateLoadFrom(module, expression->type, Register::Rax, Register::Rax, true, false);
            break;
        }
        case Unary::Kind::AddressOf:
        {
            GenerateAddressInto(module, unary->operand, Register::Rax, false);
            break;
        }

        default:
        {
            DebugHandler::PrintError("Bytecode : Unhandled Unary::Kind(%u)", unary->kind);
            exit(1);
        }
    }
}

void Bytecode::GenerateExpressionBinary(Module* module, Expression* expression)
{
    Binary* binary = &expression->binary;

    assert(binary->left);
    assert(binary->right);

    if (binary->kind == Binary::Kind::Assign)
    {
        GenerateAddressInto(module, binary->left, Register::Rax, true);
        Emit(module, PushRegister(Register::Rax));

        if (binary->right->kind == Expression::Kind::MemoryNew)
        {
            if (!binary->left->type->IsPointer() || binary->left->type->pointer.type->IsPointer())
            {
                module->lexerInfo.Error(binary->right->memory.token, "'New' can only be used with single pointer types");
            }

            GenerateExpressionMemoryNew(module, binary->right, binary->left->type->pointer.type->size);
        }
        else
        {
            GenerateExpression(module, binary->right);
        }

        Emit(module, PopRegister(Register::Rdi));
        GenerateLoadFrom(module, expression->type, Register::Rax, Register::Rdi, false, true);
    }
    else
    {
        if (binary->left->kind == Expression::Kind::MemoryNew)
        {
            module->lexerInfo.Error(binary->left->primary.token, "'New' can only be assigned to single pointer types");
        }
        else if (binary->right->kind == Expression::Kind::MemoryNew)
        {
            module->lexerInfo.Error(binary->right->memory.token, "'New' can only be assigned to single pointer types");
        }


        u8 size = static_cast<u8>(binary->left->type->size);
        if (binary->left->kind == Expression::Kind::Unary && binary->left->unary.kind == Unary::Kind::Deref)
            size = 8;

        GenerateExpression(module, binary->right);
        Emit(module, PushRegister(Register::Rax));
        GenerateExpression(module, binary->left);
        Emit(module, PopRegister(Register::Rdi));

        switch (binary->kind)
        {
            case Binary::Kind::Add:
            {
                Emit(module, AddRToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::Subtract:
            {
                Emit(module, SubRToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::Multiply:
            {
                Emit(module, MulRToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::Divide:
            {
                Emit(module, DivRToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::Modulo:
            {
                Emit(module, ModuloRToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::Equal:
            {
                Emit(module, CmpE_RToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::NotEqual:
            {
                Emit(module, CmpNE_RToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::LessThan:
            {
                Emit(module, CmpL_RToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::LessEqual:
            {
                Emit(module, CmpLE_RToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::GreaterThan:
            {
                Emit(module, CmpG_RToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }
            case Binary::Kind::GreaterEqual:
            {
                Emit(module, CmpGE_RToR(Register::Rdi, Register::Rax, static_cast<u8>(binary->left->type->size)));
                break;
            }

            default:
            {
                DebugHandler::PrintError("Bytecode : Unhandled Binary::Kind(%u)", binary->kind);
                exit(1);
            }
        }
    }
}

void Bytecode::GenerateExpressionCall(Module* module, Expression* expression)
{
    // Iterate all arguments
    //  - Generate Expression
    //  - Move Rax -> Correct Register

    Call* call = &expression->call;

    ListNode* node;
    u32 argumentCount = 0;
    u32 numPushedBytes = 0;

    ListIterate(&call->arguments, node)
    {
        argumentCount++;
    }

    Function* function = module->bytecodeInfo.currentFunction;
    u64 numVariablesInCurrentFunc = 0;

    ListIterate(&function->scope->declarations, node)
    {
        Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);
        if (declaration->kind == Declaration::Kind::Variable)
        {
            if (++numVariablesInCurrentFunc == 4)
                break;
        }
    }

    // Save Parameters in current func before calling next
    for (u64 i = 0; i < numVariablesInCurrentFunc; i++)
    {
        switch (i)
        {
            case 0:
            {
                Emit(module, PushRegister(Register::Rcx), "Save Parameter Rcx");
                break;
            }
            case 1:
            {
                Emit(module, PushRegister(Register::Rdx), "Save Parameter Rdx");
                break;
            }
            case 2:
            {
                Emit(module, PushRegister(Register::R8), "Save Parameter R8");
                break;
            }
            case 3:
            {
                Emit(module, PushRegister(Register::R9), "Save Parameter R9");
                break;
            }
        }
    }

    // If we have more than 4 arguments, we need to push extra space for RBP
    if (argumentCount > 4)
    {
        numPushedBytes += 8;
        Emit(module, PushRegister(Register::Rax), "Push Extra Rbp Space");
    }

    // Add Parameters to Signature
    ListIterateReverse(&call->arguments, node)
    {
        Expression* expr = ListGetStructPtr(node, Expression, listNode);
        GenerateExpression(module, expr); // Result is stored in Rax

        switch (--argumentCount)
        {
            case 0:
            {
                // Move Rax -> Rcx
                Emit(module, MoveRToR(Register::Rax, Register::Rcx, 8, false), "Arg1 -> Rcx");
                break;
            }
            case 1:
            {
                // Move Rax -> Rdx
                Emit(module, MoveRToR(Register::Rax, Register::Rdx, 8, false), "Arg2 -> Rdx");
                break;
            }
            case 2:
            {
                // Move Rax -> R8
                Emit(module, MoveRToR(Register::Rax, Register::R8, 8, false), "Arg3 -> R8");
                break;
            }
            case 3:
            {
                // Move Rax -> R9
                Emit(module, MoveRToR(Register::Rax, Register::R9, 8, false), "Arg4 -> R9");
                break;
            }

            default:
            {
                numPushedBytes += expr->type->size;
                Emit(module, PushRegister(Register::Rax), "Push argument");
                break;
            }
        }
    }
    
    // Generate Call Instruction
    Emit(module, FunctionCall(call->token->nameHash.hash), "Call Function");

    if (numPushedBytes > 0)
    {
        // Pop previously allocated space for arguments
        Emit(module, PopNoRegister(static_cast<u8>(numPushedBytes)), "Pop Pushed Argument");
    }

    // Restore Parameters in current func before calling next
    for (u64 i = 0; i < numVariablesInCurrentFunc; i++)
    {
        switch (i)
        {
            case 0:
            {
                Emit(module, PopRegister(Register::Rcx), "Restore Parameter Rcx");
                break;
            }
            case 1:
            {
                Emit(module, PopRegister(Register::Rdx), "Restore Parameter Rdx");
                break;
            }
            case 2:
            {
                Emit(module, PopRegister(Register::R8), "Restore Parameter R8");
                break;
            }
            case 3:
            {
                Emit(module, PopRegister(Register::R9), "Restore Parameter R9");
                break;
            }
        }
    }
}

void Bytecode::GenerateExpressionMemoryNew(Module* module, Expression* expression, size_t typeSize)
{
    MemoryExpression* memory = &expression->memory;
    GenerateExpression(module, memory->expression);
    Emit(module, MoveNToR(typeSize, Register::Rdi, 8, false));
    Emit(module, MulRToR(Register::Rdi, Register::Rax, 8));

    Emit(module, MemoryNew(), "Allocate Memory");
}

void Bytecode::GenerateExpressionMemoryFree(Module* module, Expression* expression)
{
    MemoryExpression* memory = &expression->memory;
    GenerateExpression(module, memory->expression);

    Emit(module, MemoryFree(), "Free Memory");
}

void Bytecode::GenerateExpressionDot(Module* module, Expression* expression)
{
    GenerateAddressInto(module, expression, Register::Rax, true);
    GenerateLoadFrom(module, expression->type, Register::Rax, Register::Rax, true, false);
}

void Bytecode::GenerateConditional(Module* module, Conditional* conditional)
{
    GenerateExpression(module, conditional->condition);
    Emit(module, CmpLE_NToR(0, Register::Rax, static_cast<u8>(conditional->condition->type->size)), "If True Compare");

    // The first jump should always go to the start of "falseBody", if there is no "falseBody" we will simply go out of the if
    i64 firstJumpOpcodeIndex = module->bytecodeInfo.opcodes.size();
    Emit(module, JumpAbsolute(0, true, true), "Condition : Skip True Body");

    GenerateStatement(module, conditional->trueBody);

    if (conditional->falseBody)
    {
        // This JMP exists so that we can skip the "falseBody" if we entered the "trueBody"
        i64 secondJumpOpcodeIndex = module->bytecodeInfo.opcodes.size();
        Emit(module, JumpAbsolute(0), "Condition : Skip False Body");

        // Skip the JMP above if we jumped here from failing the initial conditional check for the if
        {
            i32 startOfFalseBodyIndex = static_cast<i32>(module->bytecodeInfo.opcodes.size());

            JumpAbsolute* jumpAbsolute = reinterpret_cast<JumpAbsolute*>(&module->bytecodeInfo.opcodes[firstJumpOpcodeIndex]);
            jumpAbsolute->address = startOfFalseBodyIndex;
        }

        GenerateStatement(module, conditional->falseBody);

        // Correct the value stored by the second Jmp Opcode
        {
            i32 endOfConditionIndex = static_cast<i32>(module->bytecodeInfo.opcodes.size());

            JumpAbsolute* jumpAbsolute = reinterpret_cast<JumpAbsolute*>(&module->bytecodeInfo.opcodes[secondJumpOpcodeIndex]);
            jumpAbsolute->address = endOfConditionIndex;
        }
    }
    else
    {
        // Correct the value stored by the first Jmp Opcode
        {
            i32 endOfConditionIndex = static_cast<i32>(module->bytecodeInfo.opcodes.size());
            JumpAbsolute* jumpAbsolute = reinterpret_cast<JumpAbsolute*>(&module->bytecodeInfo.opcodes[firstJumpOpcodeIndex]);
            jumpAbsolute->address = endOfConditionIndex;
        }
    }
}

void Bytecode::GenerateLoop(Module* module, Loop* loop)
{
    EnterLoop(module, loop);
    {
        loop->paths = new std::vector<LoopPath>();

        i32 startOfLoopIndex = static_cast<i32>(module->bytecodeInfo.opcodes.size());
        GenerateExpression(module, loop->condition);
        Emit(module, CmpLE_NToR(0, Register::Rax, static_cast<u8>(loop->condition->type->size)), "Loop Compare");

        // Jump past the body if condition isn't met
        i32 endJumpOpcodeIndex = static_cast<i32>(module->bytecodeInfo.opcodes.size());
        Emit(module, JumpAbsolute(0, true, true), "Loop : Exit Jump");

        GenerateStatement(module, loop->body);

        // Jump to the start of the loop to re evalute the condition
        Emit(module, JumpAbsolute(startOfLoopIndex), "Loop : Start Jump");
        i32 endOfLoopIndex = static_cast<i32>(module->bytecodeInfo.opcodes.size());

        // Correct the value stored by the first Jmp Opcode
        {
            JumpAbsolute* jumpAbsolute = reinterpret_cast<JumpAbsolute*>(&module->bytecodeInfo.opcodes[endJumpOpcodeIndex]);
            jumpAbsolute->address = endOfLoopIndex;
        }

        // Correct the value stored by all (continue/break) paths
        for (u32 i = 0; i < loop->paths->size(); i++)
        {
            const LoopPath& loopPath = loop->paths->at(i);

            JumpAbsolute* jumpAbsolute = reinterpret_cast<JumpAbsolute*>(&module->bytecodeInfo.opcodes[loopPath.opcodeDataIndex]);
            if (loopPath.kind == LoopPath::Kind::Continue)
            {
                jumpAbsolute->address = startOfLoopIndex;
            }
            else if (loopPath.kind == LoopPath::Kind::Break)
            {
                jumpAbsolute->address = endOfLoopIndex;
            }
        }
    }
    ExitLoop(module);
}

void Bytecode::GenerateLoopPath(Module* module, LoopPath* loopPath)
{
    switch (loopPath->kind)
    {
        case LoopPath::Kind::Continue:
        {
            Emit(module, JumpAbsolute(0), "Loop : Continue");
            break;
        }
        case LoopPath::Kind::Break:
        {
            Emit(module, JumpAbsolute(0), "Loop : Break");
            break;
        }

        default:
        {
            DebugHandler::PrintError("Bytecode : Unhandled LoopPath::Kind(%u)", loopPath->kind);
            exit(1);
        }
    }
}

u64 Bytecode::GetAllocationSize(Type* type)
{
    if (type->kind == Type::Kind::Pointer && type->pointer.count)
    {
        assert(type->pointer.type);
        return type->pointer.count * GetAllocationSize(type->pointer.type);
    }
    else
    {
        return type->size;
    }
}

u64 Bytecode::GetAllocationAlignment(u64 number, u64 alignment)
{
    u64 offset = number % alignment;
    if (offset)
    {
       return number + alignment - offset;
    }

    return number;
}

i64 Bytecode::GenerateFunctionVariableOffsets(Module* module, Scope* scope)
{
    i64 variableStackSize = 0;
    GenerateScopeVariableOffsets(module, scope, variableStackSize);

    return variableStackSize;
}

Register Bytecode::GetParameterRegister(Module* module, Declaration* decl)
{
    assert(module->bytecodeInfo.currentFunction);
    assert(decl && decl->kind == Declaration::Kind::Variable);

    Scope* currentFnScope = module->bytecodeInfo.currentFunction->scope;

    ListNode* node;
    u64 parameterCount = 0;

    ListIterate(&currentFnScope->declarations, node)
    {
        Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);

        if (declaration->kind == Declaration::Kind::Variable)
        {
            parameterCount++;

            if (decl->token->nameHash.hash == declaration->token->nameHash.hash)
            {
                switch (parameterCount)
                {
                    case 1: return Register::Rcx;
                    case 2: return Register::Rdx;
                    case 3: return Register::R8;
                    case 4: return Register::R9;

                    default: break;
                }
            }

            if (parameterCount == 4)
                break;
        }
    }

    return Register::None;
}

void Bytecode::GenerateScopeVariableOffsets(Module* module, Scope* scope, i64& offset)
{
    assert(scope);

    ListNode* node;
    if (scope->parent == module->parserInfo.block->scope)
    {
        u32 numParameters = 0;

        ListIterate(&scope->declarations, node)
        {
            Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);
            if (declaration->kind == Declaration::Kind::Variable)
                numParameters++;
        }

        Declaration* currFuncDecl = reinterpret_cast<Declaration*>(module->bytecodeInfo.currentFunction);
        u32 fnHash = currFuncDecl->token->nameHash.hash;

        FunctionParamInfo& paramInfo = module->bytecodeInfo.functionHashToParamInfo[fnHash];
        paramInfo.declarations.resize(numParameters);

        ListIterateReverse(&scope->declarations, node)
        {
            Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);
            if (declaration->kind == Declaration::Kind::Variable)
            {
                assert(declaration->type);
                Type* type = declaration->type;

                paramInfo.declarations[numParameters - 1] = declaration;

                if (numParameters-- <= 4)
                {
                    offset += GetAllocationSize(type);
                    offset = GetAllocationAlignment(offset, type->alignment);
                }
                else
                {
                    offset += 8;
                    offset = GetAllocationAlignment(offset, 8);
                }

                declaration->variable.offset = offset;

#if NAI_DEBUG
                DebugHandler::PrintSuccess("Placed Variable(%.*s) at Offset(%d)", declaration->token->nameHash.length, declaration->token->nameHash.name, declaration->variable.offset);
#endif // NAI_DEBUG
            }
        }
    }

    ListIterate(&scope->scopeChildren, node)
    {
        Scope* subScope = ListGetStructPtr(node, Scope, listNode);
        GenerateScopeVariableOffsets(module, subScope, offset);
    }

    if (scope->parent != module->parserInfo.block->scope)
    {
        ListIterateReverse(&scope->declarations, node)
        {
            Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);

            if (declaration->kind == Declaration::Kind::Variable)
            {
                assert(declaration->type);

                Type* type = declaration->type;

                offset += GetAllocationSize(type);
                offset = GetAllocationAlignment(offset, type->alignment);

                declaration->variable.offset = offset;
#if NAI_DEBUG
                DebugHandler::PrintSuccess("Placed Variable(%.*s) at Offset(%d)", declaration->token->nameHash.length, declaration->token->nameHash.name, declaration->variable.offset);
#endif // NAI_DEBUG
            }
        }
    }
}
