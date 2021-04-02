#pragma once
#include <pch/Build.h>
#include <vector>

#include "Token.h"
#include "CompilerInfo.h"
#include "../Memory/BlockAllocator.h"

#include <filesystem>
namespace fs = std::filesystem;

#pragma pack(push, 1)
struct AstNode
{
    enum class Type
    {
        NONE,
        DATATYPE,
        LITERAL,
        VARIABLE,
        VALUE,
        EXPRESSION,
        FUNCTION_CALL,
        RETURN
    };

    AstNode(Type nodeType) : type(nodeType) { }
    AstNode(const Token* inToken) : token(inToken) { }
    AstNode(Type nodeType, const Token* inToken) : type(nodeType), token(inToken) { }

    Type type = Type::NONE;
    const Token* token = nullptr;
};

struct AstDataType : public AstNode
{
    AstDataType() : AstNode(AstNode::Type::DATATYPE) { }
    AstDataType(Token* inToken) : AstNode(AstNode::Type::DATATYPE, inToken) { }

    TypeInfo type;
    bool isPointer = false;
};

struct AstLiteral
{
    AstLiteral() { }

    NaiType type = NaiType::NONE;
    int size = 0;

    union
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

        char* string;
    };
};

struct AstValue : public AstNode
{
    AstValue() : AstNode(AstNode::Type::VALUE) { }
    AstValue(const Token* inToken) : AstNode(AstNode::Type::VALUE, inToken) { }

    enum class Type
    {
        NONE,
        LITERAL,
        VARIABLE
    };

    Type type = Type::NONE;

    AstLiteral literal;
    int variableIndex = -1;
};

struct AstExpression : public AstNode
{
    AstExpression() : AstNode(AstNode::Type::EXPRESSION) { }
    AstExpression(Token* inToken) : AstNode(AstNode::Type::EXPRESSION, inToken) { }

    enum class Type
    {
        NONE,
        EXPRESSION,
        VALUE
    };

    enum class OperatorType
    {
        NONE,
        ADDITION,
        SUBTRACTION,
        MULTIPLICATION,
        DIVISION,
        MODULUS,
        POW,
        AND,
        OR,
        BITSHIFT_LEFT,
        BITSHIFT_RIGHT,
        BITSHIFT_AND,
        BITSHIFT_OR
    };
    static OperatorType GetOperatorTypeFromTokenType(const Token::Type& type)
    {
        switch (type)
        {
            case Token::Type::PLUS:
            case Token::Type::PLUS_EQUALS:
                return OperatorType::ADDITION;
            case Token::Type::MINUS:
            case Token::Type::MINUS_EQUALS:
                return OperatorType::SUBTRACTION;
            case Token::Type::ASTERISK:
            case Token::Type::MULTIPLY_EQUALS:
                return OperatorType::MULTIPLICATION;
            case Token::Type::SLASH:
            case Token::Type::DIVIDE_EQUALS:
                return OperatorType::DIVISION;
            case Token::Type::PERCENT:
            case Token::Type::MODULUS_EQUALS:
                return OperatorType::MODULUS;
            case Token::Type::CARET:
            case Token::Type::POW_EQUALS:
                return OperatorType::POW;
            case Token::Type::BITSHIFT_LEFT:
            case Token::Type::BITSHIFT_LEFT_EQUALS:
                return OperatorType::BITSHIFT_LEFT;
            case Token::Type::BITSHIFT_RIGHT_EQUALS:
            case Token::Type::BITSHIFT_RIGHT:
                return OperatorType::BITSHIFT_RIGHT;
            case Token::Type::AMPERSAND:
            case Token::Type::BITWISE_AND_EQUALS:
                return OperatorType::BITSHIFT_AND;
            case Token::Type::BITWISE_OR_EQUALS:
            case Token::Type::PIPE:
                return OperatorType::BITSHIFT_OR;
            case Token::Type::AND:
                return OperatorType::AND;
            case Token::Type::OR:
                return OperatorType::OR;

            default:
                break;
        }

        return OperatorType::NONE;
    }

    OperatorType op = OperatorType::NONE;

    Type lType = Type::NONE;
    union
    {
        AstExpression* node = nullptr;
        AstValue* val;
    } left;

    Type rType = Type::NONE;
    union
    {
        AstExpression* node = nullptr;
        AstValue* val;
    } right;
};

struct AstVariable : public AstNode
{
    AstVariable() : AstNode(AstNode::Type::VARIABLE) { }
    AstVariable(Token* inToken) : AstNode(AstNode::Type::VARIABLE, inToken) { }

    uint32_t index = 0;
    uint32_t parentIndex = 0;
    1
    AstDataType dataType;
    bool isConst = false;

    AstExpression* expression = nullptr;
};

struct AstReturnSet
{
    AstDataType dataType;
    AstExpression* expression = nullptr;
};

struct AstReturn : public AstNode
{
    AstReturn() : AstNode(AstNode::Type::RETURN) { }
    AstReturn(Token* inToken) : AstNode(AstNode::Type::RETURN, inToken) { }

    std::vector<AstReturnSet*> returnSets;
};

