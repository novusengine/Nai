#include <pch/Build.h>
#include "BytecodeGenerator.h"
#include <atomic>

bool BytecodeGenerator::Run(ModuleInfo& moduleInfo)
{
    const std::vector<ASTFunctionDecl*> fnNodes = moduleInfo.GetFunctionNodes();

    for (ASTFunctionDecl* fnDecl : fnNodes)
    {
        Generate_Func(moduleInfo, fnDecl);
    }

    // Calculate the amount of registries we need
    std::atomic<uint16_t> registryCount = 0;

    for (ASTFunctionDecl* fnDecl : fnNodes)
    {
        uint16_t registerNum = GetRegisterNumForFunction(moduleInfo, fnDecl);

        if (registerNum > registryCount)
        {
            registryCount = registerNum;
        }
    }

    moduleInfo.SetRegistryCount(registryCount - 1);
    return true;
}

uint16_t BytecodeGenerator::GetRegisterNumForFunction(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl)
{
    uint16_t registerNum = fnDecl->registerNum;

    for (ByteInstruction* instruction : fnDecl->GetInstructions())
    {
        if (instruction->opcode == ByteOpcode::CALL)
        {
            ASTFunctionDecl* fnDecl = moduleInfo.GetFunctionByNameHash(instruction->val1);
            registerNum += GetRegisterNumForFunction(moduleInfo, fnDecl);
        }
    }

    return registerNum;
}

void BytecodeGenerator::Generate_Func(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl)
{
    std::vector<ByteInstruction*>& instructions = fnDecl->GetInstructions();

    // Generate Function Body
    Generate_FuncHeader(fnDecl);

    // Generate Function Body
    Generate_FuncBody(moduleInfo, fnDecl);

    // Ensure the last instruction is always a return
    ByteInstruction* instruction = instructions.back();
    if (!instruction || instruction->opcode != ByteOpcode::RETURN)
    {
        ByteInstruction* returnInstruction = moduleInfo.GetByteInstruction();
        returnInstruction->opcode = ByteOpcode::RETURN;
        fnDecl->AddInstruction(returnInstruction);
    }
}

