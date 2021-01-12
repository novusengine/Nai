#pragma once
#include <pch/Build.h>
#include "../ModuleInfo.h"
#include "../Token.h"
#include "../../NaiType.h"

#include "../../Utils/StringUtils.h"
#include "../../Utils/DynamicBytebuffer.h"
#include "../../Memory/BlockContainer.h"
#include "../Bytecode/ByteOpcode.h"

enum class ASTNodeType : unsigned char
{
    NONE,
    SEQUENCE,
    EXPRESSION,
    VALUE, // TODO: We no longer need VALUE, fix this !!!
    VARIABLE,
    DATATYPE,
    WHILE_STATEMENT,
    IF_STATEMENT,
    JMP_STATEMENT,
    RETURN_STATEMENT,
    FUNCTION_DECL,
    FUNCTION_PARAMETER,
    FUNCTION_CALL,
    FUNCTION_ARGUMENT
};

enum class ASTOperatorType : unsigned char
{
    NONE,
    ASSIGN,
    ASSIGN_ADD,
    ASSIGN_SUBTRACT,
    ASSIGN_MULTIPLY,
    ASSIGN_DIVIDE,
    ASSIGN_BITWISE_AND,
    ASSIGN_BITWISE_OR,
    ASSIGN_BITSHIFT_LEFT,
    ASSIGN_BITSHIFT_RIGHT,
    ASSIGN_INCREMENT,
    ASSIGN_DECREMENT,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    MODULUS,
    EQUALS,
    LESS,
    GREATER,
    EQUALS_LESS,
    EUQLAS_GREATER,
    EQUALS_NOT,
    OR,
    AND,
    NOT,
    BITWISE_OR,
    BITWISE_AND,
    BITWISE_SHIFT_LEFT,
    BITWISE_SHIFT_RIGHT
};

enum class IFStatementType : unsigned char
{
    NONE,
    IF,
    ELSEIF,
    ELSE
};
enum class JMPStatementType : unsigned char
{
    NONE,
    CONTINUE,
    BREAK
};

#pragma pack(push, 1)
struct ASTNode
{
public:
    ASTNode(ASTNodeType inType)
    {
        type = inType;
    }
    ASTNode(Token& inToken, ASTNodeType inType)
    {
        token = &inToken;
        type = inType;
    }

    void UpdateToken(Token* inToken)
    {
        token = inToken;
    }

    const char* GetName()
    {
        if (!token)
            return nullptr;

        return token->stringview.data();
    }
    int GetNameSize()
    {
        if (!token)
            return 0;

        return token->stringview.length();
    }

    Token* token = nullptr;
    ASTNodeType type;

    size_t GetNameHashed()
    {
        //if (_hashedName == 0)
            //_hashedName = StringUtils::hash_djb2(token->value, token->valueSize);

        return _hashedName;
    }
private:
    size_t _hashedName = 0;
};

struct ASTSequence : public ASTNode
{
public:
    ASTSequence() : ASTNode(ASTNodeType::SEQUENCE) { }

    ASTNode* left = nullptr;
    ASTSequence* right = nullptr;
};
struct ASTExpression : public ASTNode
{
    ASTExpression() : ASTNode(ASTNodeType::EXPRESSION) { }

    ASTOperatorType op = ASTOperatorType::NONE;

    ASTNode* left = nullptr;
    ASTNode* right = nullptr;

