#include <pch/Build.h>
#include <iostream>

int main()
{
#ifdef NAI_CLANG
    std::cout << "Hello Clang!";
#else
    std::cout << "Hello World!";
#endif
    return 0;
}