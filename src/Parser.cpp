#include "Parser.h"
#include "Logger.h"
#include "Utilities.h"

#include <tinyxml2/tinyxml2.h>


namespace Hansel
{
    const std::map<std::string, Platform::OperatingSystem>
        Parser::StringToOperatingSystemMapping
    {
        { "win",        Platform::OperatingSystem::Windows },
        { "windows",    Platform::OperatingSystem::Windows },
        { "mac",        Platform::OperatingSystem::Mac },
        { "macos",      Platform::OperatingSystem::Mac },
        { "linux",      Platform::OperatingSystem::Linux },
        { "all",        Platform::OperatingSystem::ANY },
        { "any",        Platform::OperatingSystem::ANY },
        { "*",          Platform::OperatingSystem::ANY },
    };

    const std::map<std::string, Platform::Architecture>
        Parser::StringToArchitectureMapping
    {
        { "x86",    Platform::Architecture::x86 },
        { "x64",    Platform::Architecture::x64 },
        { "amd64",  Platform::Architecture::x64 },
        { "all",    Platform::Architecture::ANY },
        { "any",    Platform::Architecture::ANY },
        { "*",      Platform::Architecture::ANY },
    };

    const std::map<std::string, Platform::Configuration>
        Parser::StringToConfigurationMapping
    {
        { "debug",      Platform::Configuration::Debug },
        { "dbg",        Platform::Configuration::Debug },
        { "release",    Platform::Configuration::Release },
        { "rel",        Platform::Configuration::Release },
        { "all",        Platform::Configuration::ANY },
        { "any",        Platform::Configuration::ANY },
        { "*",          Platform::Configuration::ANY },
    };


    std::vector<Dependency*> Parser::ParseBreadcrumb(const Path& path_to_breadcrumb, const Settings& settings)
    {
        // Check if file exists
        if (!std::filesystem::exists(path_to_breadcrumb))
        {
            throw std::exception(("no breadcrumb file found at '" + path_to_breadcrumb + "'").c_str());
        }

        // Load breadcrumb file and parse XML document
        tinyxml2::XMLDocument document;
        document.LoadFile(path_to_breadcrumb.c_str());

        // Check XML parsing errors
        if (document.Error())
        {
            throw std::exception(document.ErrorStr());
        }

        // Access the top-level <Breadcrumb> node
        tinyxml2::XMLElement* breadcrumb_element = document.FirstChildElement("Breadcrumb");
        if (!breadcrumb_element)
        {
            throw std::exception("invalid breadcrumb file (no top-level <Breadcrumb> element)");
        }

        // TODO: Check breadcrumb format version and do something with it
        const std::optional<Version> breadcrumb_version = GetAttributeAsVersion(breadcrumb_element, "FormatVersion");
        if (!breadcrumb_version.has_value())
        {
            throw std::exception("invalid breadcrumb file (missing 'FormatVersion' attribute)");
        }


        // Search for all <Restrict> nodes in the document and evaluate them
        ProcessChildrenRestrictNodes(breadcrumb_element, settings);


        std::vector<Dependency*> dependencies;

        // Iterate through children of the <Breadcrumb> node looking for <Dependencies> elements to parse
        // NOTE: multiple <Dependencies> nodes are supported (and their contents merged together)
        const tinyxml2::XMLElement* element = breadcrumb_element->FirstChildElement();
        while (element != nullptr)
        {
            const std::string element_name = element->Name();

            if (element_name == "Dependencies")
            {
                std::vector<Dependency*> some_dependencies = ParseDependencies(element, settings);
                dependencies.insert(dependencies.end(), some_dependencies.begin(), some_dependencies.end());
            }
            else
            {
                throw std::exception(("element of type <" + element_name + "> is not supported at this location").c_str());
            }

            element = element->NextSiblingElement();
        }

        if (dependencies.empty())
        {
            Logger::Info("The breadcrumb file '{}' did not contain any dependency", path_to_breadcrumb);
        }

        return dependencies;
    }


