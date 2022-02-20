#pragma once

#include <iostream>
#include <exception>
#include <string>
#include <map>
#include <set>
#include <regex>

using Path = std::string;
using Environment = std::map<std::string, std::string>;

struct Settings
{
    /*! TYPES */

    enum class Mode
    {
        Install,
        Check,
        List
    };

    struct Platform
    {
        enum class OperatingSystem
        {
            Windows,
            Mac,
            Linux
        };

        enum class Architecture
        {
            x86,
            x64
        };

        enum class Configuration
        {
            Debug,
            Release
        };

        OperatingSystem os;
        Architecture arch;
        Configuration config;
    };

    /*! DATA */

    static const std::map<std::string, Mode> StringToModeMapping;
    static const std::map<std::string, Platform> StringToPlatformMapping;

    Mode mode;
    Path target;
    Path output;
    Platform platform;
    Environment variables;
    bool verbose = false;

    /*! FUNCTIONS */

    void ParseCommandLine(const int argc, const char* const argv[])
    {
        if (argc < 4)
            throw std::exception("insufficient number of parameters");

        int index = 1;

        //! Execution mode specifier
        this->mode = ReadSpecialParam<Mode>(argv, index++, "execution-mode", Settings::StringToModeMapping);

        if (mode == Mode::Install && argc < 5)
            throw std::exception("insufficient number of parameters");

        //! Path to target
        this->target = ReadPathParam(argv, index++, "target");

        //! Output directory
        if (mode == Mode::Install)
            this->output = ReadPathParam(argv, index++, "install-dir");

        //! Platform specifier
        this->platform = ReadSpecialParam<Platform>(argv, index++, "platform", Settings::StringToPlatformMapping);

        //! Additional options
        std::set<std::string> parsed_options;
        while (index < argc)
        {
            std::string option = ReadOptionSpecifier(argv, index++);

            //! Verbose flag
            if (option == "-v" || option == "--verbose")
            {
                static const std::string VerboseOptionName = "verbose";

                if (parsed_options.contains(VerboseOptionName))
                    throw std::exception(("option '" + VerboseOptionName + "' has been specified multiple times").c_str());
                parsed_options.insert(VerboseOptionName);

                this->verbose = true;
                continue;
            }

            if (index == argc)
                throw std::exception(("option '" + option + "' is not followed by any value").c_str());

            //! Environment variables
            if (option == "-e" || option == "--env")
            {
                static const std::string EnvOptionName = "env";

                if (parsed_options.contains(EnvOptionName))
                    throw std::exception(("option '" + EnvOptionName + "' has been specified multiple times").c_str());
                parsed_options.insert(EnvOptionName);

                while (index < argc)
                {
                    const std::string variable_str = ReadStringParam(argv, index, "variable");
                    if (variable_str.starts_with('-') || variable_str.starts_with("--"))
                        break;  // reached the end of environment variable definitions

                    const std::pair<std::string, std::string> variable_pair = ReadEnvironmentVariable(argv, index++);

                    if (this->variables.contains(variable_pair.first))
                        throw std::exception(("variable '" + variable_pair.first + "' has been already defined").c_str());

                    this->variables.insert(variable_pair);
                }
            }
            else
            {
                // TODO: replace with call to logger
                std::cerr << "WARNING: "
                    << '\'' << option << "\' is not a supported option specifier and will be skipped"
                    << std::endl;
            }
        }
    }

private:

    inline static std::string ReadStringParam(const char* const argv[], const int index, const std::string& name)
    {
        return std::string(argv[index]);
    }

    inline static std::string ReadPathParam(const char* const argv[], const int index, const std::string& name)
    {
        std::string value_str = ReadStringParam(argv, index, name);

        // TODO: validate path and throw exception if not valid

        return value_str;
    }

    inline static uint32_t ReadUInt32Param(const char* const argv[], const int index, const std::string& name)
    {
        const std::string value_str = std::string(argv[index]);
        try
        {
            int value_int = std::stoi(value_str);
            if (value_int <= 0)
                throw std::exception();

            return uint32_t(value_int);
        }
        catch (std::exception e)
        {
            const std::string error = "\'" + value_str + "\' is not a valid value for \'" + name + '\'';
            throw std::exception(error.c_str());
        }
    }

