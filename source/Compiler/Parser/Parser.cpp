#include <pch/Build.h>
#include <execution>
#include <stack>
#include <sstream>

#include "Parser.h"
#include "../Compiler.h"
#include "../CompilerInfo.h"
#include "../../Memory/BlockAllocator.h"
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <robin_hood.h>

enum class ParseRuleSet
{
    NONE,
    IMPORT,
    IMPORT_MODULE_NAME,

    ATTRIBUTE,
    ATTRIBUTE_ASSIGNMENT,
    ATTRIBUTE_OPTION,
    ATTRIBUTE_OPTION_VALUE,
    ATTRIBUTE_OPTION_NEXT,

    FUNCTION,
    FUNCTION_EXPRESSION,
    FUNCTION_CALL_EXPRESSION,
    FUNCTION_EXPRESSION_IDENTIFIER,
    FUNCTION_EXPRESSION_IDENTIFIER_ACCESS,
    FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS,
    FUNCTION_EXPRESSION_SEQUENCE,
    FUNCTION_PARAMETER,
    FUNCTION_PARAMETER_NEXT,
    FUNCTION_RETURN_TYPE,
    FUNCTION_RETURN_TYPE_ACTION,
    FUNCTION_RETURN_TYPE_ACTION_SEQUENCE,
    FUNCTION_BODY,
    FUNCTION_IDENTIFIER,
    FUNCTION_IDENTIFIER_ACTION,
    FUNCTION_IDENTIFIER_MULTI_DECLARE,
    FUNCTION_IDENTIFIER_MULTI_DECLARE_ASSIGN,
    FUNCTION_IDENTIFIER_MULTI_DECLARE_SEQUENCE,
    FUNCTION_IDENTIFIER_ACCESS,
    FUNCTION_IDENTIFIER_ARRAY_ACCESS,
    FUNCTION_IF,
    FUNCTION_IF_NEXT,
    FUNCTION_WHILE,
    FUNCTION_FOR,
    FUNCTION_FOR_HEADER_IDENTIFIER,
    FUNCTION_FOR_HEADER_IDENTIFIER_ACTION,
    FUNCTION_FOR_HEADER_CONDITION,
    FUNCTION_FOR_HEADER_ACTION,
    FUNCTION_FOREACH,
    FUNCTION_FOREACH_ACTION,
    FUNCTION_FOREACH_IN,
    FUNCTION_FOREACH_IDENTIFIER_SEQUENCE,
    FUNCTION_FOREACH_IDENTIFIER_ACCESS,
    FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS,
    FUNCTION_CALL_PARAMETER_LIST,
    FUNCTION_CALL_PARAMETER,
    FUNCTION_CALL_PARAMETER_SEQUENCE,


    STRUCT,
    STRUCT_BODY,
    STRUCT_EXPRESSION,
    STRUCT_EXPRESSION_SEQUENCE,

    STRUCT_IDENTIFIER,
    STRUCT_IDENTIFIER_ACTION,

    ENUM,
    ENUM_BODY,
    ENUM_EXPRESSION,
    ENUM_EXPRESSION_SEQUENCE,
    ENUM_DECLARATION,
    ENUM_IDENTIFIER_ACTION,
    ENUM_IDENTIFIER_ACTION_SEQUENCE,

    IDENTIFIER,
    STRING,
    DATATYPE,
    DATATYPE_SEQUENCE,
    DECLARATION,
    DECLARATION_ASSIGNMENT,
    DECLARATION_ASSIGNMENT_ACTION,
    DECLARATION_ASSIGNMENT_FORCE,
    ASSIGNMENT,
    ACCESS,
    ASTERISK,
    PARAM_SEPERATOR,
    OPEN_PAREN,
    OPEN_BRACE,
    OPEN_BRACKET,
    CLOSE_PAREN,
    CLOSE_BRACE,
    CLOSE_BRACKET,
    END_OF_LINE
};

/*bool Parser::Process(ModuleInfo& moduleInfo)
{
    if (!CheckSyntax(moduleInfo))
        return false;

    if (!CreateAst(moduleInfo))
        return false;

    if (!CheckSemantics(moduleInfo))
        return false;

    return true;
}*/

bool Parser::CheckSyntax(ModuleInfo& moduleInfo)
{
    std::atomic<int> errorCount = 0;

    // Check Imports and Create ModuleInfo Import list
    for (ModuleDefinition& definition : moduleInfo.definitions)
    {
        int numTokens = static_cast<int>(definition.tokens.size());
        bool localDidError = numTokens == 0;

        if (numTokens > 0)
        {
            if (definition.type == ModuleDefinition::Type::IMPORT)
            {
                if (!CheckSyntaxImport(moduleInfo, definition, numTokens))
                    localDidError = true;
            }
            else
                localDidError = true;
        }

        if (localDidError)
        {
            errorCount += 1;
            break;
        }
    }

    std::for_each(std::execution::par, moduleInfo.compileUnits.begin(), moduleInfo.compileUnits.end(),
        [&errorCount](CompileUnit& compileUnit)
        {
            int numTokens = static_cast<int>(compileUnit.tokens.size());
            bool localDidError = numTokens == 0;

            //std::string_view currentAttributeName = "";

            if (numTokens > 0)
            {
                int numAttributeTokens = static_cast<int>(compileUnit.attributeTokens.size());
                if (numAttributeTokens > 0)
                {
                    if (!CheckSyntaxAttribute(compileUnit, numAttributeTokens))
                        localDidError = true;
                }

                if (localDidError == false)
                {
                    if (compileUnit.type == CompileUnit::Type::FUNCTION)
                    {
                        if (!CheckSyntaxFunction(compileUnit, numTokens))
                            localDidError = true;
                    }
                    else if (compileUnit.type == CompileUnit::Type::STRUCT)
                    {
                        if (!CheckSyntaxStruct(compileUnit, numTokens))
                            localDidError = true;
                    }
                    else if (compileUnit.type == CompileUnit::Type::ENUM)
                    {
                        if (!CheckSyntaxEnum(compileUnit, numTokens))
                            localDidError = true;
                    }
                }
            }

            if (localDidError)
            {
                errorCount += 1;
            }

            if (compileUnit.attributes.parseResult == !localDidError)
            {
                NC_LOG_MESSAGE("Compile Unit (%.*s) passed syntax check", compileUnit.name.length(), compileUnit.name.data());
            }
            else
            {
                NC_LOG_MESSAGE("Compile Unit (%.*s) failed syntax check result", compileUnit.name.length(), compileUnit.name.data());
            }
        });

    if (errorCount == 0)
    {
        NC_LOG_MESSAGE("Module (%.*s) passed syntax check", moduleInfo.name.string().length(), moduleInfo.name.string().c_str());
    }
    else
    {
        NC_LOG_MESSAGE("Module (%.*s) failed to pass syntax check", moduleInfo.name.string().length(), moduleInfo.name.string().c_str());
    }

    return errorCount == 0;
}

bool Parser::ResolveImports(Compiler* compiler, ModuleInfo& moduleInfo)
{
    int numModules = static_cast<int>(moduleInfo.moduleImports.size());
    int modulesFound = 0;

    for (int i = 0; i < numModules; i++)
    {
        ModuleImport& moduleImport = moduleInfo.moduleImports[i];

        bool isFound = false;
        bool isDirectory = false;

        fs::path baseIncludePath = (moduleInfo.parentPath / moduleImport.name);
        for (const char& c : moduleImport.name)
        {
            if (c == ':' || c == '.')
            {
                NC_LOG_ERROR("Module (%.*s) imports (%.*s) import statements may not include the following characters ('.', ':')", moduleInfo.name.string().length(), moduleInfo.name.string().c_str(), moduleImport.name.length(), moduleImport.name.data());
                return false;
            }
        }

        fs::path selectedPath = baseIncludePath.replace_extension(".nai");

        if (CheckModuleAtPath(compiler, moduleInfo, moduleImport, selectedPath))
        {
            isFound = true;
            isDirectory = fs::is_directory(selectedPath);
        }

        if (fs::is_regular_file(selectedPath))
        {
            // Check Include Paths
            for (fs::path includePath : compiler->GetIncludePaths())
            {
                // Ignore include Paths that is in the same directory as this Module
                if (includePath == moduleInfo.parentPath)
                    continue;

                moduleImport = moduleInfo.moduleImports[i]; // Vector Might Grow in CheckModuleAtPath (This is a nice hack fix)
                fs::path path = (includePath / moduleImport.name).replace_extension(".nai");

                if (isFound)
                {
                    bool doesExist = false;
                    if (isDirectory)
                    {
                        path = path.replace_extension();
                        doesExist = fs::is_directory(path);
                    }
                    else
                    {
                        doesExist = fs::exists(path);
                    }

                    if (doesExist == true)
                    {
                        NC_LOG_WARNING("Module (%.*s) imports (%.*s) from Path('%.*s') but also exists at Path('%.*s')", moduleInfo.name.string().length(), moduleInfo.name.string().c_str(), moduleImport.name.length(), moduleImport.name.data(), selectedPath.string().length(), selectedPath.string().c_str(), path.string().length(), path.string().c_str());
                        break;
                    }
                }
                else
                {
                    if (CheckModuleAtPath(compiler, moduleInfo, moduleImport, selectedPath))
                    {
                        isFound = true;
                        isDirectory = fs::is_directory(selectedPath);
                    }
                }
            }
        }

        if (!isFound)
        {
            NC_LOG_ERROR("Module (%.*s) imports (%.*s) the import could not be resolved", moduleInfo.name.string().length(), moduleInfo.name.string().c_str(), moduleImport.name.length(), moduleImport.name.data());
        }

        modulesFound += 1 * isFound;
    }

    return numModules == modulesFound;
}