void BytecodeGenerator::Generate_FuncHeader(ASTFunctionDecl* fnDecl)
{
    // Set RegisterIndex for Function Parameters
    for (ASTVariable* param : fnDecl->GetParameters())
    {
        param->registerIndex = fnDecl->GetRegisterIndex();
    }

    // Set RegisterIndex for Function Variables
    for (ASTVariable* var : fnDecl->GetVariables())
    {
        if (!var->parent)
        {
            var->registerIndex = fnDecl->GetRegisterIndex();
        }
    }
}
void BytecodeGenerator::Generate_FuncBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl)
{
    ASTSequence* nextSequence = fnDecl->body;
    while (nextSequence->left)
    {
        ASTNode* left = nextSequence->left;

        if (left->type == ASTNodeType::VARIABLE)
        {
            ASTVariable* variable = static_cast<ASTVariable*>(left);
            Generate_Variable(moduleInfo, fnDecl, variable);
        }
        else if (left->type == ASTNodeType::FUNCTION_CALL)
        {
            Generate_FuncCall(moduleInfo, fnDecl, static_cast<ASTFunctionCall*>(left));
        }
        else if (left->type == ASTNodeType::WHILE_STATEMENT)
        {
            ASTWhileStatement* whileStmt = static_cast<ASTWhileStatement*>(left);
            Generate_WhileStmt(moduleInfo, fnDecl, whileStmt);
        }
        else if (left->type == ASTNodeType::IF_STATEMENT)
        {
            ASTIfStatement* ifStmt = static_cast<ASTIfStatement*>(left);
            Generate_IfStmtChain(moduleInfo, fnDecl, 0, nullptr, ifStmt);
        }
        else if (left->type == ASTNodeType::RETURN_STATEMENT)
        {
            ASTReturnStatement* returnStmt = static_cast<ASTReturnStatement*>(left);
            Generate_ReturnStmt(moduleInfo, fnDecl, returnStmt);
        }

        nextSequence = nextSequence->right;
    }
}
void BytecodeGenerator::Generate_FuncCall(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTFunctionCall* fnCall)
{
    auto fnArgs = fnCall->GetArguments();
    auto fnArgsNum = fnArgs.size();
    std::vector<uint16_t> argsResultRegisterNums;

    // Resolve Argument values
    if (fnArgsNum > 0)
    {
        argsResultRegisterNums.reserve(fnArgsNum);
        for (ASTFunctionArgument* arg : fnArgs)
        {
            ASTNode* node = arg->value;

            if (node->type == ASTNodeType::VARIABLE)
            {
                ASTVariable* variable = static_cast<ASTVariable*>(node);
                argsResultRegisterNums.push_back(variable->registerIndex);
            }
            else if (node->type == ASTNodeType::EXPRESSION)
            {
                ASTExpression* expression = static_cast<ASTExpression*>(node);
                Generate_Expression(moduleInfo, fnDecl, expression);

                argsResultRegisterNums.push_back(fnDecl->registerOffset - 1);
            }
            else if (node->type == ASTNodeType::FUNCTION_CALL)
            {
                Generate_FuncCall(moduleInfo, fnDecl, static_cast<ASTFunctionCall*>(node));

                // registerNum <- Return Value
                argsResultRegisterNums.push_back(fnDecl->registerOffset - 1);
            }
        }
    }

    uint16_t registerOffset = fnDecl->registerOffset;

    // Put Argument values into registers
    for (uint16_t& argRegisterIndex : argsResultRegisterNums)
    {
        ByteInstruction* addParameterInstruction = moduleInfo.GetByteInstruction();
        addParameterInstruction->opcode = ByteOpcode::MOVE_REG_TO_REG;
        addParameterInstruction->val1 = argRegisterIndex;
        addParameterInstruction->val2 = fnDecl->registerOffset++;
        fnDecl->AddInstruction(addParameterInstruction);
    }

    // Push Instruction to add to ByteContext Register Offset
    ByteInstruction* addRegisterOffsetInstruction = moduleInfo.GetByteInstruction();
    addRegisterOffsetInstruction->opcode = ByteOpcode::ADD_REGISTER_OFFSET;
    addRegisterOffsetInstruction->val1 = registerOffset;
    fnDecl->AddInstruction(addRegisterOffsetInstruction);

    // Call Function
    ByteInstruction* callFunctionInstruction = moduleInfo.GetByteInstruction();
    callFunctionInstruction->opcode = ByteOpcode::CALL;
    callFunctionInstruction->val1 = static_cast<uint64_t>(fnCall->GetNameHashed());
    fnDecl->AddInstruction(callFunctionInstruction);
    // # Note: Return value will be located at the 0th register relative to the register offset

    // Push Instruction to subtract from ByteContext Register Offset
    ByteInstruction* subtractRegisterOffsetInstruction = moduleInfo.GetByteInstruction();
    subtractRegisterOffsetInstruction->opcode = ByteOpcode::SUBTRACT_REGISTER_OFFSET;
    subtractRegisterOffsetInstruction->val1 = addRegisterOffsetInstruction->val1;
    fnDecl->AddInstruction(subtractRegisterOffsetInstruction);

    fnDecl->registerOffset = registerOffset + 1;
}

