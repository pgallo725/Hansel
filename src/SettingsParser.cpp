#include "SettingsParser.h"
#include "Logger.h"
#include "Utilities.h"

#include <set>


namespace Hansel
{
    const std::map<std::string, Settings::Mode>
        SettingsParser::StringToModeMapping
    {
        { "--help",    Settings::Mode::Help },
        { "--install", Settings::Mode::Install },
        { "--debug",   Settings::Mode::Debug },
        { "--check",   Settings::Mode::Check },
        { "--list",    Settings::Mode::List },

        { "-h", Settings::Mode::Help },
        { "-i", Settings::Mode::Install },
        { "-d", Settings::Mode::Debug },
        { "-c", Settings::Mode::Check },
        { "-l", Settings::Mode::List },
    };

    const std::map<std::string, Platform>
        SettingsParser::StringToPlatformMapping
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


    Settings SettingsParser::ParseCommandLine(const int argc, const char* const argv[])
    {
        Settings settings;

        if (argc < 2)
            throw std::exception("Insufficient number of parameters");

        int index = 1;

        //! Execution mode specifier
        settings.mode = ReadSpecialParam<Settings::Mode>(argv, index++, "execution-mode", SettingsParser::StringToModeMapping);

        if (settings.mode == Settings::Mode::Help)
        {
            if (argc > 2)
                Logger::Warn("Additional parameters after -h / --help will be ignored");
            return settings;
        }

        if (settings.mode != Settings::Mode::Help && argc < 4)
            throw std::exception("Insufficient number of parameters");

        if ((settings.mode == Settings::Mode::Install ||
             settings.mode == Settings::Mode::Debug   ||
             settings.mode == Settings::Mode::Check)  && argc < 5)
            throw std::exception("Insufficient number of parameters");

        //! Path to target
        settings.target = ReadPathParam(argv, index++, "target");

        //! Output directory
        if (settings.mode == Settings::Mode::Install ||
            settings.mode == Settings::Mode::Debug   ||
            settings.mode == Settings::Mode::Check)
            settings.output = ReadPathParam(argv, index++, "install-dir");

        //! Platform specifier
        settings.platform = ReadSpecialParam<Platform>(argv, index++, "platform", SettingsParser::StringToPlatformMapping);

        //! Additional options
        std::set<std::string> parsed_options;
        while (index < argc)
        {
            const std::string option_str = ReadOptionSpecifier(argv, index++);

            //! Verbose flag
            if (option_str == "-v" || option_str == "--verbose")
            {
                static const std::string VerboseOptionName = "verbose";

                if (parsed_options.contains(VerboseOptionName))
                    throw std::exception(("Option '" + VerboseOptionName + "' has been specified multiple times").c_str());
                parsed_options.insert(VerboseOptionName);

                settings.verbose = true;
                Logger::SetVerbose(true);
                continue;
            }

            if (index == argc)
                throw std::exception(("Option '" + option_str + "' is not followed by any value").c_str());

            //! Environment variables
            if (option_str == "-e" || option_str == "--env")
            {
                static const std::string EnvOptionName = "env";

                if (parsed_options.contains(EnvOptionName))
                    throw std::exception(("Option '" + EnvOptionName + "' has been specified multiple times").c_str());
                parsed_options.insert(EnvOptionName);

                while (index < argc)
                {
                    const std::string variable_str = ReadStringParam(argv, index, "variable");
                    if (variable_str.starts_with('-') || variable_str.starts_with("--"))
                        break;  // reached the end of environment variable definitions

                    const std::pair<std::string, std::string> variable_pair = ReadEnvironmentVariable(argv, index++);

                    if (settings.variables.contains(variable_pair.first))
                        throw std::exception(("Variable '" + variable_pair.first + "' has been already defined").c_str());

                    settings.variables.insert(variable_pair);
                }

                // Define special OUTPUT_DIR and PLATFORM_DIR environment variables
                if (settings.variables.contains("OUTPUT_DIR"))
                    Logger::Warn("OUTPUT_DIR is a reserved variable, the provided value will be replaced by '{}'",
                        settings.output);
                if (settings.variables.contains("PLATFORM_DIR"))
                    Logger::Warn("PLATFORM_DIR is a reserved variable, the provided value will be replaced by '{}'",
                        settings.platform.ToString());
                // Automatically append the platform identifier (e.g. win64d) to the specified output path
                settings.variables["OUTPUT_DIR"] = Utilities::CombinePath(settings.output, settings.platform.ToString());
                settings.variables["PLATFORM_DIR"] = settings.platform.ToString();
            }
            else
            {
                Logger::Warn("'{}' is not a supported option specifier and will be skipped", option_str);
            }
        }

        // Print a summary of the execution settings in verbose mode
        if (settings.verbose)
            PrintSettings(settings);

        return settings;
    }