    std::vector<Dependency*> Parser::ParseDependencies(const tinyxml2::XMLElement* dependencies_element, const Settings& settings)
    {
        std::vector<Dependency*> dependencies;

        // Parse <ProjectPath>, <LibraryPath> and <CommandPath> attributes
        std::optional<std::string> project_path_attribute = GetAttributeAsSubstitutedString(dependencies_element, "ProjectPath", settings.variables);
        std::optional<std::string> library_path_attribute = GetAttributeAsSubstitutedString(dependencies_element, "LibraryPath", settings.variables);
        std::optional<std::string> command_path_attribute = GetAttributeAsSubstitutedString(dependencies_element, "CommandPath", settings.variables);

        // TODO(?): automatically add the current target directory to the root paths ?

        std::vector<std::string> project_root_paths;
        if (project_path_attribute.has_value())
        {
            project_root_paths = Utilities::SplitString(project_path_attribute.value(), ';');
            for (size_t i = 0; i < project_root_paths.size(); i++)
            {
                project_root_paths[i] = Utilities::TrimString(project_root_paths[i]);
                if (project_root_paths[i].starts_with('.'))
                    project_root_paths[i] = Utilities::CombinePath(settings.GetTargetDirectoryPath(), project_root_paths[i]);
            }
        }

        std::vector<std::string> library_root_paths;
        if (library_path_attribute.has_value())
        {
            library_root_paths = Utilities::SplitString(library_path_attribute.value(), ';');
            for (size_t i = 0; i < library_root_paths.size(); i++)
            {
                library_root_paths[i] = Utilities::TrimString(library_root_paths[i]);
                if (library_root_paths[i].starts_with('.'))
                    library_root_paths[i] = Utilities::CombinePath(settings.GetTargetDirectoryPath(), library_root_paths[i]);
            }
        }

        std::vector<std::string> command_root_paths;
        if (command_path_attribute.has_value())
        {
            command_root_paths = Utilities::SplitString(command_path_attribute.value(), ';');
            for (size_t i = 0; i < command_root_paths.size(); i++)
            {
                command_root_paths[i] = Utilities::TrimString(command_root_paths[i]);
                if (command_root_paths[i].starts_with('.'))
                    command_root_paths[i] = Utilities::CombinePath(settings.GetTargetDirectoryPath(), command_root_paths[i]);
            }
        }


        // Iterate over all <Dependencies> children elements and parse them accordingly
        const tinyxml2::XMLElement* element = dependencies_element->FirstChildElement();
        while (element != nullptr)
        {
            if (!element->NoChildren())
                throw std::exception("dependency specifier elements must not have any children");

            const std::string element_name = element->Name();

            if (element_name == "Project")
            {
                dependencies.push_back(ParseProjectDependency(element, settings, project_root_paths));
            }
            else if (element_name == "Library")
            {
                dependencies.push_back(ParseLibraryDependency(element, settings, library_root_paths));
            }
            else if (element_name == "File")
            {
                dependencies.push_back(ParseFileDependency(element, settings));
            }
            else if (element_name == "Files")
            {
                dependencies.push_back(ParseFilesDependency(element, settings));
            }
            else if (element_name == "Directory")
            {
                dependencies.push_back(ParseDirectoryDependency(element, settings));
            }
            else if (element_name == "Command")
            {
                dependencies.push_back(ParseCommandDependency(element, settings, command_root_paths));
            }
            else
            {
                throw std::exception(("element of type <" + element_name + "> is not supported at this location").c_str());
            }

            element = element->NextSiblingElement();
        }

        return dependencies;
    }