    bool UpdateOperator(const Token* /*inToken*/)
    {
        /*switch (inToken->subType)
        {
            case TokenSubType::OP_ADD_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN_ADD;
                return true;
            }
            case TokenSubType::OP_SUBTRACT_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN_SUBTRACT;
                return true;
            }
            case TokenSubType::OP_MULTIPLY_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN_MULTIPLY;
                return true;
            }
            case TokenSubType::OP_DIVIDE_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN_DIVIDE;
                return true;
            }
            case TokenSubType::OP_INCREMENT:
            {
                op = ASTOperatorType::ASSIGN_INCREMENT;
                return true;
            }
            case TokenSubType::OP_DECREMENT:
            {
                op = ASTOperatorType::ASSIGN_DECREMENT;
                return true;
            }
            case TokenSubType::OP_EQUALS:
            {
                op = ASTOperatorType::EQUALS;
                return true;
            }
            case TokenSubType::OP_GREATER_EQUALS:
            {
                op = ASTOperatorType::EUQLAS_GREATER;
                return true;
            }
            case TokenSubType::OP_LESS_EQUALS:
            {
                op = ASTOperatorType::EQUALS_LESS;
                return true;
            }
            case TokenSubType::OP_NOT_EQUALS:
            {
                op = ASTOperatorType::EQUALS_NOT;
                return true;
            }
            case TokenSubType::OP_AND:
            {
                op = ASTOperatorType::AND;
                return true;
            }
            case TokenSubType::OP_OR:
            {
                op = ASTOperatorType::OR;
                return true;
            }
            case TokenSubType::OP_BITWISE_AND_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN_BITWISE_AND;
                return true;
            }
            case TokenSubType::OP_BITWISE_OR_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN_BITWISE_OR;
                return true;
            }
            case TokenSubType::OP_BITWISE_SHIFT_LEFT:
            {
                op = ASTOperatorType::BITWISE_SHIFT_LEFT;
                return true;
            }
            case TokenSubType::OP_BITWISE_SHIFT_RIGHT:
            {
                op = ASTOperatorType::BITWISE_SHIFT_RIGHT;
                return true;
            }
            case TokenSubType::OP_BITWISE_SHIFT_LEFT_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN_BITSHIFT_LEFT;
                return true;
            }
            case TokenSubType::OP_BITWISE_SHIFT_RIGHT_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN_BITSHIFT_RIGHT;
                return true;
            }

            default:
                break;
        }

        switch (inToken->type)
        {
            case TokenType::OP_ASSIGN:
            {
                op = ASTOperatorType::ASSIGN;
                return true;
            }
            case TokenType::OP_ADD:
            {
                op = ASTOperatorType::ADD;
                return true;
            }
            case TokenType::OP_SUBTRACT:
            {
                op = ASTOperatorType::SUBTRACT;
                return true;
            }
            case TokenType::OP_MULTIPLY:
            {
                op = ASTOperatorType::MULTIPLY;
                return true;
            }
            case TokenType::OP_DIVIDE:
            {
                op = ASTOperatorType::DIVIDE;
                return true;
            }
            case TokenType::OP_MODULUS:
            {
                op = ASTOperatorType::MODULUS;
                return true;
            }
            case TokenType::OP_BITWISE_AND:
            {
                op = ASTOperatorType::BITWISE_AND;
                return true;
            }
            case TokenType::OP_BITWISE_OR:
            {
                op = ASTOperatorType::BITWISE_OR;
                return true;
            }
            case TokenType::OP_LESS:
            {
                op = ASTOperatorType::LESS;
                return true;
            }
            case TokenType::OP_GREATER:
            {
                op = ASTOperatorType::GREATER;
                return true;
            }

            default:
                break;
        }*/

        return false;
    }

    bool IsAssignOperator()
    {
        return op >= ASTOperatorType::ASSIGN && op <= ASTOperatorType::ASSIGN_BITSHIFT_RIGHT;
    }
    bool IsArithmeticOperator()
    {
        return op >= ASTOperatorType::ADD && op <= ASTOperatorType::MODULUS;
    }
    bool IsComparisonOperator()
    {
        return op >= ASTOperatorType::EQUALS && op <= ASTOperatorType::NOT;
    }
    bool IsBitwiseOperator()
    {
        return op >= ASTOperatorType::BITWISE_OR && op <= ASTOperatorType::BITWISE_SHIFT_RIGHT;
    }
};

struct ASTDataType : public ASTNode
{
    ASTDataType() : ASTNode(ASTNodeType::DATATYPE) { }
    ASTDataType(Token& token) : ASTNode(token, ASTNodeType::DATATYPE) { }

    NaiType& GetType() { return _type; }
    void SetType(NaiType inType) { _type = inType; }

    // TODO: Actually use SetSize
    uint8_t GetSize() { return _size; }
    void SetSize(uint8_t size) { _size = size; }

private:
    NaiType _type = NaiType::INVALID;
    uint8_t _size = 0;
};
struct ASTValue : public ASTNode
{
    ASTValue(ASTNodeType type = ASTNodeType::VALUE) : ASTNode(type) { }
    ASTValue(Token& token, ASTNodeType type = ASTNodeType::VALUE) : ASTNode(token, type) { }

    ASTDataType* dataType = nullptr;

    union NaiValue
    {
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;

        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;

        float f32;
        double f64;
    } value;

    void UpdateValueFromNumeric()
    {
        value.u64 = 0; //StringUtils::ToUInt64(token->value, token->valueSize);
    }
    void UpdateValueFromHex()
    {
        char bytes[64] = { 0 };
        //memcpy(bytes, token->value, token->valueSize);

        value.u64 = strtoull(bytes, nullptr, 16);
    }
    void UpdateValueFromFloat()
    {
        char bytes[64] = { 0 };
        //memcpy(bytes, token->value, token->valueSize);

        value.f64 = strtod(bytes, nullptr);
    }