bool Parser::CreateAst(ModuleInfo& moduleInfo)
{
    std::atomic<int> errorCount = 0;

    std::for_each(std::execution::par, moduleInfo.compileUnits.begin(), moduleInfo.compileUnits.end(),
        [&errorCount](CompileUnit& compileUnit)
        {
            int numTokens = static_cast<int>(compileUnit.tokens.size());
            int tokenIndex = 0;
            bool localDidError = false;

            Token::Type expectedTokenType = Token::Type::NONE;

            if (compileUnit.type == CompileUnit::Type::FUNCTION)
                expectedTokenType = Token::Type::KEYWORD_FUNCTION;
            else if (compileUnit.type == CompileUnit::Type::STRUCT)
                expectedTokenType = Token::Type::KEYWORD_STRUCT;
            else if (compileUnit.type == CompileUnit::Type::ENUM)
                expectedTokenType = Token::Type::KEYWORD_ENUM;

            for (; tokenIndex < numTokens;)
            {
                const Token& currentToken = compileUnit.Peek(tokenIndex);
                if (currentToken.type == expectedTokenType)
                    break;

                compileUnit.Eat(tokenIndex);
            }

            if (tokenIndex == numTokens)
                errorCount += 1;

            for (; tokenIndex < numTokens;)
            {
                const Token& currentToken = compileUnit.Peek(tokenIndex);

                if (currentToken.type == Token::Type::KEYWORD_FUNCTION)
                {
                    if (!CreateFunctionAst(compileUnit, numTokens, tokenIndex))
                    {
                        localDidError = true;
                        break;
                    }
                }
                else if (currentToken.type == Token::Type::KEYWORD_STRUCT)
                {
                    tokenIndex = numTokens;

                    localDidError = true;
                    break;
                }
                else if (currentToken.type == Token::Type::KEYWORD_ENUM)
                {
                    tokenIndex = numTokens;

                    localDidError = true;
                    break;
                }
                else
                {
                    localDidError = true;
                    break;
                }
            }

            if (localDidError == false)
            {
                NC_LOG_MESSAGE("Compile Unit (%.*s) passed ast creation", compileUnit.name.length(), compileUnit.name.data());
            }
            else
            {
                errorCount += 1;
                NC_LOG_MESSAGE("Compile Unit (%.*s) failed to pass ast creation", compileUnit.name.length(), compileUnit.name.data());
            }
        });

    if (errorCount == 0)
    {
        NC_LOG_MESSAGE("Module (%.*s) passed ast creation", moduleInfo.name.string().length(), moduleInfo.name.string().c_str());
    }
    else
    {
        NC_LOG_MESSAGE("Module (%.*s) failed to pass ast creation", moduleInfo.name.string().length(), moduleInfo.name.string().c_str());
    }

    return errorCount == 0;
}

bool Parser::CheckSemantics(ModuleInfo& /*moduleInfo*/)
{
    return true;
}

bool Parser::CheckSyntaxImport(ModuleInfo& moduleInfo, ModuleDefinition& definition, const int& numTokens)
{
    // This pointer is always stable because we have reserved enough space in the vector holding the ModuleImports
    ModuleImport& currentImport = moduleInfo.moduleImports.emplace_back();

    std::stack<ParseRuleSet> ruleSetStack;
    std::stack<Token::Type> tokenTypeStack;

    ruleSetStack.push(ParseRuleSet::IMPORT);
    tokenTypeStack.push(Token::Type::NONE);

    for (int tokenIndex = 0; tokenIndex < numTokens;)
    {
        const Token& currentToken = definition.Peek(tokenIndex);
        const Token::Type& expectedTokenType = tokenTypeStack.top();

        if (currentToken.type == expectedTokenType)
        {
            tokenIndex += 1;
            tokenTypeStack.pop();
            continue;
        }

        const ParseRuleSet& rule = ruleSetStack.top();
        ruleSetStack.pop();

        switch (rule)
        {
            case ParseRuleSet::IMPORT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_IMPORT:
                        tokenTypeStack.push(Token::Type::KEYWORD_IMPORT);

                        ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                        ruleSetStack.push(ParseRuleSet::IMPORT_MODULE_NAME);
                        break;

                    default:
                        return false;
                }

                break;
            }

            case ParseRuleSet::IMPORT_MODULE_NAME:
            {
                switch (currentToken.type)
                {
                    case Token::Type::STRING:
                        tokenTypeStack.push(Token::Type::STRING);

                        currentImport.name = currentToken.stringview;
                        currentImport.hash = currentToken.hash;
                        break;

                    default:
                        return false;
                }

                break;
            }

            case ParseRuleSet::END_OF_LINE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::END_OF_LINE:
                        tokenTypeStack.push(Token::Type::END_OF_LINE);
                        break;

                    default:
                        return false;
                }

                break;
            }

            default:
                return false;
        }
    }

    return tokenTypeStack.size() == 1 && ruleSetStack.size() == 0;
}

bool Parser::CheckSyntaxAttribute(CompileUnit& compileUnit, const int& numTokens)
{
    std::string_view currentAttributeName = "";

    std::stack<ParseRuleSet> ruleSetStack;
    std::stack<Token::Type> tokenTypeStack;

    ruleSetStack.push(ParseRuleSet::ATTRIBUTE);
    tokenTypeStack.push(Token::Type::NONE);

    for (int tokenIndex = 0; tokenIndex < numTokens;)
    {
        const Token& currentToken = compileUnit.PeekAttribute(tokenIndex);
        const Token::Type& expectedTokenType = tokenTypeStack.top();

        if (currentToken.type == expectedTokenType)
        {
            tokenIndex += 1;
            tokenTypeStack.pop();
            continue;
        }

        const ParseRuleSet& rule = ruleSetStack.top();
        ruleSetStack.pop();

        switch (rule)
        {
            case ParseRuleSet::ATTRIBUTE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ATTRIBUTE:
                        tokenTypeStack.push(Token::Type::ATTRIBUTE);

                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                        ruleSetStack.push(ParseRuleSet::ATTRIBUTE_OPTION);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACKET);
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::ATTRIBUTE_ASSIGNMENT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::ATTRIBUTE_OPTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::ATTRIBUTE_OPTION_NEXT);
                        ruleSetStack.push(ParseRuleSet::ATTRIBUTE_OPTION_VALUE);
                        ruleSetStack.push(ParseRuleSet::ATTRIBUTE_ASSIGNMENT);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);

                        currentAttributeName = currentToken.stringview;
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::ATTRIBUTE_OPTION_VALUE:
            {
                if (currentAttributeName == "Name")
                {
                    if (currentToken.type != Token::Type::STRING)
                        return false;
                        
                    compileUnit.name = currentToken.stringview;
                }
                else if (currentAttributeName == "ParseResult")
                {
                    if (currentToken.type == Token::Type::KEYWORD_TRUE)
                    {
                        compileUnit.attributes.parseResult = true;
                    }
                    else if (currentToken.type == Token::Type::KEYWORD_FALSE)
                    {
                        compileUnit.attributes.parseResult = false;
                    }
                    else
                        return false;
                }

                tokenTypeStack.push(currentToken.type);
                break;
            }
            case ParseRuleSet::ATTRIBUTE_OPTION_NEXT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        tokenTypeStack.push(Token::Type::PARAM_SEPERATOR);

                        ruleSetStack.push(ParseRuleSet::ATTRIBUTE_OPTION);
                        break;

                    case Token::Type::RBRACKET:
                        break;

                    default:
                        return false;
                }

                break;
            }
            
            case ParseRuleSet::IDENTIFIER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        tokenTypeStack.push(Token::Type::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_BRACKET:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_BRACKET:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RBRACKET:
                        tokenTypeStack.push(Token::Type::RBRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }

            default:
                return false;
        }
    }

    return tokenTypeStack.size() == 1 && ruleSetStack.size() == 0;
}

