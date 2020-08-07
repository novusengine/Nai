#pragma once
#include <iostream>
#include <fstream>

class FileReader
{
public:
    FileReader(std::string path, std::string fileName) : _path(path), _fileName(fileName) 
    {
        ZoneScoped;
    }

    bool Fetch()
    {
        ZoneScoped;
        fopen_s(&_fileStream, _path.c_str(), "r");

        fseek(_fileStream, 0, SEEK_END);
        unsigned long size = ftell(_fileStream);
        rewind(_fileStream);

        _buffer = new char[sizeof(char) * size];
        if (!_buffer)
            return false;

        _length = fread(_buffer, 1, size, _fileStream);;

        fclose(_fileStream);
        return true;
    }

    std::string Path() { return _path; }
    std::string FileName() { return _fileName; }

    char* GetBuffer() { return _buffer; }
    size_t Length() { return _length; }

private:
    FILE* _fileStream;

    std::string _path;
    std::string _fileName;

    char* _buffer = nullptr;;
    size_t _length = 0;
};