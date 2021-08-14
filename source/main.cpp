#include <pch/Build.h>
#include <iostream>
#include <chrono>

#include "Compiler/Compiler.h"
#include "Utils/CLIParser.h"
#include "Utils/FileReader.h"

#ifdef _WIN32
#include <Windows.h>
#include <WinBase.h>
#endif

#ifdef TRACY_ENABLE
void* operator new(std::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
void operator delete(void* ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
#endif

void TestCallback(Interpreter* interpreter)
{
    static i32 i = 0;
    *interpreter->GetRegister(Register::Rax) = ++i;
}

void AddStringToResult(std::string& string, const char* buffer, u32 offset, u32 count)
{
    if (count == 0)
        return;

    string += std::string(buffer, offset, count);
}

void PrintWithArgs(Interpreter* interpreter, const char* buffer, u32 length)
{
    std::string result;
    result.reserve(length);

    u32 lastFormatSign = 0;
    u8 paramCounter = 2;

    for (u32 i = 0; i < length;)
    {
        char c = buffer[i];
        char cc = buffer[i + 1];

        u32 delta = i - lastFormatSign;

        if (c == '%' && cc == 'u')
        {
            AddStringToResult(result, buffer, lastFormatSign, delta);

            u32* num = interpreter->GetParameter<u32>(paramCounter++);
            result += std::to_string(*num);

            i += 2;
            lastFormatSign = i;
        }
        if (c == '%' && cc == 'l')
        {
            AddStringToResult(result, buffer, lastFormatSign, delta);

            u64* num = interpreter->GetParameter<u64>(paramCounter++);
            result += std::to_string(*num);

            i += 2;
            lastFormatSign = i;
        }
        else if (c == '%' && cc == 's')
        {
            AddStringToResult(result, buffer, lastFormatSign, delta);

            char* str = interpreter->GetParameter<char>(paramCounter++, true);
            result += std::string(str);

            i += 2;
            lastFormatSign = i;
        }
        else
        {
            i++;
        }
    }

    // Append remaining string
    if (lastFormatSign < length)
    {
        u32 delta = length - lastFormatSign;
        AddStringToResult(result, buffer, lastFormatSign, delta);
    }

    DebugHandler::Print(result);
}

void PrintCallback(Interpreter* interpreter)
{
    char* format = interpreter->GetParameter<char>(1, true);
    PrintWithArgs(interpreter, format, static_cast<u32>(strlen(format)));
}

void AddCallback(Interpreter* interpreter)
{
    u32* num1 = interpreter->GetParameter<u32>(1);
    u32* num2 = interpreter->GetParameter<u32>(2);
    u32 result = *num1 + *num2;

    *interpreter->GetRegister(Register::Rax) = result;
}

int Compile(const std::string& fileName)
{
    ZoneScopedNC("Compile", tracy::Color::Red);

    FileReader reader(fileName, fileName);
    if (!reader.Fetch())
    {
        DebugHandler::PrintError("Compiler failed to read script (%s)");
    }
    else
    {
        Compiler cc;

        Module* module = cc.modules.Emplace();
        module->nameHash.SetNameHash(fileName);
        module->lexerInfo.buffer = reader.GetBuffer();
        module->lexerInfo.size = reader.Length();

        Lexer::Process(module);
        Parser::Process(module);

        // Add Native Calls

        //NativeFunction nfTest(module, "Test", TestCallback);
        NativeFunction nfPrint(module, "print", PrintCallback);
        {
            nfPrint.AddParamChar("string", NativeFunction::PassAs::Pointer);
        }

        NativeFunction nfAdd(module, "Add", AddCallback);
        {
            nfAdd.AddParamU32("num1", NativeFunction::PassAs::Value);
            nfAdd.AddParamU32("num2", NativeFunction::PassAs::Value);
            nfAdd.SetReturnTypeU32(NativeFunction::PassAs::Value);
        }

        Typer::Process(module);
        Bytecode::Process(module);

        Interpreter* interpreter = new Interpreter();

        u32 mainHash = "main"_djb2;
        bool foundMain = false;

        ListNode* node;
        ListIterate(&module->parserInfo.block->scope->declarations, node)
        {
            Declaration* declaration = ListGetStructPtr(node, Declaration, listNode);

            if (declaration->kind != Declaration::Kind::Function)
                continue;

            if (mainHash != declaration->token->nameHash.hash)
                continue;

            // Ensure Main is of type Void
            if (declaration->function.returnType->kind != Type::Kind::Void)
                continue;

            bool meetRequirements = true;

            // Ensure Main has no parameters
            {
                ListNode* declNode;
                ListIterate(&declaration->function.scope->declarations, declNode)
                {
                    Declaration* scopeDeclaration = ListGetStructPtr(declNode, Declaration, listNode);
                    if (scopeDeclaration->kind == Declaration::Kind::Variable)
                    {
                        meetRequirements = false;
                        break;
                    }
                }
            }

            if (!meetRequirements)
                break;

            foundMain = true;
            // Record Time
            {
                std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
                // Run Main
                {
                    interpreter->Init();
                    interpreter->Interpret(module, declaration);
                }
                std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

                // Print Time
                DebugHandler::PrintSuccess("Interpreter : Finished in %f seconds", time_span.count());
            }
            break;
        }

        if (!foundMain)
        {
            DebugHandler::PrintError("Compiler : Failed to find Entry Point ('main()') in Module(%s)", module->nameHash.name.c_str());
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    ZoneScoped;

#ifdef _WIN32
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif

    CLIParser cliParser; // Default implicit parameters are "executable" which gets the path to the current executable, and "filename" which gets the file we're acting on

    cliParser.AddParameter("unittest", "Runs a unittest on the file")
             .AddParameter("testoutput", "The output location for unittests, [REQUIRED] if doing unittest");

    CLIValues values = cliParser.ParseArguments(argc, argv);

    if (!values["filename"_h].WasDefined())
    {
        cliParser.PrintHelp();
        return -1;
    }

    std::string filename = values["filename"_h].As<std::string>();
    return Compile(filename);
}