bool Parser::CheckSyntaxFunction(CompileUnit& compileUnit, const int& numTokens)
{
    std::stack<ParseRuleSet> ruleSetStack;
    std::stack<Token::Type> tokenTypeStack;

    ruleSetStack.push(ParseRuleSet::FUNCTION);
    tokenTypeStack.push(Token::Type::NONE);

    for (int tokenIndex = 0; tokenIndex < numTokens;)
    {
        const Token& currentToken = compileUnit.Peek(tokenIndex);
        const Token::Type& expectedTokenType = tokenTypeStack.top();

        if (currentToken.type == expectedTokenType)
        {
            tokenIndex += 1;
            tokenTypeStack.pop();
            continue;
        }

        const ParseRuleSet& rule = ruleSetStack.top();

        if (ruleSetStack.size() > 1)
            ruleSetStack.pop();

        switch (rule)
        {
            case ParseRuleSet::FUNCTION_EXPRESSION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    case Token::Type::ASTERISK:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ASTERISK);
                        break;

                    case Token::Type::NUMERIC_SIGNED:
                    case Token::Type::NUMERIC_UNSIGNED:
                    case Token::Type::NUMERIC_FLOAT:
                    case Token::Type::NUMERIC_DOUBLE:
                    case Token::Type::NUMERIC_HEX:
                    case Token::Type::STRING:
                    case Token::Type::KEYWORD_TRUE:
                    case Token::Type::KEYWORD_FALSE:
                    case Token::Type::KEYWORD_NULLPTR:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                        break;

                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_CALL_EXPRESSION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    case Token::Type::ASTERISK:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ASTERISK);
                        break;

                    case Token::Type::NUMERIC_SIGNED:
                    case Token::Type::NUMERIC_UNSIGNED:
                    case Token::Type::NUMERIC_FLOAT:
                    case Token::Type::NUMERIC_DOUBLE:
                    case Token::Type::NUMERIC_HEX:
                    case Token::Type::STRING:
                    case Token::Type::KEYWORD_TRUE:
                    case Token::Type::KEYWORD_FALSE:
                    case Token::Type::KEYWORD_NULLPTR:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                        break;

                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;

                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                        break;

                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS);
                        break;

                    case Token::Type::LBRACKET:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS);
                        break;

                    case Token::Type::ASTERISK:
                    case Token::Type::PLUS:
                    case Token::Type::MINUS:
                    case Token::Type::SLASH:
                    case Token::Type::PERCENT:
                    case Token::Type::AMPERSAND:
                    case Token::Type::BITSHIFT_LEFT:
                    case Token::Type::BITSHIFT_RIGHT:
                    case Token::Type::CARET:
                    case Token::Type::PIPE:
                    case Token::Type::EQUALS:
                    case Token::Type::LESS_THAN:
                    case Token::Type::GREATER_THAN:
                    case Token::Type::AND:
                    case Token::Type::OR:
                    case Token::Type::RPAREN:
                    case Token::Type::PARAM_SEPERATOR:
                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                        break;

                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ACCESS);
                        break;

                    case Token::Type::LBRACKET:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS);
                        break;

                    case Token::Type::ASTERISK:
                    case Token::Type::PLUS:
                    case Token::Type::MINUS:
                    case Token::Type::SLASH:
                    case Token::Type::PERCENT:
                    case Token::Type::AMPERSAND:
                    case Token::Type::BITSHIFT_LEFT:
                    case Token::Type::BITSHIFT_RIGHT:
                    case Token::Type::CARET:
                    case Token::Type::PIPE:
                    case Token::Type::EQUALS:
                    case Token::Type::LESS_THAN:
                    case Token::Type::GREATER_THAN:
                    case Token::Type::AND:
                    case Token::Type::OR:
                    case Token::Type::RPAREN:
                    case Token::Type::PARAM_SEPERATOR:
                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ACCESS);
                        break;

                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION_IDENTIFIER_ARRAY_ACCESS);
                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    case Token::Type::ASTERISK:
                    case Token::Type::PLUS:
                    case Token::Type::MINUS:
                    case Token::Type::SLASH:
                    case Token::Type::PERCENT:
                    case Token::Type::AMPERSAND:
                    case Token::Type::BITSHIFT_LEFT:
                    case Token::Type::BITSHIFT_RIGHT:
                    case Token::Type::CARET:
                    case Token::Type::PIPE:
                    case Token::Type::EQUALS:
                    case Token::Type::LESS_THAN:
                    case Token::Type::GREATER_THAN:
                    case Token::Type::AND:
                    case Token::Type::OR:
                    case Token::Type::RPAREN:
                    case Token::Type::PARAM_SEPERATOR:
                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_EXPRESSION_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                    case Token::Type::PLUS:
                    case Token::Type::MINUS:
                    case Token::Type::SLASH:
                    case Token::Type::PERCENT:
                    case Token::Type::AMPERSAND:
                    case Token::Type::BITSHIFT_LEFT:
                    case Token::Type::BITSHIFT_RIGHT:
                    case Token::Type::CARET:
                    case Token::Type::PIPE:
                    case Token::Type::EQUALS:
                    case Token::Type::LESS_THAN:
                    case Token::Type::GREATER_THAN:
                    case Token::Type::AND:
                    case Token::Type::OR:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    case Token::Type::PERIOD:
                    case Token::Type::RPAREN:
                    case Token::Type::RBRACE:
                    case Token::Type::RBRACKET:
                    case Token::Type::PARAM_SEPERATOR:
                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }

            case ParseRuleSet::FUNCTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_FUNCTION:
                        tokenTypeStack.push(Token::Type::KEYWORD_FUNCTION);

                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_RETURN_TYPE);
                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_PARAMETER);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);

                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::FUNCTION_PARAMETER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_PARAMETER_NEXT);
                        ruleSetStack.push(ParseRuleSet::DECLARATION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::FUNCTION_PARAMETER_NEXT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_PARAMETER);
                        ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                        break;

                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::FUNCTION_RETURN_TYPE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RETURN_TYPE:
                        tokenTypeStack.push(Token::Type::RETURN_TYPE);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_RETURN_TYPE_ACTION);
                        break;

                    case Token::Type::LBRACE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_RETURN_TYPE_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::DATATYPE_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::DATATYPE);
                        break;

                    case Token::Type::LBRACKET:
                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_RETURN_TYPE_ACTION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::DATATYPE_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::DATATYPE);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_RETURN_TYPE_ACTION_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_RETURN_TYPE_ACTION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::DATATYPE_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::DATATYPE);
                        ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                        break;

                    case Token::Type::RBRACKET:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_BODY:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ASTERISK);
                        break;

                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER);
                        break;

                    case Token::Type::KEYWORD_IF:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IF);
                        break;

                    case Token::Type::KEYWORD_WHILE:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_WHILE);
                        break;

                    case Token::Type::KEYWORD_FOR:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOR);
                        break;

                    case Token::Type::KEYWORD_FOREACH:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH);
                        break;

                    case Token::Type::KEYWORD_CONTINUE:
                    case Token::Type::KEYWORD_BREAK:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        break;

                    case Token::Type::KEYWORD_RETURN:
                        tokenTypeStack.push(Token::Type::KEYWORD_RETURN);

                        ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    case Token::Type::RBRACE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_IDENTIFIER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACTION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_IDENTIFIER_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::DECLARATION:
                    case Token::Type::DECLARATION_CONST:
                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT);
                        ruleSetStack.push(ParseRuleSet::DECLARATION);
                        break;

                    case Token::Type::PARAM_SEPERATOR:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE);
                        break;

                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                        break;

                    case Token::Type::LBRACKET:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ARRAY_ACCESS);
                        break;

                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS);
                        break;

                    case Token::Type::ASSIGN:
                    case Token::Type::MODULUS_EQUALS:
                    case Token::Type::BITWISE_AND_EQUALS:
                    case Token::Type::MULTIPLY_EQUALS:
                    case Token::Type::PLUS_EQUALS:
                    case Token::Type::MINUS_EQUALS:
                    case Token::Type::DIVIDE_EQUALS:
                    case Token::Type::BITSHIFT_LEFT_EQUALS:
                    case Token::Type::BITSHIFT_RIGHT_EQUALS:
                    case Token::Type::POW_EQUALS:
                    case Token::Type::BITWISE_OR_EQUALS:
                        ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                        break;

                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_ASSIGN);
                        break;

                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_ASSIGN:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    case Token::Type::LBRACKET:
                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACKET);
                        break;

                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_MULTI_DECLARE_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                        break;

                    case Token::Type::RBRACKET:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                        break;

                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ACCESS);
                        break;

                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ARRAY_ACCESS);
                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    case Token::Type::ASSIGN:
                    case Token::Type::MODULUS_EQUALS:
                    case Token::Type::BITWISE_AND_EQUALS:
                    case Token::Type::MULTIPLY_EQUALS:
                    case Token::Type::PLUS_EQUALS:
                    case Token::Type::MINUS_EQUALS:
                    case Token::Type::DIVIDE_EQUALS:
                    case Token::Type::BITSHIFT_LEFT_EQUALS:
                    case Token::Type::BITSHIFT_RIGHT_EQUALS:
                    case Token::Type::POW_EQUALS:
                    case Token::Type::BITWISE_OR_EQUALS:
                        ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                        break;

                    case Token::Type::END_OF_LINE:
                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_IDENTIFIER_ARRAY_ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ACCESS);
                        break;

                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ARRAY_ACCESS);
                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    case Token::Type::ASSIGN:
                    case Token::Type::MODULUS_EQUALS:
                    case Token::Type::BITWISE_AND_EQUALS:
                    case Token::Type::MULTIPLY_EQUALS:
                    case Token::Type::PLUS_EQUALS:
                    case Token::Type::MINUS_EQUALS:
                    case Token::Type::DIVIDE_EQUALS:
                    case Token::Type::BITSHIFT_LEFT_EQUALS:
                    case Token::Type::BITSHIFT_RIGHT_EQUALS:
                    case Token::Type::POW_EQUALS:
                    case Token::Type::BITWISE_OR_EQUALS:
                        ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                        break;

                    default:
                        return false;
                }
                break;
            }

            case ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST:
            {
                ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER);
                ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                break;
            }
            case ParseRuleSet::FUNCTION_CALL_PARAMETER:
            {
                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_SEQUENCE);
                ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_EXPRESSION);
                break;
            }
            case ParseRuleSet::FUNCTION_CALL_PARAMETER_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER);
                        ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                        break;

                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }

                break;
            }

            case ParseRuleSet::FUNCTION_IF:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_IF:
                    case Token::Type::KEYWORD_ELSEIF:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_IF_NEXT);
                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;

                    case Token::Type::KEYWORD_ELSE:
                        tokenTypeStack.push(Token::Type::KEYWORD_ELSE);

                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_IF_NEXT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_ELSEIF:
                    case Token::Type::KEYWORD_ELSE:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IF);
                        break;

                    default:
                        break;
                }
                break;
            }

            case ParseRuleSet::FUNCTION_WHILE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_WHILE:
                        tokenTypeStack.push(Token::Type::KEYWORD_WHILE);

                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }

            case ParseRuleSet::FUNCTION_FOR:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_FOR:
                        tokenTypeStack.push(Token::Type::KEYWORD_FOR);

                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_ACTION);
                        ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_CONDITION);
                        ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_FOR_HEADER_IDENTIFIER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOR_HEADER_IDENTIFIER_ACTION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_FOR_HEADER_IDENTIFIER_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::DECLARATION:
                    case Token::Type::DECLARATION_CONST:
                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT);
                        ruleSetStack.push(ParseRuleSet::DECLARATION);
                        break;

                    case Token::Type::ASSIGN:
                        ruleSetStack.push(ParseRuleSet::ASSIGNMENT);
                        break;

                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_FOR_HEADER_CONDITION:
            {
                ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                break;
            }
            case ParseRuleSet::FUNCTION_FOR_HEADER_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACTION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    case Token::Type::ASTERISK:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_IDENTIFIER_ACTION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ASTERISK);
                        break;

                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }
                break;
            }

            case ParseRuleSet::FUNCTION_FOREACH:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_FOREACH:
                        tokenTypeStack.push(Token::Type::KEYWORD_FOREACH);

                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_BODY);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_ACTION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_FOREACH_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IN);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::PARAM_SEPERATOR);
                        break;

                    case Token::Type::KEYWORD_IN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_FOREACH_IN:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_IN:
                        tokenTypeStack.push(Token::Type::KEYWORD_IN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                        break;

                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS);
                        break;

                    case Token::Type::LBRACKET:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS);
                        break;

                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }
                break;
            }

            case ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_CALL_PARAMETER_LIST);
                        break;

                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ACCESS);
                        break;

                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS);
                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PERIOD:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ACCESS);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        ruleSetStack.push(ParseRuleSet::ACCESS);
                        break;

                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_FOREACH_IDENTIFIER_ARRAY_ACCESS);
                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACKET);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    case Token::Type::RPAREN:
                        break;

                    default:
                        return false;
                }
                break;
            }

            case ParseRuleSet::IDENTIFIER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        tokenTypeStack.push(Token::Type::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::STRING:
            {
                switch (currentToken.type)
                {
                    case Token::Type::STRING:
                        tokenTypeStack.push(Token::Type::STRING);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DATATYPE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        tokenTypeStack.push(Token::Type::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DATATYPE_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                        tokenTypeStack.push(Token::Type::ASTERISK);
                        break;

                    default:
                        break;
                }
                break;
            }
            case ParseRuleSet::DECLARATION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::DECLARATION:
                    case Token::Type::DECLARATION_CONST:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::DATATYPE_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::DATATYPE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);

                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION);
                        break;

                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::UNINITIALIZED:
                    case Token::Type::KEYWORD_NULLPTR:
                        tokenTypeStack.push(currentToken.type);
                        break;

                    case Token::Type::AMPERSAND:
                        tokenTypeStack.push(Token::Type::AMPERSAND);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    default:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT_FORCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);

                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ASSIGNMENT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                    case Token::Type::MODULUS_EQUALS:
                    case Token::Type::BITWISE_AND_EQUALS:
                    case Token::Type::MULTIPLY_EQUALS:
                    case Token::Type::PLUS_EQUALS:
                    case Token::Type::MINUS_EQUALS:
                    case Token::Type::DIVIDE_EQUALS:
                    case Token::Type::BITSHIFT_LEFT_EQUALS:
                    case Token::Type::BITSHIFT_RIGHT_EQUALS:
                    case Token::Type::POW_EQUALS:
                    case Token::Type::BITWISE_OR_EQUALS:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PERIOD:
                        tokenTypeStack.push(Token::Type::PERIOD);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ASTERISK:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                        tokenTypeStack.push(Token::Type::ASTERISK);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::PARAM_SEPERATOR:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        tokenTypeStack.push(Token::Type::PARAM_SEPERATOR);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_PAREN:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LPAREN:
                        tokenTypeStack.push(Token::Type::LPAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_BRACE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LBRACE:
                        tokenTypeStack.push(Token::Type::LBRACE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_BRACKET:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_PAREN:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RPAREN:
                        tokenTypeStack.push(Token::Type::RPAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_BRACE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RBRACE:
                        tokenTypeStack.push(Token::Type::RBRACE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_BRACKET:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RBRACKET:
                        tokenTypeStack.push(Token::Type::RBRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::END_OF_LINE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::END_OF_LINE:
                        tokenTypeStack.push(Token::Type::END_OF_LINE);
                        break;

                    default:
                        return false;
                }
                break;
            }

            default:
                return false;
        }
    }

    return tokenTypeStack.size() == 1 && ruleSetStack.size() == 1;
}