    template<typename T>
    inline static T ReadSpecialParam(const char* const argv[], const int index, const std::string& name, const std::map<std::string, T> values)
    {
        const std::string value_str = ReadStringParam(argv, index, name);

        const auto it = values.find(value_str);
        if (it != values.end())
            return it->second;

        const std::string error = '\'' + value_str + "\' is not a valid value for \'" + name + '\'';
        throw std::exception(error.c_str());
    }

    inline static std::string ReadOptionSpecifier(const char* const argv[], const int index)
    {
        const std::string option = std::string(argv[index]);

        if (!(option.starts_with("--") || option.starts_with('-')))
            throw std::exception("option specifiers must begin with '-' or '--' (e.g. --verbose)");

        return option;
    }

    inline static std::pair<std::string, std::string> ReadEnvironmentVariable(const char* const argv[], const int index)
    {
        // Variable definitions must be in the form <NAME>=<VALUE> where:
        //   NAME is a string of one or more alpha-numerical characters (or '_')
        //   VALUE is a string of one or more characters, which can either be alpha-numerical
        //     or '_', '~', ':', '.', '\', '/', single-quote or double-quotes
        static const std::regex variable_regex("^[a-zA-Z1-9_]+=[a-zA-Z1-9_~:.\\/\"']+$");

        const std::string variable_str = std::string(argv[index]);

        // Check correctness of the variable definition
        if (variable_str.find('$') != std::string::npos)
            throw std::exception("env variable definitions must not contain the '$' character");
        if (!std::regex_match(variable_str, variable_regex))
            throw std::exception(("env variable definition '" + variable_str + "' is not in a valid format").c_str());

        // Parse NAME=VALUE into std::pair and return
        const size_t splitpos = variable_str.find('=');
        const std::string variable_name = variable_str.substr(0, splitpos);
        const std::string variable_value = variable_str.substr(splitpos + 1, variable_str.length());
        return { variable_name, variable_value };
    }
};

const std::map<std::string, Settings::Mode>
Settings::StringToModeMapping
{
    { "--install", Mode::Install },
    { "--check",   Mode::Check },
    { "--list",    Mode::List },

    { "-i", Mode::Install },
    { "-c", Mode::Check },
    { "-l", Mode::List },
};

const std::map<std::string, Settings::Platform>
Settings::StringToPlatformMapping
{
    { "win32",     { Platform::OperatingSystem::Windows, Platform::Architecture::x86, Platform::Configuration::Release } },
    { "win32d",    { Platform::OperatingSystem::Windows, Platform::Architecture::x86, Platform::Configuration::Debug } },
    { "win64",     { Platform::OperatingSystem::Windows, Platform::Architecture::x64, Platform::Configuration::Release } },
    { "win64d",    { Platform::OperatingSystem::Windows, Platform::Architecture::x64, Platform::Configuration::Debug } },

    { "macosx32",  { Platform::OperatingSystem::Mac, Platform::Architecture::x86, Platform::Configuration::Release } },
    { "macosx32d", { Platform::OperatingSystem::Mac, Platform::Architecture::x86, Platform::Configuration::Debug } },
    { "macosx64",  { Platform::OperatingSystem::Mac, Platform::Architecture::x64, Platform::Configuration::Release } },
    { "macosx64d", { Platform::OperatingSystem::Mac, Platform::Architecture::x64, Platform::Configuration::Debug } },

    { "linux32",   { Platform::OperatingSystem::Linux, Platform::Architecture::x86, Platform::Configuration::Release } },
    { "linux32d",  { Platform::OperatingSystem::Linux, Platform::Architecture::x86, Platform::Configuration::Debug } },
    { "linux64",   { Platform::OperatingSystem::Linux, Platform::Architecture::x64, Platform::Configuration::Release } },
    { "linux64d",  { Platform::OperatingSystem::Linux, Platform::Architecture::x64, Platform::Configuration::Debug } }
};
