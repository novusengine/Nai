#pragma once
#include <stdexcept>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cassert>
#include "StringUtils.h"

class CLIValue
{
public:
    template <typename T>
    T As();

    template<>
    int As<int>()
    {
        if (_value == "UNDEFINED")
        {
            if (_defaultValue != nullptr)
            {
                return *(int*)_defaultValue;
            }

            std::cout << "Tried to get value of undefined CLIValue" << std::endl << std::flush;
            assert(false);
        }

        return std::stoi(_value);
    }

    template<>
    std::string As<std::string>()
    {
        if (_value == "UNDEFINED")
        {
            if (_defaultValue != nullptr)
            {
                return *(std::string*)_defaultValue;
            }

            std::cout << "Tried to get value of undefined CLIValue" << std::endl << std::flush;
            assert(false);
        }

        return _value;
    }

    bool WasDefined()
    {
        return _defined;
    }
    
private:
    bool _defined = false;
    std::string _value = "UNDEFINED";
    void* _defaultValue = nullptr;

    friend class CLIParser;
};

typedef std::unordered_map<unsigned int, CLIValue> CLIValues;

class CLIParser
{
public:
    CLIParser();

    CLIValues ParseArguments(int argc, char* argv[]);

    template <typename T = bool>
    CLIParser& AddParameter(const char parameterName[], const char description[])
    {
        _AddParameter(std::string(parameterName), std::string(description), false, false, T());
        return *this;
    }

    template <typename T = bool>
    CLIParser& AddParameterRequired(const char parameterName[], const char description[])
    {
        _AddParameter(std::string(parameterName), std::string(description), true, false, T());
        return *this;
    }

    template <typename T = bool>
    CLIParser& AddParameterDefault(const char parameterName[], const char description[], T defaultValue)
    {
        _AddParameter(std::string(parameterName), std::string(description), false, true, defaultValue);
        return *this;
    }

    void PrintHelp();

private:
    template <typename T>
    void _AddParameter(const std::string& parameterName, const std::string& description, const bool required, const bool hasDefaultValue, T defaultValue)
    {
        for (const CLIParameter& parameter : _requiredParameters)
        {
            if (parameter.name == parameterName)
            {
                throw std::runtime_error("Parameter '" + parameterName + "' was added twice!");
            }
        }
        for (const CLIParameter& parameter : _optionalParameters)
        {
            if (parameter.name == parameterName)
            {
                throw std::runtime_error("Parameter '" + parameterName + "' was added twice!");
            }
        }

        CLIParameter newParameter(parameterName, description, required, hasDefaultValue);
        newParameter.value = new T();
        if (hasDefaultValue)
        {
            newParameter.defaultValue = new T(defaultValue);
        }

        if (required)
        {
            _requiredParameters.push_back(newParameter);
        }
        else
        {
            _optionalParameters.push_back(newParameter);
        }
    }

    struct CLIParameter
    {
        CLIParameter(const std::string& name, const std::string& description, const bool required, const bool hasDefaultValue)
            : name(name)
            , description(description)
            , required(required)
            , hasDefaultValue(hasDefaultValue)
        {

        }

        const std::string name;
        const std::string description;
        const bool required;
        const bool hasDefaultValue;
        void* defaultValue = nullptr;
        void* value = nullptr;
    };

private:
    std::vector<CLIParameter> _requiredParameters;
    std::vector<CLIParameter> _optionalParameters;
};