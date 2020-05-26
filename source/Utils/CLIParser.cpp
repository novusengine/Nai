#include <pch/Build.h>
#include "CLIParser.h"

CLIParser::CLIParser()
{
    ZoneScoped;
}

CLIValues CLIParser::ParseArguments(int argc, char* argv[])
{
    ZoneScoped;

    CLIValues cliValues;

    // First we initialize the defaults
    for (const CLIParameter& parameter : _requiredParameters)
    {
        if (parameter.hasDefaultValue)
        {
            CLIValue value;
            value._defined = true;
            value._defaultValue = parameter.defaultValue;

            unsigned int hashedArgumentName = StringUtils::fnv1a_32(parameter.name.c_str(), parameter.name.size());
            cliValues[hashedArgumentName] = value;
        }
    }

    // Then we parse and override from the arguments
    for (int i = 0; i < argc; i++)
    {
        std::string arg(argv[i]);

        std::string argumentName;
        std::string argumentValue = "";

        // Is this argument trying to set a value?
        if (arg.find('=') != std::string::npos)
        {
            std::vector<std::string> splitArgs = StringUtils::Split(arg, '=');

            if (splitArgs.size() != 2)
            {
                std::cout <<  "CLI Parameter Error: Unexpected number of '=' splits in argument" << argumentName << std::endl << std::flush;
                assert(false);
            }
            
            argumentName = splitArgs[0];
            argumentValue = splitArgs[1];
        }
        else // Else we can just set it
        {
            argumentName = arg;
        }

        // Now lets find the matching CLIParameter
        const CLIParameter* param = nullptr;

        // First we check the required parameters
        for (const CLIParameter& parameter : _requiredParameters)
        {
            if (parameter.name == argumentName)
            {
                param = &parameter;
                break;
            }
        }

        if (param == nullptr)
        {
            // If it wasn't found we check the optional parameters
            for (const CLIParameter& parameter : _optionalParameters)
            {
                if (parameter.name == argumentName)
                {
                    param = &parameter;
                    break;
                }
            }
        }
        
        // If we didn't find it
        if (param == nullptr)
        {
            // If we are at the first argument, it's the path to the executable which gets implicitly passed
            if (i == 0)
            {
                argumentValue = argumentName;
                argumentName = "executable";
            }
            // If we are the last argument, it's the path to the file to act on
            else if (i == argc - 1 && i != 0)
            {
                // Add this as an implicit "filename" value
                argumentValue = argumentName;
                argumentName = "filename";
            }
            else
            {
                // error out since someone tried to use an unknown parameter
                std::cout << "CLI Parameter Error : Tried to set unknown option " << argumentName << std::endl << std::flush;
                assert(false);
            }
        }
        
        CLIValue value;
        value._defined = true;
        value._value = argumentValue;

        unsigned int hashedArgumentName = StringUtils::fnv1a_32(argumentName.c_str(), argumentName.size());
        cliValues[hashedArgumentName] = value;
    }

    // Make sure that all required parameters have been defined
    bool missingParameter = false;
    for (const CLIParameter& parameter : _requiredParameters)
    {
        unsigned int hashedParameterName = StringUtils::fnv1a_32(parameter.name.c_str(), parameter.name.size());
        if (!cliValues[hashedParameterName].WasDefined())
        {
            std::cout << "CLI Parameter Error : Missing required parameter " << parameter.name << std::endl;

            missingParameter = true;
        }
    }

    if (missingParameter)
    {
        std::cout << std::endl;
        PrintHelp();
        assert(false);
    }

    return cliValues;
}

void CLIParser::PrintHelp()
{
    ZoneScoped;

    std::cout << "Available arguments: " << std::endl << std::endl;

    for (const CLIParameter& parameter : _requiredParameters)
    {
        std::cout << parameter.name << " \t " << parameter.description << " [REQUIRED]" << std::endl;
    }

    for (const CLIParameter& parameter : _optionalParameters)
    {
        std::cout << parameter.name << " \t " << parameter.description << std::endl;
    }

    std::cout << "The last argument supplied is supposed to be a file, and it is [REQUIRED]";

    std::cout << std::flush;
}
