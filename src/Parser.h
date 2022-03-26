#pragma once

#include "Types.h"
#include "Dependencies.h"

#include <tinyxml2/tinyxml2.h>  // TODO(?): move to .cpp file only (maybe forward declare tinyxml2 types)


namespace Hansel
{
    class Parser
    {
    public:

        /* 
            TODO: documentation
        */
        static std::vector<Dependency*> ParseBreadcrumb(const Path& path_to_breadcrumb, const Settings& settings);

    private:

        // Dependency nodes parsing
        static std::vector<Dependency*> ParseDependencies(const tinyxml2::XMLElement* dependencies_element, const Settings& settings);

        static ProjectDependency*   ParseProjectDependency(const tinyxml2::XMLElement* project_element, const Settings& settings, const std::vector<std::string>& project_root_paths);
        static LibraryDependency*   ParseLibraryDependency(const tinyxml2::XMLElement* library_element, const Settings& settings, const std::vector<std::string>& library_root_paths);
        static FileDependency*      ParseFileDependency(const tinyxml2::XMLElement* file_element, const Settings& settings);
        static FilesDependency*     ParseFilesDependency(const tinyxml2::XMLElement* files_element, const Settings& settings);
        static DirectoryDependency* ParseDirectoryDependency(const tinyxml2::XMLElement* directory_element, const Settings& settings);
        static CommandDependency*   ParseCommandDependency(const tinyxml2::XMLElement* command_element, const Settings& settings, const std::vector<std::string>& command_root_paths);

        // Restrict nodes handling
        static void ProcessChildrenRestrictNodes(tinyxml2::XMLNode* root, const Settings& settings);
        static bool EvaluateRestrictNode(const tinyxml2::XMLElement* restrict_element, const Settings& settings);

        // XML attributes parsing
        static std::optional<String>    GetAttributeAsRawString(const tinyxml2::XMLElement* element, const char* attribute);
        static std::optional<String>    GetAttributeAsSubstitutedString(const tinyxml2::XMLElement* element, const char* attribute, const Environment& environment);
        static std::optional<Path>      GetAttributeAsPath(const tinyxml2::XMLElement* element, const char* attribute, const Environment& environment);
        static std::optional<Version>   GetAttributeAsVersion(const tinyxml2::XMLElement* element, const char* attribute);


        // Keyword mappings for plaftorm specifier flags
        static const std::map<std::string, Platform::OperatingSystem>   StringToOperatingSystemMapping;
        static const std::map<std::string, Platform::Architecture>      StringToArchitectureMapping;
        static const std::map<std::string, Platform::Configuration>     StringToConfigurationMapping;
    };
}
