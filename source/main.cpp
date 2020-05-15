#include <pch/Build.h>
#include <iostream>

#include "Lexer/Lexer.h"
#include "UnitTester/UnitTester.h"
#include "Utils/CLIParser.h"

int Compile(const std::string& fileName)
{
    Lexer lexer;
    lexer.Init();

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
                LexerFile lexerFile(buffer, static_cast<long>(readSize));
                lexer.Process(lexerFile);
            }
        }

        fclose(file);
    }

    return 0;
}

int main(int argc, char* argv[])
{
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