bool Parser::CheckSyntaxStruct(CompileUnit& compileUnit, const int& numTokens)
{
    std::stack<ParseRuleSet> ruleSetStack;
    std::stack<Token::Type> tokenTypeStack;

    ruleSetStack.push(ParseRuleSet::STRUCT);
    tokenTypeStack.push(Token::Type::NONE);

    for (int tokenIndex = 0; tokenIndex < numTokens;)
    {
        const Token& currentToken = compileUnit.Peek(tokenIndex);
        const Token::Type& expectedTokenType = tokenTypeStack.top();

        if (currentToken.type == expectedTokenType)
        {
            tokenIndex += 1;
            tokenTypeStack.pop();
            continue;
        }

        const ParseRuleSet& rule = ruleSetStack.top();

        if (ruleSetStack.size() > 1)
            ruleSetStack.pop();

        switch (rule)
        {
            case ParseRuleSet::STRUCT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_STRUCT:
                        tokenTypeStack.push(Token::Type::KEYWORD_STRUCT);

                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                        ruleSetStack.push(ParseRuleSet::STRUCT_BODY);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::STRUCT_BODY:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::STRUCT_BODY);
                        ruleSetStack.push(ParseRuleSet::STRUCT_IDENTIFIER);
                        break;

                    case Token::Type::KEYWORD_FUNCTION:
                        ruleSetStack.push(ParseRuleSet::STRUCT_BODY);
                        ruleSetStack.push(ParseRuleSet::FUNCTION);
                        break;

                    case Token::Type::RBRACE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::STRUCT_EXPRESSION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::NUMERIC_SIGNED:
                    case Token::Type::NUMERIC_UNSIGNED:
                    case Token::Type::NUMERIC_FLOAT:
                    case Token::Type::NUMERIC_DOUBLE:
                    case Token::Type::NUMERIC_HEX:
                    case Token::Type::STRING:
                    case Token::Type::KEYWORD_TRUE:
                    case Token::Type::KEYWORD_FALSE:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION_SEQUENCE);
                        break;

                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::STRUCT_EXPRESSION_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                    case Token::Type::PLUS:
                    case Token::Type::MINUS:
                    case Token::Type::SLASH:
                    case Token::Type::PERCENT:
                    case Token::Type::AMPERSAND:
                    case Token::Type::BITSHIFT_LEFT:
                    case Token::Type::BITSHIFT_RIGHT:
                    case Token::Type::CARET:
                    case Token::Type::PIPE:
                    case Token::Type::EQUALS:
                    case Token::Type::LESS_THAN:
                    case Token::Type::GREATER_THAN:
                    case Token::Type::AND:
                    case Token::Type::OR:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION);
                        break;

                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::STRUCT_IDENTIFIER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::END_OF_LINE);
                        ruleSetStack.push(ParseRuleSet::STRUCT_IDENTIFIER_ACTION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::STRUCT_IDENTIFIER_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::DECLARATION:
                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT);
                        ruleSetStack.push(ParseRuleSet::DECLARATION);
                        break;

                    case Token::Type::DECLARATION_CONST:
                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_FORCE);
                        ruleSetStack.push(ParseRuleSet::DECLARATION);
                        break;

                    default:
                        return false;
                }
                break;
            }

            case ParseRuleSet::IDENTIFIER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        tokenTypeStack.push(Token::Type::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::STRING:
            {
                switch (currentToken.type)
                {
                    case Token::Type::STRING:
                        tokenTypeStack.push(Token::Type::STRING);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DATATYPE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        tokenTypeStack.push(Token::Type::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DATATYPE_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                        tokenTypeStack.push(Token::Type::ASTERISK);
                        break;

                    default:
                        break;
                }
                break;
            }
            case ParseRuleSet::DECLARATION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::DECLARATION:
                    case Token::Type::DECLARATION_CONST:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::DATATYPE_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::DATATYPE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);

                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION);
                        break;

                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::UNINITIALIZED:
                    case Token::Type::KEYWORD_NULLPTR:
                        tokenTypeStack.push(currentToken.type);
                        break;

                    case Token::Type::AMPERSAND:
                        tokenTypeStack.push(Token::Type::AMPERSAND);
                        ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION);
                        break;

                    default:
                        ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION);
                        break;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT_FORCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);

                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ASSIGNMENT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                    case Token::Type::MODULUS_EQUALS:
                    case Token::Type::BITWISE_AND_EQUALS:
                    case Token::Type::MULTIPLY_EQUALS:
                    case Token::Type::PLUS_EQUALS:
                    case Token::Type::MINUS_EQUALS:
                    case Token::Type::DIVIDE_EQUALS:
                    case Token::Type::BITSHIFT_LEFT_EQUALS:
                    case Token::Type::BITSHIFT_RIGHT_EQUALS:
                    case Token::Type::POW_EQUALS:
                    case Token::Type::BITWISE_OR_EQUALS:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::STRUCT_EXPRESSION);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PERIOD:
                        tokenTypeStack.push(Token::Type::PERIOD);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ASTERISK:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                        tokenTypeStack.push(Token::Type::ASTERISK);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::PARAM_SEPERATOR:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        tokenTypeStack.push(Token::Type::PARAM_SEPERATOR);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_PAREN:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LPAREN:
                        tokenTypeStack.push(Token::Type::LPAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_BRACE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LBRACE:
                        tokenTypeStack.push(Token::Type::LBRACE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_BRACKET:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_PAREN:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RPAREN:
                        tokenTypeStack.push(Token::Type::RPAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_BRACE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RBRACE:
                        tokenTypeStack.push(Token::Type::RBRACE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_BRACKET:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RBRACKET:
                        tokenTypeStack.push(Token::Type::RBRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::END_OF_LINE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::END_OF_LINE:
                        tokenTypeStack.push(Token::Type::END_OF_LINE);
                        break;

                    default:
                        return false;
                }
                break;
            }

            default:
                return false;
        }
    }

    bool didSucceed = tokenTypeStack.size() == 1 && ruleSetStack.size() == 1;
    if (didSucceed)
    {
        const Token& firstToken = compileUnit.Peek(0);
        
        TypeInfo typeInfo(compileUnit.moduleNameHash, firstToken.stringview, NaiType::STRUCT, 0);

        if (!CompilerInfo::AddTypeInfo(typeInfo))
            return false;
    }

    return didSucceed;
}