void BytecodeGenerator::Generate_Variable(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTVariable* var)
{
    if (var->expression)
    {
        ByteInstruction* setVariableValueInstruction = moduleInfo.GetByteInstruction();
        setVariableValueInstruction->opcode = ByteOpcode::MOVE_REG_TO_REG;

        setVariableValueInstruction->val1 = Generate_Expression(moduleInfo, fnDecl, var->expression);
        setVariableValueInstruction->val2 = var->registerIndex;
        fnDecl->AddInstruction(setVariableValueInstruction);
    }
}
uint16_t BytecodeGenerator::Generate_Expression(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTExpression* expression)
{
    /* 
        Expressions work in the following way. 
        - The left & right node may contain an expression sequence, literal, variable or function call
        - If an expression is seen in the left node, it is a sub expression (An expression that is wrapped in parenthesis)
        - If an expression is seen in the right node, it is an expression sequence (The next part of the expression)
        - Literal, Variables and function calls are treated equally on both sides

        How does Generate_Expression work?
        - We ensure that the first part of the expression is calculated last, this is allows us to properly manage
          the registries so we don't have to do any complicated resolving.

        Lets take the following example (5 + 4 + 3 + 2 + 1)
        - In this example the calculation order would look like this
          - 2 + 1
          - 3 + 3
          - 4 + 6
          - 5 + 10
          - Result would be 15

        The right node is always "calculated" first, meaning if it is a function call or expression we resolve it
        then we take the value we "calculate" the left node and run the operator on them.

        Taking what we know so far, the function's logic tree should look similar to this
        - Check if the expression has an operator
            - if true, produce the right node

        - Produce the left node
        - Produce instructions for the left node
        - Produce instructions for the right node (If the right node exists)
    */

    // Check if we need to evaluate the right hand side

    bool hasRightNode = expression->op != ASTOperatorType::NONE;
    assert(hasRightNode ? expression->right != nullptr : expression->right == nullptr);

    uint64_t leftNodeRegisterNum = 0;
    uint64_t rightNodeRegisterNum = 0;

    if (hasRightNode)
    {
        rightNodeRegisterNum = Generate_ExpressionNode(moduleInfo, fnDecl, expression->right);
    }

    leftNodeRegisterNum = Generate_ExpressionNode(moduleInfo, fnDecl, expression->left);

    // Handle operater
    if (hasRightNode)
    {
        ByteInstruction* operaterInstruction = moduleInfo.GetByteInstruction();
        operaterInstruction->UpdateOpcode(static_cast<ByteOpcode>(expression->op), true);

        operaterInstruction->val1 = rightNodeRegisterNum;
        operaterInstruction->val2 = leftNodeRegisterNum;
        fnDecl->AddInstruction(operaterInstruction);
    }

    return leftNodeRegisterNum;
}
uint16_t BytecodeGenerator::Generate_ExpressionNode(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTNode* node)
{
    if (node->type == ASTNodeType::VALUE || node->type == ASTNodeType::VARIABLE)
    {
        ByteInstruction* rightExpressionInstruction = moduleInfo.GetByteInstruction();
        rightExpressionInstruction->UpdateOpcode(ByteOpcode::MOVE_TO_REG, node->type == ASTNodeType::VARIABLE);

        rightExpressionInstruction->val1 = GetExpressionValueFromNode(node);
        rightExpressionInstruction->val2 = fnDecl->GetRegisterIndex();
        fnDecl->AddInstruction(rightExpressionInstruction);
    }
    else if (node->type == ASTNodeType::FUNCTION_CALL)
    {
        Generate_FuncCall(moduleInfo, fnDecl, static_cast<ASTFunctionCall*>(node));
    }
    else if (node->type == ASTNodeType::EXPRESSION)
    {
        Generate_Expression(moduleInfo, fnDecl, static_cast<ASTExpression*>(node));
    }

    return fnDecl->registerOffset - 1; // Return Node register
}
uint64_t BytecodeGenerator::GetExpressionValueFromNode(ASTNode* node)
{
    uint64_t value = 0;
    if (node->type == ASTNodeType::VALUE)
    {
        ASTValue* val = static_cast<ASTValue*>(node);
        value = val->value;
    }
    else if (node->type == ASTNodeType::VARIABLE)
    {
        ASTVariable* val = static_cast<ASTVariable*>(node);
        value = val->GetRegistryIndex();
    }

    return value;
}