    void UpdateTypeFromValue()
    {
        /*if (token->subType == TokenSubType::NUMERIC || token->subType == TokenSubType::HEX)
        {
            if (value.i64 < std::numeric_limits<int32_t>().max())
                dataType->SetType(NaiType::I32);
            else
            {
                dataType->SetType(NaiType::I64);
            }
        }
        else if (token->subType == TokenSubType::FLOAT)
        {
            if (value.f64 < std::numeric_limits<float>().max())
                dataType->SetType(NaiType::F32);
            else
            {
                dataType->SetType(NaiType::F64);
            }
        }*/
    }
};
struct ASTVariable : public ASTValue
{
    ASTVariable() : ASTValue(ASTNodeType::VARIABLE) { }
    ASTVariable(Token& token) : ASTValue(token, ASTNodeType::VARIABLE) { }

    NaiValue& GetValue()
    {
        return parent ? parent->value : value;
    }
    uint16_t& GetStackOffset()
    {
        return parent ? parent->heapOffset : heapOffset;
    }

    ASTDataType* GetDataType()
    {
        return parent ? parent->dataType : dataType;
    }

    // Originally declared variable
    ASTVariable* parent = nullptr;

    // Calculated Value
    ASTExpression* expression = nullptr;

    uint16_t heapOffset = 0;
};

struct ASTWhileStatement : public ASTNode
{
    ASTWhileStatement() : ASTNode(ASTNodeType::WHILE_STATEMENT) { }

    ASTExpression* condition = nullptr;
    ASTSequence* body = nullptr;

    std::vector<uint16_t*> continuePtrs;
    std::vector<uint16_t*> breakPtrs;
};
struct ASTIfStatement : public ASTNode
{
    ASTIfStatement() : ASTNode(ASTNodeType::IF_STATEMENT) { }

    ASTExpression* condition = nullptr;
    ASTSequence* body = nullptr;
    ASTIfStatement* next = nullptr;

    IFStatementType ifType = IFStatementType::NONE;
    bool isInsideLoop = false; // Defines if the If Statement is inside a loop (for, while... etc)
};
struct ASTJmpStatement : public ASTNode
{
    ASTJmpStatement() : ASTNode(ASTNodeType::JMP_STATEMENT) { }

    JMPStatementType jmpType = JMPStatementType::NONE;
};
struct ASTReturnStatement : public ASTNode
{
    ASTReturnStatement() : ASTNode(ASTNodeType::RETURN_STATEMENT) { }

    ASTExpression* value = nullptr;
};

struct ASTFunctionDecl : public ASTNode
{
    ASTFunctionDecl() : ASTNode(ASTNodeType::FUNCTION_DECL) 
    {
        _byteInstructions.reserve(64);
        _parameters.reserve(16);
        _variables.reserve(16);
    }

    void AddInstruction(ByteInstruction* byteInstruction)
    {
        _byteInstructions.push_back(byteInstruction);
    }
    void AddParameter(ASTVariable* param)
    {
        _parameters.push_back(param);
    }
    void AddVariable(ASTVariable* var)
    {
        _variables.push_back(var);
    }

    std::vector<ByteInstruction*>& GetInstructions()
    {
        return _byteInstructions;
    }
    std::vector<ASTVariable*>& GetParameters()
    {
        return _parameters;
    }
    std::vector<ASTVariable*>& GetVariables()
    {
        return _variables;
    }

    uint32_t& GetHeapOffset()
    {
        return heapOffset;
    }

    uint32_t GetHeapOffsetAndAdd(uint32_t size)
    {
        uint32_t oldStackOffset = heapOffset;
        heapOffset += size;

        return oldStackOffset;
    }

    ASTDataType* returnType = nullptr;
    ASTSequence* body = nullptr;

    uint32_t heapOffset = 0;

private:
    std::vector<ByteInstruction*> _byteInstructions;
    std::vector<ASTVariable*> _parameters;
    std::vector<ASTVariable*> _variables;
};
struct ASTFunctionArgument : public ASTNode
{
    ASTFunctionArgument() : ASTNode(ASTNodeType::FUNCTION_ARGUMENT) { }

    ASTNode* node = nullptr;
    uint32_t stackOffset = 0;
};
struct ASTFunctionCall : public ASTNode
{
    ASTFunctionCall() : ASTNode(ASTNodeType::FUNCTION_CALL) { }

    void AddArgument(ASTFunctionArgument* arg)
    {
        _arguments.push_back(arg);
    }

    std::vector<ASTFunctionArgument*>& GetArguments()
    {
        return _arguments;
    }

private:
    std::vector<ASTFunctionArgument*> _arguments;
};
#pragma pack(pop)