bool Parser::CheckSyntaxEnum(CompileUnit& compileUnit, const int& numTokens)
{
    std::stack<ParseRuleSet> ruleSetStack;
    std::stack<Token::Type> tokenTypeStack;

    ruleSetStack.push(ParseRuleSet::ENUM);
    tokenTypeStack.push(Token::Type::NONE);

    for (int tokenIndex = 0; tokenIndex < numTokens;)
    {
        const Token& currentToken = compileUnit.Peek(tokenIndex);
        const Token::Type& expectedTokenType = tokenTypeStack.top();

        if (currentToken.type == expectedTokenType)
        {
            tokenIndex += 1;
            tokenTypeStack.pop();
            continue;
        }

        const ParseRuleSet& rule = ruleSetStack.top();

        if (ruleSetStack.size() > 1)
            ruleSetStack.pop();

        switch (rule)
        {
            case ParseRuleSet::ENUM:
            {
                switch (currentToken.type)
                {
                    case Token::Type::KEYWORD_ENUM:
                        tokenTypeStack.push(Token::Type::KEYWORD_ENUM);

                        ruleSetStack.push(ParseRuleSet::CLOSE_BRACE);
                        ruleSetStack.push(ParseRuleSet::ENUM_BODY);
                        ruleSetStack.push(ParseRuleSet::OPEN_BRACE);

                        ruleSetStack.push(ParseRuleSet::ENUM_DECLARATION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::ENUM_BODY:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::ENUM_IDENTIFIER_ACTION);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    case Token::Type::RBRACE:
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::ENUM_EXPRESSION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::IDENTIFIER);
                        break;

                    case Token::Type::NUMERIC_SIGNED:
                    case Token::Type::NUMERIC_UNSIGNED:
                    case Token::Type::NUMERIC_FLOAT:
                    case Token::Type::NUMERIC_DOUBLE:
                    case Token::Type::NUMERIC_HEX:
                    case Token::Type::STRING:
                    case Token::Type::KEYWORD_TRUE:
                    case Token::Type::KEYWORD_FALSE:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION_SEQUENCE);
                        break;

                    case Token::Type::LPAREN:
                        ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::CLOSE_PAREN);
                        ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION);
                        ruleSetStack.push(ParseRuleSet::OPEN_PAREN);
                        break;

                    case Token::Type::PARAM_SEPERATOR:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ENUM_EXPRESSION_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                    case Token::Type::PLUS:
                    case Token::Type::MINUS:
                    case Token::Type::SLASH:
                    case Token::Type::PERCENT:
                    case Token::Type::AMPERSAND:
                    case Token::Type::BITSHIFT_LEFT:
                    case Token::Type::BITSHIFT_RIGHT:
                    case Token::Type::CARET:
                    case Token::Type::PIPE:
                    case Token::Type::EQUALS:
                    case Token::Type::LESS_THAN:
                    case Token::Type::GREATER_THAN:
                    case Token::Type::AND:
                    case Token::Type::OR:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION);
                        break;

                    case Token::Type::PARAM_SEPERATOR:
                    case Token::Type::RBRACE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ENUM_DECLARATION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::DECLARATION:
                        tokenTypeStack.push(Token::Type::DECLARATION);

                        ruleSetStack.push(ParseRuleSet::DATATYPE);
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::ENUM_IDENTIFIER_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);

                        ruleSetStack.push(ParseRuleSet::ENUM_IDENTIFIER_ACTION_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::ENUM_EXPRESSION);
                        break;

                    case Token::Type::PARAM_SEPERATOR:
                        ruleSetStack.push(ParseRuleSet::ENUM_IDENTIFIER_ACTION_SEQUENCE);
                        break;

                    case Token::Type::RBRACE:
                        break;

                    default:
                        return false;
                }

                break;
            }
            case ParseRuleSet::ENUM_IDENTIFIER_ACTION_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        tokenTypeStack.push(Token::Type::PARAM_SEPERATOR);

                        ruleSetStack.push(ParseRuleSet::ENUM_BODY);
                        break;

                    case Token::Type::RBRACE:
                        break;

                    default:
                        return false;
                }

                break;
            }

            case ParseRuleSet::IDENTIFIER:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        tokenTypeStack.push(Token::Type::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::STRING:
            {
                switch (currentToken.type)
                {
                    case Token::Type::STRING:
                        tokenTypeStack.push(Token::Type::STRING);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DATATYPE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::IDENTIFIER:
                        tokenTypeStack.push(Token::Type::IDENTIFIER);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DATATYPE_SEQUENCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                        tokenTypeStack.push(Token::Type::ASTERISK);
                        break;

                    default:
                        break;
                }
                break;
            }
            case ParseRuleSet::DECLARATION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::DECLARATION:
                    case Token::Type::DECLARATION_CONST:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::DATATYPE_SEQUENCE);
                        ruleSetStack.push(ParseRuleSet::DATATYPE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);

                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION);
                        break;

                    case Token::Type::END_OF_LINE:
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION:
            {
                switch (currentToken.type)
                {
                    case Token::Type::UNINITIALIZED:
                    case Token::Type::KEYWORD_NULLPTR:
                        tokenTypeStack.push(currentToken.type);
                        break;

                    case Token::Type::AMPERSAND:
                        tokenTypeStack.push(Token::Type::AMPERSAND);
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    default:
                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;
                }
                break;
            }
            case ParseRuleSet::DECLARATION_ASSIGNMENT_FORCE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                        tokenTypeStack.push(Token::Type::ASSIGN);

                        ruleSetStack.push(ParseRuleSet::DECLARATION_ASSIGNMENT_ACTION);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ASSIGNMENT:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASSIGN:
                    case Token::Type::MODULUS_EQUALS:
                    case Token::Type::BITWISE_AND_EQUALS:
                    case Token::Type::MULTIPLY_EQUALS:
                    case Token::Type::PLUS_EQUALS:
                    case Token::Type::MINUS_EQUALS:
                    case Token::Type::DIVIDE_EQUALS:
                    case Token::Type::BITSHIFT_LEFT_EQUALS:
                    case Token::Type::BITSHIFT_RIGHT_EQUALS:
                    case Token::Type::POW_EQUALS:
                    case Token::Type::BITWISE_OR_EQUALS:
                        tokenTypeStack.push(currentToken.type);

                        ruleSetStack.push(ParseRuleSet::FUNCTION_EXPRESSION);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ACCESS:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PERIOD:
                        tokenTypeStack.push(Token::Type::PERIOD);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::ASTERISK:
            {
                switch (currentToken.type)
                {
                    case Token::Type::ASTERISK:
                        tokenTypeStack.push(Token::Type::ASTERISK);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::PARAM_SEPERATOR:
            {
                switch (currentToken.type)
                {
                    case Token::Type::PARAM_SEPERATOR:
                        tokenTypeStack.push(Token::Type::PARAM_SEPERATOR);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_PAREN:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LPAREN:
                        tokenTypeStack.push(Token::Type::LPAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_BRACE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LBRACE:
                        tokenTypeStack.push(Token::Type::LBRACE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::OPEN_BRACKET:
            {
                switch (currentToken.type)
                {
                    case Token::Type::LBRACKET:
                        tokenTypeStack.push(Token::Type::LBRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_PAREN:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RPAREN:
                        tokenTypeStack.push(Token::Type::RPAREN);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_BRACE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RBRACE:
                        tokenTypeStack.push(Token::Type::RBRACE);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::CLOSE_BRACKET:
            {
                switch (currentToken.type)
                {
                    case Token::Type::RBRACKET:
                        tokenTypeStack.push(Token::Type::RBRACKET);
                        break;

                    default:
                        return false;
                }
                break;
            }
            case ParseRuleSet::END_OF_LINE:
            {
                switch (currentToken.type)
                {
                    case Token::Type::END_OF_LINE:
                        tokenTypeStack.push(Token::Type::END_OF_LINE);
                        break;

                    default:
                        return false;
                }
                break;
            }

            default:
                return false;
        }
    }

    return tokenTypeStack.size() == 1 && ruleSetStack.size() == 1;
}

bool Parser::CheckModuleAtPath(Compiler* compiler, ModuleInfo& moduleInfo, ModuleImport& currentImport, fs::path& path)
{
    if (fs::exists(path))
    {
        currentImport.path = path;
        currentImport.pathHash = StringUtils::hash_djb2(path.string().c_str(), path.string().length());
        currentImport.isFound = true;

        compiler->AddImport(path);
    }
    else
    {
        path = path.replace_extension();

        if (fs::is_directory(path))
        {
            currentImport.path = path;
            currentImport.pathHash = StringUtils::hash_djb2(path.string().c_str(), path.string().length());
            currentImport.isFound = true;
            currentImport.isDirectory = true;

            for (auto& entry : fs::directory_iterator(path))
            {
                const fs::path& subPath = entry.path();
                if (fs::is_regular_file(subPath) && subPath.extension() == ".nai")
                {
                    ModuleImport& subModuleImport = moduleInfo.moduleImports.emplace_back();
                    subModuleImport.path = subPath;
                    subModuleImport.pathHash = StringUtils::hash_djb2(subPath.string().c_str(), subPath.string().length());
                    subModuleImport.isFound = true;

                    compiler->AddImport(subPath);
                }
            }
        }
    }

    return currentImport.isFound;
}

bool Parser::CreateFunctionAst(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex)
{
    compileUnit.Eat(tokenIndex); // KEYWORD_FUNCTION

    compileUnit.astFunction = compileUnit.CreateFunction(&compileUnit.Peek(tokenIndex));

    compileUnit.Eat(tokenIndex); // IDENTIFIER

    if (!CreateFunctionHeaderAst(compileUnit, numTokens, tokenIndex))
        return false;

    if (!CreateFunctionBodyAst(compileUnit, numTokens, tokenIndex))
        return false;

    return true;
}

bool Parser::CreateFunctionHeaderAst(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex)
{
    compileUnit.Eat(tokenIndex); // LPAREN

    // Handle Parameter List
    for (; tokenIndex < numTokens;)
    {
        Token& currentToken = compileUnit.Peek(tokenIndex);
        if (currentToken.type == Token::Type::RPAREN)
            break;

        if (currentToken.type == Token::Type::IDENTIFIER)
        {
            compileUnit.Eat(tokenIndex); // IDENTIFIER
            compileUnit.Eat(tokenIndex); // DECLARATION

            compileUnit.astFunction->numParameters += 1;

            AstVariable& parameter = compileUnit.astFunction->variables.emplace_back(&currentToken);
            if (!GetDataTypeFromTokenIndex(compileUnit, tokenIndex, parameter.dataType))
                return false;

            Token& seperatorToken = compileUnit.Peek(tokenIndex);
            if (seperatorToken.type == Token::Type::PARAM_SEPERATOR)
                compileUnit.Eat(tokenIndex); // PARAM_SEPERATOR
        }
    }

    // Failed to find RPAREN
    if (tokenIndex == numTokens)
        return false;

    compileUnit.Eat(tokenIndex); // RPAREN

    if (compileUnit.Peek(tokenIndex).type == Token::Type::RETURN_TYPE)
    {
        compileUnit.Eat(tokenIndex); // RETURN_TYPE

        // Handle Multi Return
        if (compileUnit.Peek(tokenIndex).type == Token::Type::LBRACKET)
        {
            compileUnit.Eat(tokenIndex); // LBRACKET

            for (; tokenIndex < numTokens;)
            {
                AstDataType& dataType = compileUnit.astFunction->returnTypeNodes.emplace_back();
                if (!GetDataTypeFromTokenIndex(compileUnit, tokenIndex, dataType))
                    return false; 

                if (compileUnit.Peek(tokenIndex).type == Token::Type::PARAM_SEPERATOR)
                {
                    compileUnit.Eat(tokenIndex); // PARAM_SEPERATOR
                    continue;
                }
                   
                break;
            }

            compileUnit.Eat(tokenIndex); // RBRACKET
        }
        else
        {
            AstDataType& dataType = compileUnit.astFunction->returnTypeNodes.emplace_back(&compileUnit.Peek(tokenIndex));
            if (!GetDataTypeFromTokenIndex(compileUnit, tokenIndex, dataType))
                return false;
        }
    }

    return true;
}

bool Parser::CreateFunctionBodyAst(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex)
{
    compileUnit.Eat(tokenIndex); // LBRACE

    compileUnit.astFunction->sequence = compileUnit.CreateNode<AstSequence>();
    AstSequence* currentSequence = compileUnit.astFunction->sequence;

    for (; tokenIndex < numTokens;)
    {
        Token& firstToken = compileUnit.Peek(tokenIndex);
        if (firstToken.type == Token::Type::RBRACE)
            break;

        if (currentSequence->node != nullptr)
        {
            currentSequence->nextSequence = compileUnit.CreateNode<AstSequence>();
            currentSequence = currentSequence->nextSequence;
        }

        switch (firstToken.type)
        {
            case Token::Type::IDENTIFIER:
            {
                Token& secondToken = compileUnit.Peek(tokenIndex + 1);

                switch (secondToken.type)
                {
                    // Function Call
                    case Token::Type::LPAREN:
                    {
                        return false; // break;
                    }

                    // Struct Access
                    case Token::Type::PERIOD:
                    {
                        return false; // break;
                    }

                    // Variable Declaration
                    case Token::Type::DECLARATION:
                    case Token::Type::DECLARATION_CONST:
                    {
                        AstVariable* variable = nullptr;
                        if (!GetVariableDeclarationFromTokenIndex(compileUnit, numTokens, tokenIndex, variable))
                            return false;

                        currentSequence->node = variable;
                        break;
                    }

                    // Variable Assignment
                    case Token::Type::ASSIGN:
                    case Token::Type::PLUS_EQUALS:
                    case Token::Type::MINUS_EQUALS:
                    case Token::Type::MULTIPLY_EQUALS:
                    case Token::Type::DIVIDE_EQUALS:
                    case Token::Type::MODULUS_EQUALS:
                    case Token::Type::POW_EQUALS:
                    case Token::Type::BITSHIFT_LEFT_EQUALS:
                    case Token::Type::BITSHIFT_RIGHT_EQUALS:
                    case Token::Type::BITWISE_AND_EQUALS:
                    case Token::Type::BITWISE_OR_EQUALS:
                    {
                        AstVariable* variable = nullptr;
                        if (!GetVariableFromTokenIndex(compileUnit, numTokens, tokenIndex, variable))
                            return false;

                        currentSequence->node = variable;
                        break;
                    }

                    default:
                        return false;
                }

                compileUnit.Eat(tokenIndex); // END_OF_LINE

                break;
            }

            case Token::Type::KEYWORD_RETURN:
            {
                AstReturn* ret = nullptr;
                if (!GetReturnFromTokenIndex(compileUnit, numTokens, tokenIndex, ret))
                    return false;

                currentSequence->node = ret;
                break;
            }

            default:
                return false;
        }
    }

    compileUnit.Eat(tokenIndex); // RBRACE
    return true;
}

bool Parser::GetExpressionFromTokenIndex(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex, AstExpression& expression)
{
    Token& initialToken = compileUnit.Peek(tokenIndex);

    if (initialToken.type == Token::Type::LPAREN)
    {
        compileUnit.Eat(tokenIndex); // LPAREN

        if (expression.left.node == nullptr)
        {
            expression.lType = AstExpression::Type::EXPRESSION;
            expression.left.node = compileUnit.CreateNode<AstExpression>(&initialToken);
            if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, *expression.left.node))
                return false;
        }
        else
        {
            expression.rType = AstExpression::Type::EXPRESSION;
            expression.right.node = compileUnit.CreateNode<AstExpression>(&initialToken);
            if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, *expression.right.node))
                return false;
        }

        compileUnit.Eat(tokenIndex); // RPAREN
    }
    else if (initialToken.type == Token::Type::IDENTIFIER || (initialToken.type >= Token::Type::NUMERIC_SIGNED && initialToken.type <= Token::Type::STRING))
    {
        // TODO: Implement Function Calls and then add them to expressions
        if (compileUnit.Peek(tokenIndex + 1).type == Token::Type::LPAREN)
            return false;

        bool isVariable = initialToken.type == Token::Type::IDENTIFIER;
        if (isVariable)
        {
            AstVariable* variable = nullptr;
            if (!GetVariableFromTokenIndex(compileUnit, numTokens, tokenIndex, variable))
                return false;

            if (expression.left.node == nullptr)
            {
                expression.lType = AstExpression::Type::VALUE;
                expression.left.val = compileUnit.CreateNode<AstValue>(&initialToken);
                expression.left.val->type = AstValue::Type::VARIABLE;
                expression.left.val->variableIndex = variable->index;
            }
            else
            {
                expression.rType = AstExpression::Type::VALUE;
                expression.right.val = compileUnit.CreateNode<AstValue>(&initialToken);
                expression.right.val->type = AstValue::Type::VARIABLE;
                expression.right.val->variableIndex = variable->index;
            }
        }
        else
        {
            AstLiteral literal;

            switch (initialToken.type)
            {
                case Token::Type::NUMERIC_SIGNED:
                {
                    long val = static_cast<long>(StringUtils::ToUInt64(initialToken.stringview.data(), initialToken.stringview.size()));

                    if (val < std::numeric_limits<int>().min() || val > std::numeric_limits<int>().max())
                    {
                        literal.type = NaiType::I64;
                        literal.size = 8;

                        literal.i64 = static_cast<long>(val);
                    }
                    else
                    {
                        literal.type = NaiType::I32;
                        literal.size = 4;

                        literal.i32 = static_cast<int>(val);
                    }
                    break;
                }

                case Token::Type::NUMERIC_UNSIGNED:
                {
                    uint64_t val = StringUtils::ToUInt64(initialToken.stringview.data(), initialToken.stringview.size());

                    if (val > std::numeric_limits<uint32_t>().max())
                    {
                        literal.type = NaiType::U64;
                        literal.size = 8;

                        literal.u64 = val;
                    }
                    else
                    {
                        literal.type = NaiType::U32;
                        literal.size = 4;

                        literal.u32 = static_cast<uint32_t>(val);
                    }

                    break;
                }

                case Token::Type::NUMERIC_FLOAT:
                {
                    char* endPtr;
                    double val = strtod(initialToken.stringview.data(), &endPtr);

                    if (val > std::numeric_limits<float>().max())
                    {
                        literal.type = NaiType::F64;
                        literal.size = 8;

                        literal.f64 = val;
                    }
                    else
                    {
                        literal.type = NaiType::F32;
                        literal.size = 4;

                        literal.f32 = static_cast<float>(val);
                    }
                    break;
                }
                case Token::Type::NUMERIC_DOUBLE:
                {
                    char* endPtr;
                    double val = strtod(initialToken.stringview.data(), &endPtr);

                    literal.type = NaiType::F64;
                    literal.size = 8;

                    literal.f64 = val;
                    break;
                }

                case Token::Type::NUMERIC_HEX:
                {
                    char* endPtr;
                    uint64_t val = strtoull(initialToken.stringview.data(), &endPtr, 16);

                    if (val > std::numeric_limits<uint32_t>().max())
                    {
                        literal.type = NaiType::U64;
                        literal.size = 8;

                        literal.u64 = val;
                    }
                    else
                    {
                        literal.type = NaiType::U32;
                        literal.size = 4;

                        literal.u32 = static_cast<uint32_t>(val);
                    }
                    break;
                }

                case Token::Type::STRING:
                {
                    literal.type = NaiType::STRING;
                    literal.size = static_cast<int>(initialToken.stringview.size());

                    literal.string = new char[literal.size + 1];
                    memcpy(literal.string, initialToken.stringview.data(), literal.size);
                    literal.string[literal.size] = 0;
                    break;
                }

                default:
                    break;
            }

            if (expression.left.node == nullptr)
            {
                expression.lType = AstExpression::Type::VALUE;
                expression.left.val = compileUnit.CreateNode<AstValue>(&initialToken);
                expression.left.val->type = AstValue::Type::LITERAL;
                expression.left.val->literal = literal;
            }
            else
            {
                expression.rType = AstExpression::Type::VALUE;
                expression.right.val = compileUnit.CreateNode<AstValue>(&initialToken);
                expression.right.val->type = AstValue::Type::LITERAL;
                expression.right.val->literal = literal;
            }

            compileUnit.Eat(tokenIndex); // LITERAL
        }
    }

    Token& currentToken = compileUnit.Peek(tokenIndex);

    AstExpression::OperatorType opType = AstExpression::GetOperatorTypeFromTokenType(currentToken.type);
    if (opType == AstExpression::OperatorType::NONE)
        return true;

    compileUnit.Eat(tokenIndex); // OPERATOR

    // Handle Expression Sequence
    if (expression.op == AstExpression::OperatorType::NONE)
    {
        expression.op = opType;

        if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, expression))
            return false;
    }
    else
    {
        AstExpression* newExpression = compileUnit.CreateNode<AstExpression>();
        *newExpression = expression;

        expression.token = &currentToken;
        expression.op = opType;
        expression.lType = AstExpression::Type::EXPRESSION;
        expression.rType = AstExpression::Type::NONE;

        expression.left.node = newExpression;
        expression.right.node = nullptr;

        if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, expression))
            return false;
    }

    return true;
}