    ProjectDependency* Parser::ParseProjectDependency(const tinyxml2::XMLElement* project_element,
        const Settings& settings, const std::vector<std::string>& project_root_paths)
    {
        const std::optional<std::string> name = GetAttributeAsSubstitutedString(project_element, "Name", settings.variables);
        if (!name.has_value())
            throw std::exception("invalid <Project> node (missing 'Name' attribute)");

        const std::optional<Path> path = GetAttributeAsPath(project_element, "Path", settings.variables);

        const std::optional<Path> destination = GetAttributeAsPath(project_element, "Destination", settings.variables);
        if (!destination.has_value())
            throw std::exception("invalid <Project> node (missing 'Destination' attribute)");

        // Resolve project directory using the Path attribute (if specified) or the value of the Name attribute
        Path project_directory_path;
        if (path.has_value())
        {
            project_directory_path = Utilities::CombinePath(settings.GetTargetDirectoryPath(), path.value());
        }
        else
        {
            const std::optional<Path> resolved_path = Utilities::ResolvePath(name.value(), project_root_paths);
            if (!resolved_path.has_value())
                throw std::exception(("couldn't resolve '" + name.value() + "' project directory").c_str());

            project_directory_path = resolved_path.value();
        }

        // Derive the path of the target breadcrumb
        Path project_breadcrumb_path = Utilities::CombinePath(project_directory_path, name.value() + ".hbc");

        // Recursively parse the target breadcrumb with updated settings
        Settings parser_settings = settings;
        parser_settings.target = project_breadcrumb_path;

        std::vector<Dependency*> project_dependencies = ParseBreadcrumb(project_breadcrumb_path, parser_settings);

        return new ProjectDependency
        (
            name.value(),
            project_directory_path,
            destination.value(),
            project_dependencies
        );
    }

    LibraryDependency* Parser::ParseLibraryDependency(const tinyxml2::XMLElement* library_element,
        const Settings& settings, const std::vector<std::string>& library_root_paths)
    {
        const std::optional<std::string> name = GetAttributeAsSubstitutedString(library_element, "Name", settings.variables);
        if (!name.has_value())
            throw std::exception("invalid <Library> node (missing 'Name' attribute)");

        const std::optional<Version> version = GetAttributeAsVersion(library_element, "Version");
        if (!version.has_value())
            throw std::exception("invalid <Library> node (missing 'Version' attribute)");

        const std::optional<Path> path = GetAttributeAsPath(library_element, "Path", settings.variables);

        const std::optional<Path> destination = GetAttributeAsPath(library_element, "Destination", settings.variables);
        if (!destination.has_value())
            throw std::exception("invalid <Library> node (missing 'Destination' attribute)");

        // Resolve library directory using the Path attribute (if specified) or the values of the Name/Version attributes
        Path library_directory_path;
        if (path.has_value())
        {
            library_directory_path = Utilities::CombinePath(settings.GetTargetDirectoryPath(), path.value());
        }
        else
        {
            const std::optional<Path> resolved_path = Utilities::ResolvePath(name.value() + "/" + version.value().ToString(), library_root_paths);
            if (!resolved_path.has_value())
                throw std::exception(("couldn't resolve '" + name.value() + "(" + version.value().ToString() + ")' library directory").c_str());

            library_directory_path = resolved_path.value();
        }

        // Derive the path of the target breadcrumb
        Path library_breadcrumb_path = Utilities::CombinePath(library_directory_path, name.value() + ".hbc");

        // Recursively parse the target breadcrumb with updated settings
        Settings parser_settings = settings;
        parser_settings.target = library_breadcrumb_path;

        std::vector<Dependency*> library_dependencies = ParseBreadcrumb(library_breadcrumb_path, parser_settings);

        return new LibraryDependency
        (
            name.value(),
            version.value(),
            library_directory_path,
            destination.value(),
            library_dependencies
        );
    }

    FileDependency* Parser::ParseFileDependency(const tinyxml2::XMLElement* file_element,
        const Settings& settings)
    {
        const std::optional<Path> path = GetAttributeAsPath(file_element, "Path", settings.variables);
        if (!path.has_value())
            throw std::exception("invalid <File> node (missing 'Path' attribute)");

        const std::optional<Path> destination = GetAttributeAsPath(file_element, "Destination", settings.variables);
        if (!destination.has_value())
            throw std::exception("invalid <File> node (missing 'Destination' attribute)");

        // Extract the "full" path to the dependency file
        // TODO: just combining these paths is probably not enough: need to handle the case where it may be 
        // an absolute path instead of a relative one, and other correctness checks
        const Path complete_file_path = Utilities::CombinePath(settings.GetTargetDirectoryPath(), path.value());

        return new FileDependency
        (
            complete_file_path,
            destination.value()
        );
    }