void BytecodeGenerator::Generate_ReturnStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTReturnStatement* stmt)
{
    if (stmt->value)
    {
        Generate_Expression(moduleInfo, fnDecl, stmt->value);

        std::vector<ByteInstruction*>& instructions = fnDecl->GetInstructions();
        ByteInstruction* instruction = instructions.back();

        // Return Value always goes into 0th registry, this is because when we have function calls within a function
        // we are going to ignore all registers within the function call once we return, resulting in us placing the
        // return value into the first Registry of that function call.
        ByteInstruction* returnValueInstruction = moduleInfo.GetByteInstruction();
        returnValueInstruction->opcode = ByteOpcode::MOVE_REG_TO_REG;
        returnValueInstruction->val1 = instruction->val2;
        returnValueInstruction->val2 = 0;
        fnDecl->AddInstruction(returnValueInstruction);
    }

    ByteInstruction* returnInstruction = moduleInfo.GetByteInstruction();
    returnInstruction->opcode = ByteOpcode::RETURN;
    fnDecl->AddInstruction(returnInstruction);
}
void BytecodeGenerator::Generate_IfStmtChain(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTIfStatement* stmt)
{
    std::vector<ByteInstruction*>& instructions = fnDecl->GetInstructions();

    std::vector<uint64_t*> escapeJmpPtrs;
    escapeJmpPtrs.reserve(8);

    Generate_IfStmt(moduleInfo, fnDecl, &escapeJmpPtrs, loopStartIndex, breakJmpPtrs, stmt);

    // Set the Instruction Index for Escape Jmp
    size_t ifChainEndIndex = instructions.size();

    // Set the Break Instruction Index for where to jump
    for (uint64_t* index : escapeJmpPtrs)
    {
        *index = ifChainEndIndex;
    }
}
void BytecodeGenerator::Generate_IfStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, std::vector<uint64_t*>* escapeJmpPtrs, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTIfStatement* stmt)
{
    uint64_t* instructionPtr = nullptr;

    if (stmt->ifType != IFStatementType::ELSE)
    {
        // Parse If Condition
        Generate_IfHead(moduleInfo, fnDecl, &instructionPtr, stmt->condition);
    }

    // Parse If Body
    Generate_IfBody(moduleInfo, fnDecl, escapeJmpPtrs, loopStartIndex, breakJmpPtrs, stmt);

    if (stmt->ifType != IFStatementType::ELSE)
    {
        // Set the Instruction Index for where to jump if condition is false (This will automatically align with the next statement if one exists
        *instructionPtr = fnDecl->GetInstructions().size();
    }
    
    // This will never be true if "stmt->type == IFStatementType::ELSE" because the parser prevents the else from having a "->next"
    if (stmt->next)
    {
        Generate_IfStmt(moduleInfo, fnDecl, escapeJmpPtrs, loopStartIndex, breakJmpPtrs, stmt->next);
    }
}
void BytecodeGenerator::Generate_IfHead(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint64_t** nextInstructionIndex, ASTExpression* expression)
{
    Generate_Expression(moduleInfo, fnDecl, expression);

    ByteInstruction* jmpToEndConditionalInstruction = moduleInfo.GetByteInstruction();
    jmpToEndConditionalInstruction->opcode = ByteOpcode::JMP_CONDITIONAL;
    jmpToEndConditionalInstruction->val1 = 0; // This is set later
    jmpToEndConditionalInstruction->val2 = 0; // JMP if last comparison was false
    fnDecl->AddInstruction(jmpToEndConditionalInstruction);

    *nextInstructionIndex = &jmpToEndConditionalInstruction->val1;
}
void BytecodeGenerator::Generate_IfBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, std::vector<uint64_t*>* escapeJmpPtrs, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTIfStatement* stmt)
{
    ASTSequence* nextSequence = stmt->body;
    while (nextSequence->left)
    {
        ASTNode* left = nextSequence->left;

        if (left->type == ASTNodeType::VARIABLE)
        {
            ASTVariable* variable = static_cast<ASTVariable*>(left);
            Generate_Variable(moduleInfo, fnDecl, variable);
        }
        else if (left->type == ASTNodeType::FUNCTION_CALL)
        {
            Generate_FuncCall(moduleInfo, fnDecl, static_cast<ASTFunctionCall*>(left));
        }
        else if (left->type == ASTNodeType::IF_STATEMENT)
        {
            ASTIfStatement* ifStmt = static_cast<ASTIfStatement*>(left);
            Generate_IfStmtChain(moduleInfo, fnDecl, loopStartIndex, breakJmpPtrs, ifStmt);
        }
        else if (left->type == ASTNodeType::RETURN_STATEMENT)
        {
            ASTReturnStatement* returnStmt = static_cast<ASTReturnStatement*>(left);
            Generate_ReturnStmt(moduleInfo, fnDecl, returnStmt);
        }
        else if (left->type == ASTNodeType::JMP_STATEMENT)
        {
            ASTJmpStatement* jmp = static_cast<ASTJmpStatement*>(left);

            if (jmp->jmpType == JMPStatementType::CONTINUE)
            {
                ByteInstruction* continueInstruction = moduleInfo.GetByteInstruction();
                continueInstruction->opcode = ByteOpcode::JMP;
                continueInstruction->val1 = loopStartIndex;
                fnDecl->AddInstruction(continueInstruction);
            }
            else if (jmp->jmpType == JMPStatementType::BREAK)
            {
                ByteInstruction* breakInstruction = moduleInfo.GetByteInstruction();
                breakInstruction->opcode = ByteOpcode::JMP;
                breakInstruction->val1 = 0; // This is set later
                fnDecl->AddInstruction(breakInstruction);

                breakJmpPtrs->push_back(&breakInstruction->val1);
            }
        }

        nextSequence = nextSequence->right;
    }

    // Place Escape Jump for If Statement
    if (stmt->next)
    {
        ByteInstruction* escapeJmpInstruction = moduleInfo.GetByteInstruction();
        escapeJmpInstruction->opcode = ByteOpcode::JMP;
        escapeJmpInstruction->val1 = 0; // This is set later
        fnDecl->AddInstruction(escapeJmpInstruction);

        escapeJmpPtrs->push_back(&escapeJmpInstruction->val1);
    }
}
void BytecodeGenerator::Generate_WhileStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTWhileStatement* stmt)
{
    std::vector<ByteInstruction*>& instructions = fnDecl->GetInstructions();

    std::vector<uint64_t*> breakInstructionIndexPtrs;
    breakInstructionIndexPtrs.reserve(8);

    // Parse While Condition
    Generate_LoopHead(moduleInfo, fnDecl, breakInstructionIndexPtrs, stmt->condition);

    size_t loopStartIndex = instructions.size() - 2; // Size() - 2 will give us the "Compare" instruction generated in "Generate_LoopHead" through "Generate_Expression"

    // Parse While Body
    Generate_LoopBody(moduleInfo, fnDecl, loopStartIndex, breakInstructionIndexPtrs, stmt->body);

    // Set the Instruction Index for where to jump if condition is false
    size_t loopEndIndex = instructions.size();

    // Set the Break Instruction Index for where to jump
    for (uint64_t* index : breakInstructionIndexPtrs)
    {
        *index = loopEndIndex;
    }
}