    std::string SettingsParser::ReadStringParam(const char* const argv[], const int index, const std::string& name)
    {
        return std::string(argv[index]);
    }

    std::string SettingsParser::ReadPathParam(const char* const argv[], const int index, const std::string& name)
    {
        const std::string value_str = ReadStringParam(argv, index, name);

        try // validate path and throw exception if not valid
        {
            const std::filesystem::path value_path(value_str);
            return std::filesystem::absolute(value_path.lexically_normal()).string();
        }
        catch (std::exception)
        {
            const std::string error = "\'" + value_str + "\' is not a valid \'" + name + "\' path";
            throw std::exception(error.c_str());
        }
    }

    uint32_t SettingsParser::ReadUInt32Param(const char* const argv[], const int index, const std::string& name)
    {
        const std::string value_str = std::string(argv[index]);
        try
        {
            int value_int = std::stoi(value_str);
            if (value_int <= 0)
                throw std::exception();

            return uint32_t(value_int);
        }
        catch (std::exception)
        {
            const std::string error = "\'" + value_str + "\' is not a valid value for \'" + name + '\'';
            throw std::exception(error.c_str());
        }
    }

    template<typename T>
    T SettingsParser::ReadSpecialParam(const char* const argv[], const int index, const std::string& name, const std::map<std::string, T> values)
    {
        const std::string value_str = ReadStringParam(argv, index, name);

        const auto it = values.find(value_str);
        if (it != values.end())
            return it->second;

        const std::string error = '\'' + value_str + "\' is not a valid value for \'" + name + '\'';
        throw std::exception(error.c_str());
    }

    std::string SettingsParser::ReadOptionSpecifier(const char* const argv[], const int index)
    {
        const std::string option_str = std::string(argv[index]);

        if (!(option_str.starts_with("--") || option_str.starts_with('-')))
            throw std::exception("Option specifiers must begin with '-' or '--' (e.g. --verbose)");

        return option_str;
    }

    std::pair<std::string, std::string> SettingsParser::ReadEnvironmentVariable(const char* const argv[], const int index)
    {
        // Variable definitions must be in the form <NAME>=<VALUE> where:
        //   NAME is a string of one or more alpha-numerical characters (or '_')
        //   VALUE is a string of one or more characters, which can either be alpha-numerical
        //     or '_', '~', ':', '.', '\', '/', single-quote or double-quotes
        static const std::regex variable_regex("^[a-zA-Z1-9_]+=[a-zA-Z1-9_~:.\\/\"']+$");

        const std::string variable_str = std::string(argv[index]);

        // Check correctness of the variable definition
        if (variable_str.find('$') != std::string::npos)
            throw std::exception("Env. variable definitions must not contain the '$' character");
        if (variable_str.find('(') != std::string::npos || variable_str.find(')') != std::string::npos)
            throw std::exception("Env. variable definitions must not contain the '(' or ')' characters");
        if (!std::regex_match(variable_str, variable_regex))
            throw std::exception(("Env. variable definition '" + variable_str + "' is not in a valid format").c_str());

        // Parse NAME=VALUE into std::pair and return
        const size_t splitpos = variable_str.find('=');
        const std::string variable_name = Utilities::UpperString(variable_str.substr(0, splitpos));
        const std::string variable_value = variable_str.substr(splitpos + 1, variable_str.length());
        return { variable_name, variable_value };
    }


    void SettingsParser::PrintSettings(const Settings& settings)
    {
        std::string mode;
        switch (settings.mode)
        {
            case Settings::Mode::Install: mode = "Install"; break;
            case Settings::Mode::Debug:   mode = "Debug";   break;
            case Settings::Mode::List:    mode = "List";    break;
            case Settings::Mode::Check:   mode = "Check";   break;
            default: throw std::exception("Unknown execution mode");
        }

        std::stringstream environment;
        for (const auto entry : settings.variables)
            environment << "\n        - " << entry.first << " = " << entry.second;

        std::stringstream message;
        message << "Hansel execution settings:"
            << "\n    - Mode: " << mode
            << "\n    - Target: '" << settings.target << "'"
            << (settings.mode != Settings::Mode::List ?
               "\n    - Output path: '" + settings.output + "'" : "")
            << "\n    - Platform: " << settings.platform.ToString()
            << "\n    - Environment variables:" << environment.str()
            << "\n    - Verbose: " << (settings.verbose ? "Yes" : "No")
            << std::endl;

        Logger::InfoVerbose(message.str());
    }
}
