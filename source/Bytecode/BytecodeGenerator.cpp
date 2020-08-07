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

        ASTSequence* nextSequence = fnDecl->body;
        while (nextSequence->left)
        {
            ASTNode* left = nextSequence->left;

            if (left->type == ASTNodeType::VARIABLE)
            {
                ASTVariable* variable = static_cast<ASTVariable*>(left);

                if (ASTExpression* expression = variable->expression)
                {
                    ByteInstruction* byteInstruction = moduleInfo.GetByteInstruction();

                    // I have aligned the ASTOperaterType & ByteOpcode as such that they map 1:1 for ASTNodeType::VALUE, and ASTNodeType::Variable we add + 5
                    byteInstruction->opcode = static_cast<ByteOpcode>(static_cast<uint8_t>(expression->op) + 5 * (expression->left->type == ASTNodeType::VARIABLE));

                    uint64_t value = 0;
                    if (expression->left->type == ASTNodeType::VALUE)
                    {
                        ASTValue* val = static_cast<ASTValue*>(expression->left);
                        value = val->value;
                    }
                    else if (expression->left->type == ASTNodeType::VARIABLE)
                    {
                        ASTVariable* val = static_cast<ASTVariable*>(expression->left);
                        value = val->GetRegistryIndex();
                    }

                    byteInstruction->val1 = value;
                    byteInstruction->val2 = variable->GetRegistryIndex();

                    fnDecl->AddInstruction(byteInstruction);
                }
            }
            /*else if (left->type == ASTNodeType::FUNCTION_CALL)
            {
                ASTFunctionCall* fnCall = static_cast<ASTFunctionCall*>(left);
                ASTFunctionDecl* functionDecl = moduleInfo.GetFunctionByNameHash(fnCall->GetNameHashed());
            }*/
            else if (left->type == ASTNodeType::WHILE_STATEMENT)
            {
                ASTWhileStatement* whileStmt = static_cast<ASTWhileStatement*>(left);
                ASTExpression* expression = whileStmt->condition;

                ByteInstruction* conditionInstruction = moduleInfo.GetByteInstruction();
                conditionInstruction->opcode = static_cast<ByteOpcode>(static_cast<uint8_t>(ByteOpcode::MOVE_TO_REG) + 5 * (expression->left->type == ASTNodeType::VARIABLE));

                uint64_t value = 0;
                if (expression->left->type == ASTNodeType::VALUE)
                {
                    ASTValue* val = static_cast<ASTValue*>(expression->left);
                    value = val->value;
                }
                else if (expression->left->type == ASTNodeType::VARIABLE)
                {
                    ASTVariable* val = static_cast<ASTVariable*>(expression->left);
                    value = val->GetRegistryIndex();
                }

                uint16_t conditionRegisterNum = registerNum++;
                conditionInstruction->val1 = value;
                conditionInstruction->val2 = conditionRegisterNum;
                fnDecl->AddInstruction(conditionInstruction);

                ByteInstruction* cmpInstruction = moduleInfo.GetByteInstruction();
                size_t cmpInstructionIndex = instructions.size();

                cmpInstruction->opcode = ByteOpcode::COMPARE_TO_REG;
                cmpInstruction->val1 = 0;
                cmpInstruction->val2 = conditionRegisterNum;
                fnDecl->AddInstruction(cmpInstruction);

                ByteInstruction* jmpToEndConditionalInstruction = moduleInfo.GetByteInstruction();
                jmpToEndConditionalInstruction->opcode = ByteOpcode::JMP_CONDITIONAL;
                jmpToEndConditionalInstruction->val1 = 0; // This is set later
                fnDecl->AddInstruction(jmpToEndConditionalInstruction);

                std::vector<uint64_t*> breakInstructionIndexPtrs;
                breakInstructionIndexPtrs.reserve(8);

                // Parse While Body
                ASTSequence* whileNextSequence = whileStmt->body;
                while (whileNextSequence->left)
                {
                    ASTNode* whileLeft = whileNextSequence->left;

                    if (whileLeft->type == ASTNodeType::VARIABLE)
                    {
                        ASTVariable* variable = static_cast<ASTVariable*>(whileLeft);

                        if (ASTExpression* expression = variable->expression)
                        {
                            ByteInstruction* byteInstruction = moduleInfo.GetByteInstruction();

                            // I have aligned the ASTOperaterType & ByteOpcode as such that they map 1:1 for ASTNodeType::VALUE, and ASTNodeType::Variable we add + 5
                            byteInstruction->opcode = static_cast<ByteOpcode>(static_cast<uint8_t>(expression->op) + 5 * (expression->left->type == ASTNodeType::VARIABLE));

                            uint64_t value = 0;
                            if (expression->left->type == ASTNodeType::VALUE)
                            {
                                ASTValue* val = static_cast<ASTValue*>(expression->left);
                                value = val->value;
                            }
                            else if (expression->left->type == ASTNodeType::VARIABLE)
                            {
                                ASTVariable* val = static_cast<ASTVariable*>(expression->left);
                                value = val->GetRegistryIndex();
                            }

                            byteInstruction->val1 = value;
                            byteInstruction->val2 = variable->GetRegistryIndex();

                            fnDecl->AddInstruction(byteInstruction);
                        }
                    }
                    else if (whileLeft->type == ASTNodeType::RETURN_STATEMENT)
                    {
                        ASTReturnStatement* returnStmt = static_cast<ASTReturnStatement*>(whileLeft);

                        if (returnStmt->value)
                        {
                            // Handle Expression (For now we assume we can only return a single literal/variable
                            ASTExpression* expression = returnStmt->value;

                            ByteInstruction* byteInstruction = moduleInfo.GetByteInstruction();
                            byteInstruction->opcode = static_cast<ByteOpcode>(static_cast<uint8_t>(ByteOpcode::MOVE_TO_REG) + 5 * (expression->left->type == ASTNodeType::VARIABLE));

                            uint64_t value = 0;
                            if (expression->left->type == ASTNodeType::VALUE)
                            {
                                ASTValue* val = static_cast<ASTValue*>(expression->left);
                                value = val->value;
                            }
                            else if (expression->left->type == ASTNodeType::VARIABLE)
                            {
                                ASTVariable* val = static_cast<ASTVariable*>(expression->left);
                                value = val->GetRegistryIndex();
                            }

                            byteInstruction->val1 = value;
                            byteInstruction->val2 = 0; // Return Values goes into the 0th registry (relative to the function)
                            fnDecl->AddInstruction(byteInstruction);
                        }

                        ByteInstruction* returnInstruction = moduleInfo.GetByteInstruction();
                        returnInstruction->opcode = ByteOpcode::RETURN;
                        fnDecl->AddInstruction(returnInstruction);
                    }
                    else if (whileLeft->type == ASTNodeType::JMP_STATEMENT)
                    {
                        ASTJmpStatement* jmp = static_cast<ASTJmpStatement*>(whileLeft);
                        if (jmp->jmpType == JMPStatementType::CONTINUE)
                        {
                            ByteInstruction* continueInstruction = moduleInfo.GetByteInstruction();
                            continueInstruction->opcode = ByteOpcode::JMP;
                            continueInstruction->val1 = cmpInstructionIndex;
                            fnDecl->AddInstruction(continueInstruction);
                        }
                        else if (jmp->jmpType == JMPStatementType::BREAK)
                        {
                            ByteInstruction* breakInstruction = moduleInfo.GetByteInstruction();
                            breakInstruction->opcode = ByteOpcode::JMP;
                            breakInstruction->val1 = 0; // This is set later
                            fnDecl->AddInstruction(breakInstruction);

                            breakInstructionIndexPtrs.push_back(&breakInstruction->val1);
                        }
                    }

                    whileNextSequence = whileNextSequence->right;
                }

                ByteInstruction* jmpToStartInstruction = moduleInfo.GetByteInstruction();
                jmpToStartInstruction->opcode = ByteOpcode::JMP;
                jmpToStartInstruction->val1 = cmpInstructionIndex;
                fnDecl->AddInstruction(jmpToStartInstruction);

                // Set the Instruction Index for where to jump if condition is false
                size_t loopEndIndex = instructions.size();
                jmpToEndConditionalInstruction->val1 = loopEndIndex;

                // Set the Break Instruction Index for where to jump
                for (uint64_t* index : breakInstructionIndexPtrs)
                {
                    *index = loopEndIndex;
                }
            }
            /*else if (left->type == ASTNodeType::IF_STATEMENT)
            {
                ASTIfStatement* ifStmt = static_cast<ASTIfStatement*>(left);
            }*/
            else if (left->type == ASTNodeType::RETURN_STATEMENT)
            {
                ASTReturnStatement* returnStmt = static_cast<ASTReturnStatement*>(left);

                if (returnStmt->value)
                {
                    // Handle Expression (For now we assume we can only return a single literal/variable
                    ASTExpression* expression = returnStmt->value;

                    ByteInstruction* byteInstruction = moduleInfo.GetByteInstruction();
                    byteInstruction->opcode = static_cast<ByteOpcode>(static_cast<uint8_t>(ByteOpcode::MOVE_TO_REG) + 5 * (expression->left->type == ASTNodeType::VARIABLE));

                    uint64_t value = 0;
                    if (expression->left->type == ASTNodeType::VALUE)
                    {
                        ASTValue* val = static_cast<ASTValue*>(expression->left);
                        value = val->value;
                    }
                    else if (expression->left->type == ASTNodeType::VARIABLE)
                    {
                        ASTVariable* val = static_cast<ASTVariable*>(expression->left);
                        value = val->GetRegistryIndex();
                    }

                    byteInstruction->val1 = value;
                    byteInstruction->val2 = 0; // Return Values goes into the 0th registry (relative to the function)
                    fnDecl->AddInstruction(byteInstruction);
                }

                ByteInstruction* returnInstruction = moduleInfo.GetByteInstruction();
                returnInstruction->opcode = ByteOpcode::RETURN;
                fnDecl->AddInstruction(returnInstruction);
            }

            nextSequence = nextSequence->right;
        }

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
