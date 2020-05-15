#include <pch/Build.h>
#include "UnitTester.h"

#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>

#include <Lexer\Lexer.h>

int UnitTester::UnitTest(const std::string& fileName, const std::string& outputPath)
{
    std::cout << "Unit test " << fileName;

    std::string testResult;

    Lexer lexer;
    lexer.Init();

    FILE* testFile = nullptr;
    fopen_s(&testFile, fileName.c_str(), "r");
    if (testFile)
    {
        fseek(testFile, 0, SEEK_END);
        unsigned long fileSize = ftell(testFile);
        rewind(testFile);

        char* buffer = new char[sizeof(char) * fileSize];
        if (buffer)
        {
            size_t readSize = fread(buffer, 1, fileSize, testFile);
            if (readSize)
            {
                LexerFile lexerFile(buffer, static_cast<long>(readSize));
                lexer.Process(lexerFile);

                testResult = Lexer::UnitTest_TokensToCode(lexerFile.GetTokens());
            }
        }
    }

    const std::string resultFileName = fileName + ".result";
    std::string expectedResult;
    testFile = nullptr;
    fopen_s(&testFile, resultFileName.c_str(), "r");
    if (testFile)
    {
        fseek(testFile, 0, SEEK_END);
        unsigned long fileSize = ftell(testFile);
        rewind(testFile);

        char* buffer = new char[sizeof(char) * fileSize];
        if (buffer)
        {
            size_t readSize = fread(buffer, 1, fileSize, testFile);
            expectedResult = std::string(buffer, readSize);
        }
    }

    if (testResult != expectedResult)
    {
        std::cout << " FAILED!" << std::endl;
        CreateResultFile(false, outputPath, testResult, expectedResult);
        return -1;
    }

    std::cout << " succeeded!" << std::endl;
    CreateResultFile(true, outputPath, testResult, expectedResult);

    return 0;
}

void UnitTester::CreateResultFile(bool succeeded, const std::string& outputPath, const std::string& testResult, const std::string& expectedResult)
{
    std::ofstream output(outputPath);
    output << "Test " << ((succeeded) ? "SUCCEEDED" : "FAILED") << std::endl << std::endl;
    output << "Lexer test result: " << std::endl;
    output << testResult << std::endl << std::endl;
    output << "Lexer Expected: " << std::endl;
    output << expectedResult;

    output.close();
}