bool Parser::GetDataTypeFromTokenIndex(CompileUnit& compileUnit, int& tokenIndex, AstDataType& dataType)
{
    // Get Data Type from Token
    dataType.token = &compileUnit.Peek(tokenIndex);
    if (dataType.token->type != Token::Type::IDENTIFIER)
        return false;

    compileUnit.Eat(tokenIndex); // DATATYPE

    if (!CompilerInfo::GetTypeInfo(compileUnit.moduleNameHash, dataType.token->hash, dataType.type))
        return false;

    Token& pointerToken = compileUnit.Peek(tokenIndex);
    if (pointerToken.type == Token::Type::ASTERISK)
    {
        compileUnit.Eat(tokenIndex); // POINTER
        dataType.isPointer = true;
    }
    else
    {
        dataType.isPointer = false;
    }

    return true;
}

bool Parser::GetVariableFromTokenIndex(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex, AstVariable*& variable)
{
    Token& identifier = compileUnit.Peek(tokenIndex);
    compileUnit.Eat(tokenIndex); // IDENTIFIER

    uint32_t parentIndex = std::numeric_limits<uint32_t>().max();
    size_t varIndex = compileUnit.astFunction->variables.size();

    for (uint32_t i = 0; i < varIndex; i++)
    {
        AstVariable& variable = compileUnit.astFunction->variables[i];
        if (identifier.hash == variable.token->hash)
        {
            parentIndex = i;
            break;
        }
    }

    // Undeclared Variable being used
    if (parentIndex == std::numeric_limits<uint32_t>().max())
        return false;

    variable = &compileUnit.astFunction->variables.emplace_back(&identifier);
    variable->index = varIndex;
    variable->parentIndex = parentIndex;

    AstVariable& parent = compileUnit.astFunction->variables[parentIndex];
    variable->dataType = parent.dataType;
    variable->isConst = parent.isConst;

    Token& nextToken = compileUnit.Peek(tokenIndex);
    if (nextToken.type == Token::Type::ASSIGN ||
        nextToken.type == Token::Type::PLUS_EQUALS ||
        nextToken.type == Token::Type::MINUS_EQUALS ||
        nextToken.type == Token::Type::MULTIPLY_EQUALS ||
        nextToken.type == Token::Type::DIVIDE_EQUALS ||
        nextToken.type == Token::Type::MODULUS_EQUALS ||
        nextToken.type == Token::Type::POW_EQUALS ||
        nextToken.type == Token::Type::BITSHIFT_LEFT_EQUALS ||
        nextToken.type == Token::Type::BITSHIFT_RIGHT_EQUALS ||
        nextToken.type == Token::Type::BITWISE_AND_EQUALS ||
        nextToken.type == Token::Type::BITWISE_OR_EQUALS)
    {
        compileUnit.Eat(tokenIndex); // ASSIGN / *_EQUALS

        variable->expression = compileUnit.CreateNode<AstExpression>(&nextToken);

        if (nextToken.type != Token::Type::ASSIGN)
        {
            variable->expression->op = AstExpression::GetOperatorTypeFromTokenType(nextToken.type);

            variable->expression->lType = AstExpression::Type::VALUE;
            variable->expression->left.val = compileUnit.CreateNode<AstValue>(variable->token);
            variable->expression->left.val->type = AstValue::Type::VARIABLE;
            variable->expression->left.val->variableIndex = variable->index;

            variable->expression->rType = AstExpression::Type::EXPRESSION;
            variable->expression->right.node = compileUnit.CreateNode<AstExpression>(&nextToken);

            if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, *variable->expression->right.node))
                return false;
        }
        else
        {
            if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, *variable->expression))
                return false;
        }
    }

    return true;
}

