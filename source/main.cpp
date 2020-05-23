#include <pch/Build.h>
#include <iostream>
#include <chrono>

#include "Lexer/Lexer.h"
#include "Parser/Parser.h"

int main()
{
    Lexer lexer;
    lexer.Init();

    Parser parser;
    parser.Init();

    FILE* file = nullptr;
    fopen_s(&file, "text.nai", "r");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        unsigned long size = ftell(file);
        rewind(file);

        char* buffer = new char[sizeof(char) * size];
        if (buffer)
        {
            size_t result = fread(buffer, 1, size, file);
            if (result)
            {
                std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

                LexerFile lexerFile(buffer, static_cast<long>(result));
                lexer.Process(lexerFile);

                ParserFile parserFile(lexerFile);
                parser.Process(parserFile);

                std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
                std::cout << "Lexer/Parsing took " << time_span.count() << " seconds.";
                std::cout << std::endl;

                std::string code = Lexer::UnitTest_TokensToCode(lexerFile.GetTokens());
            }
        }

        fclose(file);
    }

    //auto tokens = Lexer::UnitTest_CodeToTokens("[identifier (int)] [identifier (myInt)][operator (=)] [literal (5)] [seperator(;)]");
    return 0;
}