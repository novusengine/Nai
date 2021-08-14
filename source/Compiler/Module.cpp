#include "pch/Build.h"
#include "Module.h"
#include <Compiler/Frontend/Typer.h>
#include <Compiler/Frontend/Parser.h>

LexerInfo& LexerInfo::operator=(const LexerInfo& a)
{
    buffer = a.buffer;
    size = a.size;
    index = a.index;
    tokens = std::move(a.tokens);
    return *this;
}

NativeFunction::NativeFunction(Module* module, const String& inName, std::function<NativeFunctionCallbackFunc> callback)
{
    name = inName;

    _module = module;
    _declaration = new Declaration();
    _declaration->kind = Declaration::Kind::Function;
    _declaration->type = Type::CreateFunction();

    // Create Token
    {
        Token* token = new Token();
        token->kind = Token::Kind::Identifier;
        token->nameHash.SetNameHash(name.c_str(), name.length());
        token->number = 0;

        _declaration->token = token;
    }

    Function* function = &_declaration->function;
    function->flags.nativeCall = true;

    // Create Scope
    {
        Scope* globalScope = _module->parserInfo.block->scope;
        function->scope = new Scope();

        if (globalScope)
        {
            List::AddNodeBack(&globalScope->scopeChildren, &function->scope->listNode);
        }
        function->scope->parent = globalScope;
    }

    // Set Return Type
    {
        function->returnType = Type::Create(Type::Kind::Void);
    }

    // Create Dummy Body
    {
        Compound* compound = Statement::CreateCompound();

        // Setup Scope
        {
            compound->scope = new Scope();

            List::AddNodeBack(&function->scope->scopeChildren, &compound->scope->listNode);

            compound->scope->parent = function->scope;
        }

        function->body = reinterpret_cast<Statement*>(compound);
    }

    // Try to add the function to this module
    {
        _module->parserInfo.currentScope = _module->parserInfo.block->scope;
        {
            Parser::DeclarationPushToCurrentScope(_module, _declaration);
        }
        _module->parserInfo.currentScope = nullptr;
    }

    _module->_nativeFunctionHashToCallback[_declaration->token->nameHash.hash] = callback;
}

void NativeFunction::AddParamChar(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_Char, passAs);
}

void NativeFunction::AddParamBool(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_Bool, passAs);
}

void NativeFunction::AddParamI8(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_I8, passAs);
}

void NativeFunction::AddParamI16(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_I16, passAs);
}

void NativeFunction::AddParamI32(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_I32, passAs);
}

void NativeFunction::AddParamI64(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_I64, passAs);
}

void NativeFunction::AddParamU8(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_U8, passAs);
}

void NativeFunction::AddParamU16(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_U16, passAs);
}

void NativeFunction::AddParamU32(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_U32, passAs);
}

void NativeFunction::AddParamU64(const String& inName, PassAs passAs)
{
    TryAddParam(inName, Type_U64, passAs);
}

void NativeFunction::SetReturnTypeChar(PassAs passAs)
{
    SetReturnType(Type_Char, passAs);
}

void NativeFunction::SetReturnTypeBool(PassAs passAs)
{
    SetReturnType(Type_Bool, passAs);
}

void NativeFunction::SetReturnTypeI8(PassAs passAs)
{
    SetReturnType(Type_I8, passAs);
}

void NativeFunction::SetReturnTypeI16(PassAs passAs)
{
    SetReturnType(Type_I16, passAs);
}

void NativeFunction::SetReturnTypeI32(PassAs passAs)
{
    SetReturnType(Type_I32, passAs);
}

void NativeFunction::SetReturnTypeI64(PassAs passAs)
{
    SetReturnType(Type_I64, passAs);
}

void NativeFunction::SetReturnTypeU8(PassAs passAs)
{
    SetReturnType(Type_U8, passAs);
}

void NativeFunction::SetReturnTypeU16(PassAs passAs)
{
    SetReturnType(Type_U16, passAs);
}

void NativeFunction::SetReturnTypeU32(PassAs passAs)
{
    SetReturnType(Type_U32, passAs);
}

void NativeFunction::SetReturnTypeU64(PassAs passAs)
{
    SetReturnType(Type_U64, passAs);
}

void NativeFunction::SetReturnTypeUnknown(const String& inName, PassAs passAs)
{
    Type* type = Type::Create(Type::Kind::Unknown);

    // Setup Token
    {
        returnName = inName;

        type->unknown.token = new Token();
        type->unknown.token->kind = Token::Kind::Identifier;
        type->unknown.token->nameHash.SetNameHash(returnName.c_str(), returnName.length());
        type->unknown.token->number = 0;
    }
    
    SetReturnType(type, passAs);
}

void NativeFunction::TryAddParam(const String& inName, Type* type, PassAs passAs)
{
    Declaration* declaration = new Declaration();
    declaration->kind = Declaration::Kind::Variable;

    // Setup Token
    {
        std::string& paramName = _paramNames.emplace_back(inName);

        Token* token = new Token();
        token->kind = Token::Kind::Identifier;
        token->nameHash.SetNameHash(paramName.c_str(), paramName.length());
        token->number = 0;

        declaration->token = token;
    }

    if (passAs == PassAs::Pointer)
    {
        Type* ptrType = Type::CreatePointer();
        ptrType->pointer.type = type;
        declaration->type = ptrType;
    }
    else
    {
        declaration->type = type;
    }

    // Try to add the parameter to the function
    {
        _module->parserInfo.currentScope = _declaration->function.scope;
        {
            Parser::DeclarationPushToCurrentScope(_module, declaration);
        }
        _module->parserInfo.currentScope = nullptr;
    }

    _numParameters++;
}

void NativeFunction::SetReturnType(Type* type, PassAs passAs)
{
    if (passAs == PassAs::Pointer)
    {
        Type* ptrType = Type::CreatePointer();
        ptrType->pointer.type = type;
        _declaration->function.returnType = ptrType;
    }
    else
    {
        _declaration->function.returnType = type;
    }
}
