#pragma once
#include <pch/Build.h>
#include "Lexer/Token.h"
#include "../ByteCode/ByteOpcode.h"
#include "../Utils/StringUtils.h"

enum class ASTNodeType : unsigned char
{
    NONE,
    SEQUENCE,
    OPERATOR,
    EXPRESSION,
    DATATYPE,
    VARIABLE,
    STRUCT,
    ENUM,
    ENUM_VALUE,
    FUNCTION_DECL,
    FUNCTION_CALL,
    FUNCTION_PARAM,
    FUNCTION_RETURN,
};

struct ASTNode
{
public:
    ASTNode(ASTNodeType inType) : token(nullptr), type(inType) { }
    ASTNode(const Token* inToken, ASTNodeType inType) : token(inToken), type(inType) { }

    std::string_view GetName() const
    {
        return std::string_view(token->value, token->valueSize);
    }
    inline int GetNameSize() const
    {
        return token->valueSize;
    }
    inline uint32_t GetNameHash()
    {
        if (_nameHash == 0)
            _nameHash = StringUtils::fnv1a_32(token->value, token->valueSize);

        return _nameHash;
    }

    const Token* token;
    ASTNodeType type;

private:
    uint32_t _nameHash = 0;
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
public:
    ASTExpression(const Token* inToken) : ASTNode(inToken, ASTNodeType::EXPRESSION) { }

    const std::string GetTypeName() const
    {
        if (token->type == TokenType::IDENTIFIER)
        {
            if (token->subType == TokenSubType::FUNCTION_CALL)
                return "Function Call";
            else
                return "Variable";
        }
        else if (token->type == TokenType::LITERAL)
            return "Literal";

        return "Invalid";
    }
};
struct ASTOperator : public ASTNode
{
public:
    ASTOperator(const Token* inToken) : ASTNode(inToken, ASTNodeType::OPERATOR) { }

    ASTNode* left = nullptr;
    ASTNode* right = nullptr;
};
struct ASTDataType : public ASTNode
{
public:
    ASTDataType(const Token* inToken) : ASTNode(inToken, ASTNodeType::DATATYPE) { }
};
struct ASTVariable : public ASTNode
{
public:
    ASTVariable(const Token* inToken) : ASTNode(inToken, ASTNodeType::VARIABLE) { }

    ASTDataType* GetDataType()
    {
        if (!dataType)
            return parent->dataType;

        return dataType;
    }

    ASTDataType* dataType = nullptr;
    ASTNode* value = nullptr;

    ASTVariable* parent = nullptr;
    bool isConst = false;

};

struct ASTFunctionParam : public ASTNode
{
public:
    ASTFunctionParam(const Token* inToken) : ASTNode(inToken, ASTNodeType::FUNCTION_PARAM) { }

    ASTDataType* dataType = nullptr;
    ASTNode* defaultValue = nullptr;
};

struct ASTFunctionDecl : public ASTNode
{
public:
    ASTFunctionDecl(const Token* inToken) : ASTNode(inToken, ASTNodeType::FUNCTION_DECL)
    {
        parameters.reserve(4);
        variables.reserve(4);
        opcodes.reserve(64);
    }

    std::vector<ASTFunctionParam*> parameters;
    std::vector<ASTVariable*> variables;
    ASTDataType* returnType = nullptr;

    ASTSequence* top = new ASTSequence();

    std::vector<ByteOpcode> opcodes;
};
struct ASTFunctionCall : public ASTNode
{
public:
    ASTFunctionCall(const Token* inToken) : ASTNode(inToken, ASTNodeType::FUNCTION_CALL)
    {
        parameters.reserve(4);
    }

    std::vector<ASTNode*> parameters;
}; 
struct ASTFunctionReturn : public ASTNode
{
public:
    ASTFunctionReturn(const Token* inToken) : ASTNode(inToken, ASTNodeType::FUNCTION_RETURN) { }

    ASTNode* top = nullptr;
};

struct ASTStruct : public ASTNode
{
public:
    ASTStruct(const Token* inToken) : ASTNode(inToken, ASTNodeType::STRUCT) 
    {
        variables.reserve(4);
        functions.reserve(4);
    }

    std::vector<ASTVariable*> variables;
    std::vector<ASTFunctionDecl*> functions;
};

struct ASTEnumValue : public ASTNode
{
public:
    ASTEnumValue(const Token* inToken) : ASTNode(inToken, ASTNodeType::ENUM_VALUE) { }

    ASTDataType* dataType = nullptr;
    ASTNode* defaultValue = nullptr;
};
struct ASTEnum : public ASTNode
{
public:
    ASTEnum(const Token* inToken) : ASTNode(inToken, ASTNodeType::ENUM) { }

    std::vector<ASTEnumValue> values;
};