bool Parser::GetVariableDeclarationFromTokenIndex(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex, AstVariable*& variable)
{
    Token& identifier = compileUnit.Peek(tokenIndex);
    compileUnit.Eat(tokenIndex); // IDENTIFIER

    size_t numVariables = compileUnit.astFunction->variables.size();
    for (uint32_t i = 0; i < numVariables; i++)
    {
        AstVariable& variable = compileUnit.astFunction->variables[i];
        if (identifier.hash == variable.token->hash)
        {
            // Redeclaration of Variable
            return false;
        }
    }

    Token& declaration = compileUnit.Peek(tokenIndex);
    compileUnit.Eat(tokenIndex); // DECLARATION

    variable = &compileUnit.astFunction->variables.emplace_back(&identifier);
    variable->index = compileUnit.astFunction->variables.size() - 1;
    variable->isConst = declaration.type == Token::Type::DECLARATION_CONST;

    if (!GetDataTypeFromTokenIndex(compileUnit, tokenIndex, variable->dataType))
        return false;

    Token& nextToken = compileUnit.Peek(tokenIndex);
    if (nextToken.type == Token::Type::ASSIGN ||
        nextToken.type == Token::Type::PLUS_EQUALS ||
        nextToken.type == Token::Type::MINUS_EQUALS ||
        nextToken.type == Token::Type::MULTIPLY_EQUALS ||
        nextToken.type == Token::Type::DIVIDE_EQUALS ||
        nextToken.type == Token::Type::MODULUS_EQUALS ||
        nextToken.type == Token::Type::POW_EQUALS ||
        nextToken.type == Token::Type::BITSHIFT_LEFT_EQUALS ||
        nextToken.type == Token::Type::BITSHIFT_RIGHT_EQUALS ||
        nextToken.type == Token::Type::BITWISE_AND_EQUALS ||
        nextToken.type == Token::Type::BITWISE_OR_EQUALS)
    {
        compileUnit.Eat(tokenIndex); // ASSIGN / *_EQUALS

        variable->expression = compileUnit.CreateNode<AstExpression>(&nextToken);

        if (nextToken.type != Token::Type::ASSIGN)
        {
            variable->expression->op = AstExpression::GetOperatorTypeFromTokenType(nextToken.type);

            variable->expression->lType = AstExpression::Type::VALUE;
            variable->expression->left.val = compileUnit.CreateNode<AstValue>(variable->token);
            variable->expression->left.val->type = AstValue::Type::VARIABLE;
            variable->expression->left.val->variableIndex = variable->index;

            variable->expression->rType = AstExpression::Type::EXPRESSION;
            variable->expression->right.node = compileUnit.CreateNode<AstExpression>(&nextToken);

            if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, *variable->expression->right.node))
                return false;
        }
        else
        {
            if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, *variable->expression))
                return false;
        }
    }

    return true;
}

