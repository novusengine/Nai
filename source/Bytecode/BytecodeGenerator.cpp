#include <pch/Build.h>
#include "BytecodeGenerator.h"
#include <atomic>

bool BytecodeGenerator::Run(ModuleInfo& moduleInfo)
{
    const std::vector<ASTFunctionDecl*> fnNodes = moduleInfo.GetFunctionNodes();

    std::atomic<uint16_t> registryCount = 0;

    for (ASTFunctionDecl* fnDecl : fnNodes)
    {
        uint16_t registerNum = 0;

        std::vector<ByteInstruction*>& instructions = fnDecl->GetInstructions();

        // Generate Function Body
        Generate_FuncHeader(fnDecl, registerNum);

        // Generate Function Body
        Generate_FuncBody(moduleInfo, fnDecl, registerNum);

        // Ensure the last instruction is always a return
        ByteInstruction* instruction = instructions.back();
        if (!instruction || instruction->opcode != ByteOpcode::RETURN)
        {
            ByteInstruction* returnInstruction = moduleInfo.GetByteInstruction();
            returnInstruction->opcode = ByteOpcode::RETURN;
            fnDecl->AddInstruction(returnInstruction);
        }

        // Make sure we know the max amount of Registries we need for this module
        if (registerNum > registryCount)
            registryCount = registerNum;
    }

    moduleInfo.SetRegistryCount(registryCount);
    return true;
}

void BytecodeGenerator::Generate_FuncHeader(ASTFunctionDecl* fnDecl, uint16_t& registerNum)
{
    // Set RegisterIndex for Function Return
    if (fnDecl->returnType->GetType() != NaiType::NAI_VOID)
        registerNum++;

    // Set RegisterIndex for Function Parameters
    for (ASTFunctionParameter* param : fnDecl->GetParameters())
    {
        param->registerIndex = registerNum++;
    }

    // Set RegisterIndex for Function Variables
    for (ASTVariable* var : fnDecl->GetVariables())
    {
        bool isParent = !var->parent;

        var->registerIndex = registerNum * isParent;
        registerNum += 1 * isParent;
    }
}
void BytecodeGenerator::Generate_FuncBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum)
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
        /*else if (left->type == ASTNodeType::FUNCTION_CALL)
        {
            ASTFunctionCall* fnCall = static_cast<ASTFunctionCall*>(left);
            ASTFunctionDecl* functionDecl = moduleInfo.GetFunctionByNameHash(fnCall->GetNameHashed());
        }*/
        else if (left->type == ASTNodeType::WHILE_STATEMENT)
        {
            ASTWhileStatement* whileStmt = static_cast<ASTWhileStatement*>(left);
            Generate_WhileStmt(moduleInfo, fnDecl, registerNum, whileStmt);
        }
        else if (left->type == ASTNodeType::IF_STATEMENT)
        {
            ASTIfStatement* ifStmt = static_cast<ASTIfStatement*>(left);
            Generate_IfStmt(moduleInfo, fnDecl, registerNum, 0, nullptr, ifStmt);
        }
        else if (left->type == ASTNodeType::RETURN_STATEMENT)
        {
            ASTReturnStatement* returnStmt = static_cast<ASTReturnStatement*>(left);
            Generate_ReturnStmt(moduleInfo, fnDecl, returnStmt);
        }

        nextSequence = nextSequence->right;
    }
}

