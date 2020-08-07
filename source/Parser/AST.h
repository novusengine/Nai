#pragma once
#include <pch/Build.h>
#include "Lexer/Token.h"
#include "../NaiType.h"

#include "../Utils/StringUtils.h"
#include "../Memory/BlockContainer.h"
#include "../Bytecode/ByteOpcode.h"

enum class ASTNodeType : unsigned char
{
    NONE,
    SEQUENCE,
    EXPRESSION,
    VALUE,
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
    ASSIGN_MODULUS,
    ASSIGN_INCREMENT,
    ASSIGN_DECREMENT,
    LESS,
    GREATER,
    EQUALS,
    EQUALS_LESS,
    EUQLAS_GREATER,
    EQUALS_NOT,
    BITWISE_OR,
    BITWISE_AND,
    OR,
    AND,
    NOT
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

        return token->value;
    }
    int GetNameSize()
    {
        if (!token)
            return 0;

        return token->valueSize;
    }

    Token* token = nullptr;
    ASTNodeType type;

    size_t GetNameHashed()
    {
        if (_hashedName == 0)
            _hashedName = StringUtils::hash_djb2(token->value, token->valueSize);

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

    bool UpdateOperator(const Token* token)
    {
        switch (token->subType)
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

        default:
            break;
        }

        switch (token->type)
        {
        case TokenType::OP_ASSIGN:
        {
            op = ASTOperatorType::ASSIGN;
            return true;
        }
        case TokenType::OP_ADD:
        {
            op = ASTOperatorType::ASSIGN_ADD;
            return true;
        }
        case TokenType::OP_SUBTRACT:
        {
            op = ASTOperatorType::ASSIGN_SUBTRACT;
            return true;
        }
        case TokenType::OP_MULTIPLY:
        {
            op = ASTOperatorType::ASSIGN_MULTIPLY;
            return true;
        }
        case TokenType::OP_DIVIDE:
        {
            op = ASTOperatorType::ASSIGN_DIVIDE;
            return true;
        }
        case TokenType::OP_MODULUS:
        {
            op = ASTOperatorType::ASSIGN_MODULUS;
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
        }

        return false;
    }
};

struct ASTDataType : public ASTNode
{
    ASTDataType() : ASTNode(ASTNodeType::DATATYPE) { }
    ASTDataType(Token& token) : ASTNode(token, ASTNodeType::DATATYPE) { }

    NaiType& GetType() { return _type; }
    void SetType(NaiType inType) { _type = inType; }

private:
    NaiType _type = NaiType::INVALID;
};
struct ASTValue : public ASTNode
{
    ASTValue(ASTNodeType type = ASTNodeType::VALUE) : ASTNode(type) { }
    ASTValue(Token& token, ASTNodeType type = ASTNodeType::VALUE) : ASTNode(token, type) { }

    ASTDataType* dataType = nullptr;

    // Numeric Constant Value
    uint64_t value = 0;
    uint16_t registerIndex = 0;

    void UpdateValue()
    {
        value = StringUtils::ToUInt64(token->value, token->valueSize);
    }
};
struct ASTVariable : public ASTValue
{
    ASTVariable() : ASTValue(ASTNodeType::VARIABLE) { }
    ASTVariable(Token& token) : ASTValue(token, ASTNodeType::VARIABLE) { }

    uint64_t& GetValue()
    {
        return parent ? parent->value : value;
    }
    uint16_t& GetRegistryIndex()
    {
        return parent ? parent->registerIndex : registerIndex;
    }

    ASTDataType* GetDataType()
    {
        return parent ? parent->dataType : dataType;
    }

    // Originally declared variable
    ASTVariable* parent = nullptr;

    // Calculated Value
    ASTExpression* expression = nullptr;
};

struct ASTWhileStatement : public ASTNode
{
    ASTWhileStatement() : ASTNode(ASTNodeType::WHILE_STATEMENT) { }

    ASTExpression* condition = nullptr;
    ASTSequence* body = nullptr;
};
struct ASTIfStatement : public ASTNode
{
    ASTIfStatement() : ASTNode(ASTNodeType::IF_STATEMENT) { }

    ASTExpression* condition = nullptr;
    ASTSequence* body = nullptr;
    ASTIfStatement* next = nullptr;

    IFStatementType ifType = IFStatementType::NONE;
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

struct ASTFunctionParameter : public ASTNode
{
    ASTFunctionParameter() : ASTNode(ASTNodeType::FUNCTION_PARAMETER) { }

    uint16_t registerIndex = 0;

    ASTDataType* dataType = nullptr;
    ASTExpression* expression = nullptr;
};
struct ASTFunctionDecl : public ASTNode
{
    ASTFunctionDecl() : ASTNode(ASTNodeType::FUNCTION_DECL) 
    {
        _byteInstructions.reserve(16);
        _parameters.reserve(16);
        _variables.reserve(16);
    }

    void AddInstruction(ByteInstruction* byteInstruction)
    {
        _byteInstructions.push_back(byteInstruction);
    }
    void AddParameter(ASTFunctionParameter* param)
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
    std::vector<ASTFunctionParameter*>& GetParameters()
    {
        return _parameters;
    }
    std::vector<ASTVariable*>& GetVariables()
    {
        return _variables;
    }

    ASTDataType* returnType = nullptr;
    ASTSequence* body = nullptr;

private:
    std::vector<ByteInstruction*> _byteInstructions;
    std::vector<ASTFunctionParameter*> _parameters;
    std::vector<ASTVariable*> _variables;
};
struct ASTFunctionArgument : public ASTNode
{
    ASTFunctionArgument() : ASTNode(ASTNodeType::FUNCTION_ARGUMENT) { }

    ASTNode* value = nullptr;
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