    FilesDependency* Parser::ParseFilesDependency(const tinyxml2::XMLElement* files_element,
        const Settings& settings)
    {
        const std::optional<Path> path = GetAttributeAsPath(files_element, "Path", settings.variables);
        if (!path.has_value())
            throw std::exception("invalid <Files> node (missing 'Path' attribute)");

        const std::optional<Path> destination = GetAttributeAsPath(files_element, "Destination", settings.variables);
        if (!destination.has_value())
            throw std::exception("invalid <Files> node (missing 'Destination' attribute)");

        // Extract the "full" path to the dependency files
        // TODO: just combining these paths is probably not enough: need to handle the case where it may be 
        // an absolute path instead of a relative one, and other correctness checks
        const Path complete_files_path = Utilities::CombinePath(settings.GetTargetDirectoryPath(), path.value());

        return new FilesDependency
        (
            complete_files_path,
            destination.value()
        );
    }

    DirectoryDependency* Parser::ParseDirectoryDependency(const tinyxml2::XMLElement* directory_element,
        const Settings& settings)
    {
        const std::optional<Path> path = GetAttributeAsPath(directory_element, "Path", settings.variables);
        if (!path.has_value())
            throw std::exception("invalid <Directory> node (missing 'Path' attribute)");

        const std::optional<Path> destination = GetAttributeAsPath(directory_element, "Destination", settings.variables);
        if (!destination.has_value())
            throw std::exception("invalid <Directory> node (missing 'Destination' attribute)");

        // Extract the "full" path to the dependency directory
        // TODO: just combining these paths is probably not enough: need to handle the case where it may be 
        // an absolute path instead of a relative one, and other correctness checks
        const Path complete_directory_path = Utilities::CombinePath(settings.GetTargetDirectoryPath(), path.value());

        return new DirectoryDependency
        (
            complete_directory_path,
            destination.value()
        );
    }

    CommandDependency* Parser::ParseCommandDependency(const tinyxml2::XMLElement* command_element,
        const Settings& settings, const std::vector<std::string>& command_root_paths)
    {
        const std::optional<std::string> name = GetAttributeAsSubstitutedString(command_element, "Name", settings.variables);

        const std::optional<Path> path = GetAttributeAsPath(command_element, "Path", settings.variables);
        if (!name.has_value() && !path.has_value())
            throw std::exception("invalid <Command> node (missing atleast one of 'Name' or 'Path' attributes)");

        const std::optional<std::string> arguments = GetAttributeAsSubstitutedString(command_element, "Arguments", settings.variables);
        if (!arguments.has_value())
            throw std::exception("invalid <Command> node (missing 'Arguments' attribute)");

        // Derive filename from Name or Path attributes
        const std::string filename = (name.has_value() && !path.has_value())
            ? name.value()
            : std::filesystem::path(path.value()).filename().string();

        // Resolve command path using the Path attribute (if specified) or the value of the Name attribute
        Path command_path;
        if (path.has_value())
        {
            command_path = Utilities::CombinePath(settings.GetTargetDirectoryPath(), path.value());
        }
        else
        {
            const std::optional<Path> resolved_path = Utilities::ResolvePath(name.value(), command_root_paths);
            if (!resolved_path.has_value())
                throw std::exception(("couldn't resolve '" + name.value() + "' command path").c_str());

            command_path = resolved_path.value();
        }

        return new CommandDependency
        (
            name.value_or(filename),
            command_path,
            arguments.value()
        );
    }