void BytecodeGenerator::Generate_Variable(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTVariable* var)
{
    if (ASTExpression* expression = var->expression)
    {
        // I have aligned the ASTOperaterType & ByteOpcode as such that they map 1:1 for ASTNodeType::VALUE, and ASTNodeType::Variable we add + 5
        ByteInstruction* byteInstruction = moduleInfo.GetByteInstruction();
        byteInstruction->UpdateOpcode(static_cast<ByteOpcode>(expression->op), expression->left->type == ASTNodeType::VARIABLE);

        uint64_t value = GetExpressionValueFromNode(expression->left);

        byteInstruction->val1 = value;
        byteInstruction->val2 = var->GetRegistryIndex();

        fnDecl->AddInstruction(byteInstruction);
    }
}
void BytecodeGenerator::Generate_Expression(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, ASTExpression* expression)
{
    // TODO: For now we dumbly assume there can be a single value on the left or a value on the left & right, we don't recursively parse expressions

    ByteInstruction* leftExpressionInstruction = moduleInfo.GetByteInstruction();
    leftExpressionInstruction->UpdateOpcode(ByteOpcode::MOVE_TO_REG, expression->left->type == ASTNodeType::VARIABLE);
    
    uint64_t leftValue = GetExpressionValueFromNode(expression->left);

    uint16_t valueRegister = registerNum++;
    leftExpressionInstruction->val1 = leftValue;
    leftExpressionInstruction->val2 = valueRegister;
    fnDecl->AddInstruction(leftExpressionInstruction);

    // If we have an operator, it means we must handle the right side of the expression
    if (expression->op != ASTOperatorType::NONE)
    {
        ByteInstruction* rightExpressionInstruction = moduleInfo.GetByteInstruction();
        rightExpressionInstruction->UpdateOpcode(static_cast<ByteOpcode>(expression->op), expression->right->type == ASTNodeType::VARIABLE);

        uint64_t rightValue = GetExpressionValueFromNode(expression->right);

        rightExpressionInstruction->val1 = rightValue;
        rightExpressionInstruction->val2 = valueRegister;
        fnDecl->AddInstruction(rightExpressionInstruction);
    }
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

void BytecodeGenerator::Generate_ReturnStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, ASTReturnStatement* returnStmt)
{
    if (returnStmt->value)
    {
        // Handle Expression (For now we assume we can only return a single literal/variable
        ASTExpression* returnExpression = returnStmt->value;

        ByteInstruction* byteInstruction = moduleInfo.GetByteInstruction();
        byteInstruction->opcode = static_cast<ByteOpcode>(static_cast<uint8_t>(ByteOpcode::MOVE_TO_REG) + 5 * (returnExpression->left->type == ASTNodeType::VARIABLE));

        uint64_t returnValue = 0;
        if (returnExpression->left->type == ASTNodeType::VALUE)
        {
            ASTValue* val = static_cast<ASTValue*>(returnExpression->left);
            returnValue = val->value;
        }
        else if (returnExpression->left->type == ASTNodeType::VARIABLE)
        {
            ASTVariable* val = static_cast<ASTVariable*>(returnExpression->left);
            returnValue = val->GetRegistryIndex();
        }

        byteInstruction->val1 = returnValue;
        byteInstruction->val2 = 0; // Return Values goes into the 0th registry (relative to the function)
        fnDecl->AddInstruction(byteInstruction);
    }

    ByteInstruction* returnInstruction = moduleInfo.GetByteInstruction();
    returnInstruction->opcode = ByteOpcode::RETURN;
    fnDecl->AddInstruction(returnInstruction);
}
void BytecodeGenerator::Generate_IfStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTIfStatement* stmt)
{
    uint64_t* instructionPtr = nullptr;

    // Parse If Condition
    Generate_IfHead(moduleInfo, fnDecl, registerNum, &instructionPtr, stmt->condition);

    // Parse If Body
    Generate_IfBody(moduleInfo, fnDecl, registerNum, loopStartIndex, breakJmpPtrs, stmt->body);

    // Set the Instruction Index for where to jump if condition is false
    *instructionPtr = fnDecl->GetInstructions().size();
}
void BytecodeGenerator::Generate_IfHead(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, uint64_t** nextInstructionIndex, ASTExpression* expression)
{
    Generate_Expression(moduleInfo, fnDecl, registerNum, expression);

    ByteInstruction* jmpToEndConditionalInstruction = moduleInfo.GetByteInstruction();
    jmpToEndConditionalInstruction->opcode = ByteOpcode::JMP_CONDITIONAL;
    jmpToEndConditionalInstruction->val1 = 0; // This is set later
    jmpToEndConditionalInstruction->val2 = 0; // JMP if last comparison was false
    fnDecl->AddInstruction(jmpToEndConditionalInstruction);

    *nextInstructionIndex = &jmpToEndConditionalInstruction->val1;
}
void BytecodeGenerator::Generate_IfBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, size_t loopStartIndex, std::vector<uint64_t*>* breakJmpPtrs, ASTSequence* body)
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
        else if (left->type == ASTNodeType::IF_STATEMENT)
        {
            ASTIfStatement* ifStmt = static_cast<ASTIfStatement*>(left);
            Generate_IfStmt(moduleInfo, fnDecl, registerNum, loopStartIndex, breakJmpPtrs, ifStmt);
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
}
void BytecodeGenerator::Generate_WhileStmt(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, ASTWhileStatement* stmt)
{
    std::vector<ByteInstruction*>& instructions = fnDecl->GetInstructions();

    std::vector<uint64_t*> breakInstructionIndexPtrs;
    breakInstructionIndexPtrs.reserve(8);

    // Parse While Condition
    Generate_LoopHead(moduleInfo, fnDecl, registerNum, breakInstructionIndexPtrs, stmt->condition);

    size_t loopStartIndex = instructions.size() - 2; // Size() - 2 will give us the "Compare" instruction generated in "Generate_LoopHead" through "Generate_Expression"

    // Parse While Body
    Generate_LoopBody(moduleInfo, fnDecl, registerNum, loopStartIndex, breakInstructionIndexPtrs, stmt->body);

    // Set the Instruction Index for where to jump if condition is false
    size_t loopEndIndex = instructions.size();

    // Set the Break Instruction Index for where to jump
    for (uint64_t* index : breakInstructionIndexPtrs)
    {
        *index = loopEndIndex;
    }
}

void BytecodeGenerator::Generate_LoopHead(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, std::vector<uint64_t*>& breakJmpPtrs, ASTExpression* expression)
{
    Generate_Expression(moduleInfo, fnDecl, registerNum, expression);

    ByteInstruction* jmpToEndConditionalInstruction = moduleInfo.GetByteInstruction();
    jmpToEndConditionalInstruction->opcode = ByteOpcode::JMP_CONDITIONAL;
    jmpToEndConditionalInstruction->val1 = 0; // This is set later
    jmpToEndConditionalInstruction->val2 = 0; // Jump if last comparison was false
    fnDecl->AddInstruction(jmpToEndConditionalInstruction);

    breakJmpPtrs.push_back(&jmpToEndConditionalInstruction->val1);
}
void BytecodeGenerator::Generate_LoopBody(ModuleInfo& moduleInfo, ASTFunctionDecl* fnDecl, uint16_t& registerNum, size_t loopStartIndex, std::vector<uint64_t*>& breakJmpPtrs, ASTSequence* body)
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
        else if (left->type == ASTNodeType::IF_STATEMENT)
        {
            ASTIfStatement* ifStmt = static_cast<ASTIfStatement*>(left);
            Generate_IfStmt(moduleInfo, fnDecl, registerNum, loopStartIndex, &breakJmpPtrs, ifStmt);
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
