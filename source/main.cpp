#include <pch/Build.h>
#include <iostream>
#include <chrono>

#include "Lexer/Lexer.h"
#include "Parser/Parser.h"

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
    ZoneScoped;

    Lexer lexer;
    lexer.Init();

    Parser parser;
    parser.Init();

    FILE* file = nullptr;
    fopen_s(&file, fileName.c_str(), "r");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        unsigned long size = ftell(file);
        rewind(file);

        char* buffer = new char[sizeof(char) * size];
        if (buffer)
        {
            size_t readSize = fread(buffer, 1, size, file);
            if (readSize)
            {
                std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

                LexerFile lexerFile(buffer, static_cast<long>(readSize));
                lexer.Process(lexerFile);

                std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
                std::cout << "Lexer took " << time_span.count() << " seconds.\n";

                // Time Parsing
                t1 = std::chrono::high_resolution_clock::now();

                ParserFile parserFile(lexerFile);
                parser.Process(parserFile);

                t2 = std::chrono::high_resolution_clock::now();
                time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
                std::cout << "Parsing took " << time_span.count() << " seconds.\n";

                std::cout << std::endl;

                std::string code = Lexer::UnitTest_TokensToCode(lexerFile.GetTokens());
            }
        }

        fclose(file);
    }

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