    void Parser::ProcessChildrenRestrictNodes(tinyxml2::XMLNode* root, const Settings& settings)
    {
        if (!root || root->NoChildren())
            return;

        // Iterate over all children nodes to find <Restrict> elements
        tinyxml2::XMLNode* node = root->FirstChild();
        while (node != nullptr)
        {
            tinyxml2::XMLElement* element = node->ToElement();
            if (element)
            {
                const std::string element_name = element->Name();

                if (element_name == "Restrict")
                {
                    if (element->NoChildren())
                    {
                        Logger::Warn("The <Restrict> node at {} (line {}) has no children and will be skipped",
                            settings.GetTargetBreadcrumbFilename(), element->GetLineNum());
                    }
                    else
                    {
                        // Check for <Restrict> nodes in children first
                        ProcessChildrenRestrictNodes(node, settings);

                        // Evaluate the condition to know what to do with children nodes
                        if (EvaluateRestrictNode(element, settings) == true)
                        {
                            // Move all children nodes up a level
                            while (!element->NoChildren())
                                element->Parent()->InsertAfterChild(element, element->FirstChild());
                        }
                        else
                        {
                            // Remove all children nodes
                            node->DeleteChildren();
                        }

                        tinyxml2::XMLNode* prev = node->PreviousSibling();

                        // Remove the <Restrict> node after it has been evaluated
                        node->Parent()->DeleteChild(node);

                        node = prev;
                    }
                }
                else
                {
                    // Recursively check the entire XML document tree
                    ProcessChildrenRestrictNodes(node, settings);
                }
            }

            node = node->NextSibling();
        }
    }

    /* Utility function used to parse OR'd combinations of flags for the 
        'Platform', 'Architecture' and 'Configuration' restrict attributes
    */
    template<Enum T>
    static T ParsePlatformSpecifierFlags(const std::string& field_name, const std::string& field_value, const std::map<std::string, T>& mapping)
    {
        T result = T(0);

        std::vector<std::string> strings = Utilities::SplitString(field_value, '|');
        for (std::string& str : strings)
        {
            // Remove possible variations by trimming whitespaces and lowering the string
            str = Utilities::TrimString(str);
            str = Utilities::LowerString(str);

            if (str.empty())
                continue;

            const auto it = mapping.find(str);
            if (it != mapping.end())
            {
                // Combine flags together with OR
                // TODO(?): check if the flag has been repeated multiple times
                //   and possibly warn the user if in verbose mode
                result = result | it->second;
            }
            else
            {
                throw std::exception(("'" + str + "' is not a valid <" + field_name + "> flag").c_str());
            }
        }

        if (result == T(0))
        {
            throw std::exception("platform specifier flags cannot be left empty");
        }
        return result;
    }

    bool Parser::EvaluateRestrictNode(const tinyxml2::XMLElement* restrict_element, const Settings& settings)
    {
        if (!restrict_element)
            return false;

        // Read <Platform> attribute flags
        std::optional<std::string> platform_attribute = GetAttributeAsRawString(restrict_element, "Platform");
        if (!platform_attribute.has_value())
            throw std::exception("invalid <Restrict> node (missing 'Platform' attribute)");

        Platform::OperatingSystem os_mask = ParsePlatformSpecifierFlags<Platform::OperatingSystem>
        (
            "Platform",
            platform_attribute.value(),
            StringToOperatingSystemMapping
        );

        // Read <Architecture> attribute flags
        std::optional<std::string> architecture_attribute = GetAttributeAsRawString(restrict_element, "Architecture");
        if (!architecture_attribute.has_value())
            throw std::exception("invalid <Restrict> node (missing 'Architecture' attribute)");

        Platform::Architecture arch_mask = ParsePlatformSpecifierFlags<Platform::Architecture>
        (
            "Architecture",
            architecture_attribute.value(),
            StringToArchitectureMapping
        );

        // Read <Configuration> attribute flags
        std::optional<std::string> configuration_attribute = GetAttributeAsRawString(restrict_element, "Configuration");
        if (!configuration_attribute.has_value())
            throw std::exception("invalid <Restrict> node (missing 'Configuration' attribute)");

        Platform::Configuration config_mask = ParsePlatformSpecifierFlags<Platform::Configuration>
        (
            "Configuration",
            configuration_attribute.value(),
            StringToConfigurationMapping
        );

        // Compare the restrict flags against the global platform specifier to check if it matches
        if ((uint16_t(settings.platform.os) & uint16_t(os_mask)) != 0) return true;
        if ((uint16_t(settings.platform.arch) & uint16_t(arch_mask)) != 0) return true;
        if ((uint16_t(settings.platform.config) & uint16_t(config_mask)) != 0) return true;

        return false;
    }


