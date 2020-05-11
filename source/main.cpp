#include <pch/Build.h>
#include <iostream>

#include "Lexer/Lexer.h"

int main()
{
    /*FILE* file = nullptr;
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
                Lexer lexer;
                lexer.Init(buffer, static_cast<long>(result));
                lexer.Process();
            }
        }

        fclose(file);
    }*/

    auto tokens = Lexer::UnitTest("[identifier (int)] [identifier (myInt)][operator (=)] [literal (5)] [seperator(;)]");

    return 0;
}