struct AstFunctionCall : public AstNode
{
public:
    AstFunctionCall() : AstNode(AstNode::Type::FUNCTION_CALL) { }
    AstFunctionCall(Token* inToken) : AstNode(AstNode::Type::FUNCTION_CALL, inToken) { }

    uint32_t moduleHash = 0;
    uint32_t functionHash = 0;

    std::vector<AstExpression> parameters;
};

struct AstSequence
{
    AstSequence() { }

    AstSequence* nextSequence = nullptr;
    AstNode* node = nullptr;
};

struct AstFunction
{
public:
    AstFunction(Token* inToken) : token(inToken)
    {
        returnTypeNodes.reserve(8);
        variables.reserve(8);
    }

    const Token* token = nullptr;
    uint32_t numParameters = 0;

    std::vector<AstDataType> returnTypeNodes;
    std::vector<AstVariable> variables;

    AstSequence* sequence = nullptr;

public:
    int GetVariableIndexByToken(const Token* token)
    {
        for (int i = 0; i < static_cast<int>(variables.size()); i++)
        {
            if (variables[i].token == token)
                return i;
        }

        return -1;
    }

private:
    AstFunction() { }
};
struct AstStruct
{
    //std::vector<AstDataType*> parameters;
    //std::vector<AstDataType*> returnTypes;

    //AstSequence* action;
};
struct AstEnum
{
    //std::vector<AstDataType*> parameters;
    //std::vector<AstDataType*> returnTypes;

    //AstSequence* action;
};
#pragma pack(pop)

struct CompileUnitAttributes
{
    bool parseResult = true;
};

struct CompileUnit
{
public:
    enum class Type : uint8_t
    {
        NONE,
        FUNCTION,
        STRUCT,
        ENUM
    };

    Type type = Type::NONE;

    std::string_view name = "";
    uint32_t moduleNameHash = 0;

    const char* startPtr = nullptr;
    const char* endPtr = nullptr;
    int startLineNum = 0;
    int startColumnNum = 0;

    const char* startAttributePtr = nullptr;
    const char* endAttributePtr = nullptr;
    int startAttributeLineNum = 0;
    int startAttributeColumnNum = 0;

    std::vector<Token> tokens;

    CompileUnitAttributes attributes;
    std::vector<Token> attributeTokens;

    AstFunction* astFunction = nullptr;
    AstStruct* astStruct = nullptr;
    AstEnum* astEnum = nullptr;

    Memory::BlockAllocator blockAllocator = Memory::BlockAllocator(64 * 1024);

public:
    Token& Peek(int index) { assert(index < static_cast<int>(tokens.size())); return tokens[index]; }
    Token& PeekAttribute(int index) { assert(index < static_cast<int>(attributeTokens.size())); return attributeTokens[index]; }
    void Eat(int& index) { index += 1; }

    template <typename T, typename... Args>
    T* CreateNode(Args... args)
    {
        return blockAllocator.New<T>(args...);
    }

    template <typename... Args>
    AstFunction* CreateFunction(Args... args)
    {
        return blockAllocator.New<AstFunction>(args...);
    }
    template <typename... Args>
    AstStruct* CreateStruct(Args... args)
    {
        return blockAllocator.New<AstStruct>(args...);
    }
    template <typename... Args>
    AstEnum* CreateEnum(Args... args)
    {
        return blockAllocator.New<AstEnum>(args...);
    }
};

struct ModuleDefinition
{
public:
    enum class Type : uint8_t
    {
        NONE,
        IMPORT
    };

    Type type = Type::NONE;

    const char* startPtr = nullptr;
    const char* endPtr = nullptr;

    int startLineNum = 0;
    int startColumnNum = 0;

    std::vector<Token> tokens;

public:
    Token& Peek(int index) { assert(index < static_cast<int>(tokens.size())); return tokens[index]; }
};
struct ModuleImport
{
    std::string_view name = "";
    uint32_t hash = 0;

    fs::path path = "";
    uint32_t pathHash = 0;

    bool isDirectory = false;
    bool isFound = false;
};

struct ModuleInfo
{
    ModuleInfo() { }

    void Init(const fs::path& modulePath, char* inFileBuffer, size_t inFileBufferSize)
    {
        name = modulePath.filename();
        nameHash = StringUtils::hash_djb2(name.string().c_str(), name.string().length());

        path = modulePath;
        pathHash = StringUtils::hash_djb2(path.string().c_str(), path.string().length());

        parentPath = path.parent_path();

        fileBuffer = inFileBuffer;
        fileBufferSize = static_cast<long>(inFileBufferSize);

        compileUnits.reserve(32);
        compileUnits.reserve(32);
        moduleImports.reserve(32);
    }

    fs::path name = "";
    uint32_t nameHash = 0;

    fs::path path = "";
    uint32_t pathHash = 0;

    fs::path parentPath = "";

    char* fileBuffer = nullptr;
    long fileBufferSize = 0;

    std::vector<ModuleDefinition> definitions;
    std::vector<CompileUnit> compileUnits;
    std::vector<ModuleImport> moduleImports;
};