bool Parser::GetReturnFromTokenIndex(CompileUnit& compileUnit, const int& numTokens, int& tokenIndex, AstReturn*& ret)
{
    ret = compileUnit.CreateNode<AstReturn>(&compileUnit.Peek(tokenIndex));

    compileUnit.Eat(tokenIndex); // RETURN
    Token& firstToken = compileUnit.Peek(tokenIndex);

    size_t numReturnValues = compileUnit.astFunction->returnTypeNodes.size();
    if (firstToken.type == Token::Type::END_OF_LINE)
    {
        if (numReturnValues > 0)
            return false;
    }
    else
    {
        if (firstToken.type == Token::Type::LBRACKET)
        {
            compileUnit.Eat(tokenIndex); // LBRACKET

            if (numReturnValues <= 1)
                return false;
            
            // Handle Multiple Return Statements
            for (; tokenIndex < numTokens;)
            {
                Token& currentToken = compileUnit.Peek(tokenIndex);
                if (currentToken.type == Token::Type::END_OF_LINE)
                    break;

                AstReturnSet* retSet = compileUnit.CreateNode<AstReturnSet>();
                retSet->expression = compileUnit.CreateNode<AstExpression>(&firstToken);

                if (GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, *retSet->expression))
                    return false;

                // TODO: Get Data Type from Expression

                ret->returnSets.push_back(retSet);
            }

            if (numReturnValues != ret->returnSets.size())
                return false;
        }
        else
        {
            AstReturnSet* retSet = compileUnit.CreateNode<AstReturnSet>();
            retSet->expression = compileUnit.CreateNode<AstExpression>(&firstToken);

            if (!GetExpressionFromTokenIndex(compileUnit, numTokens, tokenIndex, *retSet->expression))
                return false;

            // TODO: Get Data Type from Expression

            ret->returnSets.push_back(retSet);
        }
    }

    compileUnit.Eat(tokenIndex); // END_OF_LINE
    return true;
}
