#include <pch/Build.h>
#include <iostream>
#include <chrono>

#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Bytecode/BytecodeGenerator.h"
#include "Bytecode/BytecodeVM.h"

#include "UnitTester/UnitTester.h"
#include "Utils/CLIParser.h"


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

int Compile(const std::string& fileName)
{
    ZoneScopedNC("Compile", tracy::Color::Red);

    BytecodeVM vm(1);
    vm.RunScript(fileName);

    return 0;
}

int main(int argc, char* argv[])
{
    ZoneScoped;

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

    if (values["unittest"_h].WasDefined())
    {
        if (!values["testoutput"_h].WasDefined())
        {
            std::cout << "ERROR: If you use the unittest flag you also need testoutput to be set" << std::endl;
            cliParser.PrintHelp();
            return -1;
        }

        std::string testOutputPath = values["testoutput"_h].As<std::string>();

        UnitTester unitTester;
        return unitTester.UnitTest(filename, testOutputPath);
    }
    else
    {
        return Compile(filename);
    }
}