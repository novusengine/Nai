#pragma once
#include "pch/Build.h"

#include "Token.h"
#include "Utils/LinkedList.h"

struct Scope;
struct Type;
struct Declaration;
struct Expression;

struct Primary
{
public:
    enum class Kind
    {
        None,
        Number,
        Identifier,
        String,
        Count
    };

    Kind kind = Kind::None;
    Token* token = nullptr;

    union
    {
        u64 number;
        Declaration* declaration;
        String* string = nullptr;
    };
};

struct Unary
{
public:
    enum class Kind
    {
        None,
        Deref,
        AddressOf,
        Count
    };

    Kind kind = Kind::None;

    Token* op = nullptr;
    Expression* operand = nullptr;

public:
    const char* GetKindName();
};

struct Binary
{
public:
    enum class Kind
    {
        None,
        Add,
        Subtract,
        Multiply,
        Divide,
        Modulo,
        Equal,
        NotEqual,
        LessThan,
        LessEqual,
        GreaterThan,
        GreaterEqual,
        Assign,
        Count
    };

    Kind kind = Kind::None;

    Token* op = nullptr;
    Expression* left = nullptr;
    Expression* right = nullptr;

public:
    const char* GetKindName();
};

struct Call
{
public:
    Call()
    {
        arguments.Init();
    }

    Token* token;
    Expression* expression;

    List arguments;
};

struct Dot
{
public:
    Token* token = nullptr;
    Token* tokenMember = nullptr;

    u32 offset;
    Expression* expression;
};

struct MemoryExpression
{
public:
    Token* token = nullptr;
    Expression* expression;
};

struct Expression
{
public:
    enum class Kind
    {
        None,
        Primary,
        Unary,
        Binary,
        Call,
        MemoryNew,
        MemoryFree,
        Dot,
        Count
    };

    union
    {
        Primary primary;
        Unary unary;
        Binary binary;
        Call call;
        MemoryExpression memory;
        Dot dot;
    };

    Expression() { }

    Kind kind = Kind::None;
    Type* type = nullptr;

    ListNode listNode;

public:
    static constexpr i8 InitialPriority = -1;
    static Expression* Create(Kind kind);
    static Expression* CreateCall();
    static Expression* CreatePrimary(Primary::Kind kind);
    static Expression* CreateUnary(Unary::Kind kind);
    static Expression* CreateBinary(Binary::Kind kind);
};

struct StructScope
{
public:
    StructScope()
    {
        members.Init();
    }

    StructScope* parent = nullptr;
    List members;
};

struct Struct
{
public:
    Struct()
    {
        members.Init();
    }

    List members;

    bool isStruct;
    StructScope* scope = nullptr;
};

struct StructMember
{
public:
    Type* type = nullptr;
    Token* token = nullptr;

    bool isAnonymous;
    u32 offset;
    ListNode scopeNode;
    ListNode listNode;
};

struct Enum
{
public:
    Scope* scope = nullptr;
    ListNode listNode;
};

struct TypeBasic
{
public:
    bool isSigned;
};

struct TypePointer
{
public:
    Type* type = nullptr;
    u64 count; // Array Size
};

struct UnknownType
{
public:
    Token* token;
};

struct FunctionType
{
    List argumentTypes;
};

struct Type
{
public:
    enum class Kind
    {
        None,
        Struct,
        Basic,
        Pointer,
        Unknown,
        Void,
        Function,
        Count
    };

    union
    {
        Struct structType;
        TypeBasic basic;
        TypePointer pointer;
        UnknownType unknown;
        FunctionType function;
    };

    Type() { }

    Kind kind = Kind::None;

    u32 size;
    u32 alignment;
    bool isResolved = false;

    ListNode listNode;

public:
    bool IsSigned();
    bool IsBasic();
    bool IsPointer();

public:
    static Type* Create(Kind kind);
    static Type* CreateBasic(u32 size, u32 alignment, bool isResolved, bool isSigned);
    static Type* CreatePointer();
    static Type* CreateStruct();
    static Type* CreateFunction();
};

struct Compound
{
public:
    Compound()
    {
        statements.Init();
    }

    List statements;
    Scope* scope = nullptr;
};

struct Statement;
struct Comment
{
public:
    Token* token = nullptr;
};

struct Return
{
    Expression* expression = nullptr;
};

struct Conditional
{
    Expression* condition = nullptr;

    Statement* trueBody = nullptr;
    Statement* falseBody = nullptr;
};

struct LoopPath
{
    enum class Kind
    {
        Continue,
        Break
    };

    Kind kind;
    u64 opcodeDataIndex;
};

struct Loop
{
    Loop* parent = nullptr;
    Expression* condition = nullptr;
    Statement* body = nullptr;
    std::vector<LoopPath>* paths = nullptr;
};

struct Statement
{
public:
    enum class Kind
    {
        None,
        Compound,
        Comment,
        Expression,
        Return,
        Conditional,
        Loop,
        Continue,
        Break,
        Count
    };

    union
    {
        Compound compound;
        Comment comment;
        Expression* expression;
        Return returnStmt;
        Conditional conditional;
        Loop loop;
        u32 number;
    };

    Statement() { }

    Kind kind = Kind::None;
    ListNode listNode;

public:
    static Statement* Create(Kind kind);
    static Compound* CreateCompound();
};

struct Function
{
public:
    Statement* body = nullptr;
    Type* returnType = nullptr;

    Scope* scope = nullptr;

    struct
    {
        u8 nativeCall : 1;
    } flags;

    ListNode listNode;
};

struct Variable
{
public:
    i64 offset;
};

struct Declaration
{
public:
    enum class Kind
    {
        None,
        Type,
        Function,
        Variable,
        Count
    };

    union
    {
        Function function;
        Variable variable;
    };

    Declaration() { }

    Kind kind = Kind::None;

    Type* type = nullptr;
    Token* token = nullptr;

    ListNode listNode;
};

struct Scope
{
public:
    Scope()
    {
        declarations.Init();
        scopeChildren.Init();
    }

    Scope* parent = nullptr;

    List declarations;

    ListNode listNode;
    List scopeChildren;
};

class TreePrinter
{
public:
    static void PrintStatement(Statement* statement);
    static void PrintComment(Comment* comment);
    static void PrintExpression(Expression* expression);
    static void PrintReturn(Return* ret);
    static void PrintConditional(Conditional* conditional);
    static void PrintLoop(Loop* loop);

private:
    template <typename... Args>
    static void PrintIdentation(const char* str, Args... args)
    {
        if (identationLevel)
        {
            for (u32 i = 0; i < (identationLevel - 1); i++)
            {
                if (identationMask[i])
                {
                    DebugHandler::Print_NoNewLine("|   ");
                }
                else
                {
                    DebugHandler::Print_NoNewLine("    ");
                }
            }

            DebugHandler::Print_NoNewLine("|-->");
        }

        DebugHandler::Print(str, args...);
    }

private:
    static constexpr u32 MaxIndentationLevel = 128;

    static u32 identationLevel;
    static bool identationMask[];
};