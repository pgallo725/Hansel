#pragma once

#include "Types.h"


namespace Hansel
{
    struct SettingsParser
    {
        /* Parse the application's command line parameters into the returned Settings struct.
           Throws an std::exception for any unrecoverable issue that is encountered during parsing. */
        static Settings ParseCommandLine(const int argc, const char* const argv[]);

    private:

        // Command line parameters parsing
        inline static std::string ReadStringParam(const char* const argv[], const int index, const std::string& name);
        inline static std::string ReadPathParam(const char* const argv[], const int index, const std::string& name);
        inline static uint32_t ReadUInt32Param(const char* const argv[], const int index, const std::string& name);
        template<typename T>
        inline static T ReadSpecialParam(const char* const argv[], const int index, const std::string& name, const std::map<std::string, T> values);
        inline static std::string ReadOptionSpecifier(const char* const argv[], const int index);
        inline static std::pair<std::string, std::string> ReadEnvironmentVariable(const char* const argv[], const int index);


        // Keywords mapping for the execution mode and the platform specifier string 
        static const std::map<std::string, Settings::Mode> StringToModeMapping;
        static const std::map<std::string, Platform> StringToPlatformMapping;
    };
}
