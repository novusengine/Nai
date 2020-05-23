#pragma once
#include <string>

class UnitTester
{
public:
    int UnitTest(const std::string& fileName, const std::string& outputPath);

private:
    void CreateResultFile(bool succeeded, const std::string& outputPath, const std::string& testResult, const std::string& expectedResult);

private:
};