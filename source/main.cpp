#include <pch/Build.h>
#include <iostream>

//#include "Lexer/Lexer.h"

int main()
{
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
               /* Lexer lexer;
                lexer.Init(buffer, result);
                lexer.Process();*/
            }
        }

        fclose(file);
    }

    return 0;
}