    std::optional<std::string> Parser::GetAttributeAsRawString(const tinyxml2::XMLElement* element, const char* attribute)
    {
        const char* attribute_value = element->Attribute(attribute);
        if (attribute_value == nullptr)
            return std::optional<std::string>(std::nullopt);
        return std::string(attribute_value);
    }

    std::optional<std::string> Parser::GetAttributeAsSubstitutedString(const tinyxml2::XMLElement* element, const char* attribute, const Environment& environment)
    {
        std::optional<std::string> attribute_string = GetAttributeAsRawString(element, attribute);
        if (!attribute_string.has_value())
            return std::optional<std::string>(std::nullopt);

        // Variable placeholders in attribute strings must be of the form $(NAME)
        static const std::regex variable_regex("\\$\\([a-zA-Z1-9_]*\\)");

        std::string attribute_value = attribute_string.value();

        // Find all variable placeholders and substitute them with variable values
        std::smatch variable_match;
        while (std::regex_search(attribute_value, variable_match, variable_regex))
        {
            size_t match_position = variable_match.position();
            size_t match_length = variable_match.length();

            if (match_length > 3)
            {
                // Get variable name
                const std::string& variable_name = Utilities::UpperString(attribute_value.substr(match_position + 2, match_length - 3));

                // Find variable in map and get its value
                const auto it = environment.find(variable_name);
                if (it == environment.end())
                    throw std::exception(("cannot substitute $(" + variable_name + "), variable not defined").c_str());
                const std::string& variable_value = it->second;

                // Replace variable value into original string
                attribute_value.erase(match_position, match_length);
                attribute_value.insert(match_position, variable_value);
            }
            else
            {
                Logger::Warn("Empty variable placeholder '$()', skipping substitution");
            }
        }

        return attribute_value;
    }

    std::optional<Path> Parser::GetAttributeAsPath(const tinyxml2::XMLElement* element, const char* attribute, const Environment& environment)
    {
        // TODO(?): is it correct to always substitute variables in paths ?
        std::optional<std::string> path_string = GetAttributeAsSubstitutedString(element, attribute, environment);
        if (!path_string.has_value())
            return std::optional<Path>(std::nullopt);

        try // validate path and throw exception if not valid
        {
            const std::filesystem::path path(path_string.value());
            return Path(path.lexically_normal().string());
        }
        catch (std::exception)
        {
            const std::string error = "\'" + path_string.value() + "\' is not a valid path";
            return std::optional<Path>(std::nullopt);
        }
    }

    std::optional<Version> Parser::GetAttributeAsVersion(const tinyxml2::XMLElement* element, const char* attribute)
    {
        // Version numbers must be in the form of MAJOR.MINOR[.PATCH] where
        // all three components are positive integer values, only numeric characters
        // are tolerated (leading and trailing whitespaces are trimmed before parsing)
        static const std::regex version_regex("^\\d+\\.\\d+(\\.\\d+)?$");

        std::optional<std::string> version_attribute = GetAttributeAsRawString(element, attribute);
        if (!version_attribute.has_value())
            return std::optional<Version>(std::nullopt);

        const std::string version_str = Utilities::TrimString(version_attribute.value());
        if (!std::regex_match(version_str, version_regex))
            throw std::exception("version number does not match the MAJOR.MINOR[.PATCH] format");   // TODO: maybe should return null ? (unsure)

        std::vector<std::string> version_number_components = Utilities::SplitString(version_str, '.');

        uint32_t version_major = uint32_t(std::stoi(version_number_components[0]));
        uint32_t version_minor = uint32_t(std::stoi(version_number_components[1]));

        if (version_number_components.size() == 3)
        {
            uint32_t version_patch = uint32_t(std::stoi(version_number_components[2]));
            return Version{ version_major, version_minor, version_patch };
        }
        else
        {
            return Version{ version_major, version_minor };
        }
    }
}