void BytecodeGenerator::Generate_LoopHead(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, std::vector<uint64_t*>& breakJmpPtrs, ASTExpression* expression)
{
    Generate_Expression(moduleInfo, fnDecl, expression);

    ByteInstruction* jmpToEndConditionalInstruction = moduleInfo.GetByteInstruction();
    jmpToEndConditionalInstruction->opcode = ByteOpcode::JMP_CONDITIONAL;
    jmpToEndConditionalInstruction->val1 = 0; // This is set later
    jmpToEndConditionalInstruction->val2 = 0; // Jump if last comparison was false
    fnDecl->AddInstruction(jmpToEndConditionalInstruction);

    breakJmpPtrs.push_back(&jmpToEndConditionalInstruction->val1);
}
void BytecodeGenerator::Generate_LoopBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, size_t loopStartIndex, std::vector<uint64_t*>& breakJmpPtrs, ASTSequence* body)
{
    ASTSequence* nextSequence = body;
    while (nextSequence->left)
    {
        ASTNode* left = nextSequence->left;

        if (left->type == ASTNodeType::VARIABLE)
        {
            ASTVariable* variable = static_cast<ASTVariable*>(left);
            Generate_Variable(moduleInfo, fnDecl, variable);
        }
        else if (left->type == ASTNodeType::FUNCTION_CALL)
        {
            Generate_FuncCall(moduleInfo, fnDecl, static_cast<ASTFunctionCall*>(left));
        }
        else if (left->type == ASTNodeType::IF_STATEMENT)
        {
            ASTIfStatement* ifStmt = static_cast<ASTIfStatement*>(left);
            Generate_IfStmtChain(moduleInfo, fnDecl, loopStartIndex, &breakJmpPtrs, ifStmt);
        }
        else if (left->type == ASTNodeType::RETURN_STATEMENT)
        {
            ASTReturnStatement* returnStmt = static_cast<ASTReturnStatement*>(left);
            Generate_ReturnStmt(moduleInfo, fnDecl, returnStmt);
        }
        else if (left->type == ASTNodeType::JMP_STATEMENT)
        {
            ASTJmpStatement* jmp = static_cast<ASTJmpStatement*>(left);

            if (jmp->jmpType == JMPStatementType::CONTINUE)
            {
                ByteInstruction* continueInstruction = moduleInfo.GetByteInstruction();
                continueInstruction->opcode = ByteOpcode::JMP;
                continueInstruction->val1 = loopStartIndex;
                fnDecl->AddInstruction(continueInstruction);
            }
            else if (jmp->jmpType == JMPStatementType::BREAK)
            {
                ByteInstruction* breakInstruction = moduleInfo.GetByteInstruction();
                breakInstruction->opcode = ByteOpcode::JMP;
                breakInstruction->val1 = 0; // This is set later
                fnDecl->AddInstruction(breakInstruction);

                breakJmpPtrs.push_back(&breakInstruction->val1);
            }
        }

        nextSequence = nextSequence->right;
    }

    ByteInstruction* jmpToStartInstruction = moduleInfo.GetByteInstruction();
    jmpToStartInstruction->opcode = ByteOpcode::JMP;
    jmpToStartInstruction->val1 = loopStartIndex;
    fnDecl->AddInstruction(jmpToStartInstruction);
}
