#include "Logger.h"
#include "Types.h"
#include "SettingsParser.h"
#include "Dependencies.h"
#include "Parser.h"
#include "DependencyChecker.h"

using namespace Hansel;


/** The Hansel tool is designed to be used in three possible ways:

    1) hansel.exe --install <path-to-breadcrumb> <install-dir> <platform-specifier> [--env <variables>] [-v,--verbose]

    Running Hansel in its standard form will act as an 'install' step
    after the target build process has finished. It will take care of
    copying the specified dependencies and resources to the output
    folder, running additional scripts (if specified), trying to
    automatically resolve paths and potential library conflicts.

    2) hansel.exe --check <path-to-breadcrumb> <platform-specifier> [--env <variables>] [-v,--verbose]

    When launched with the 'check' option, Hansel does not perform any
    build step but is able to analyze the dependency tree of the target
    and detect any error of missing library folders, conflicts between
    library versions, wrong paths and so on...

    3) hansel.exe --list <path-to-breadcrumb> <platform-specifier> [--env <variables>] [-v,--verbose]

    In 'list' mode, Hansel traverses the dependency tree of the specified
    target and prints it in a clear and understandable format in the
    output console.
*/


// TODOs:
//   1. Add support for <Command> dependencies by implementing their Realize() function
//   2. Improve <Restrict> node by adding a way to express conditions on environment variables
//   3. Implement Check() functionality at both the application and Dependency class levels

void RealizeDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings);
void CheckDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings);
void PrintDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings);


int main(int argc, char* argv[])
{
    Logger::Init();

    // Usage example:
    //  hansel.exe --list ./application.hbc win64d --env PLATFORM_DIR=win64d HW_ROOTDIR=./hw --verbose

    Settings settings;
    try
    {
        settings = SettingsParser::ParseCommandLine(argc, argv);
    }
    catch (std::exception e)
    {
        Logger::Error("{}\n"
            "Usage: {} TO BE DEFINED"                                           // TODO: Required parameters
            "[-e / --env <variable-definitions...>] [-v / --verbose]\n",        // Optional parameters
            e.what(), argv[0]);

        return -1;
    }

    std::vector<Dependency*> dependencies;
    try
    {
        dependencies = Parser::ParseBreadcrumb(settings.target, settings);
    }
    catch (std::exception e)
    {
        Logger::Error("{}", e.what());

        return -1;
    }

    // TEST PRINT
    PrintDependencies(dependencies, settings);

    // TEST CHECK
    CheckDependencies(dependencies, settings);

    // TEST REALIZE
    RealizeDependencies(dependencies, settings);

    return 0;
}



void RealizeDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings)
{
    std::printf("\n\nCopying dependencies of %s to '%s'\n", 
        settings.GetTargetBreadcrumbFilename().c_str(), settings.output.c_str());

    if (dependencies.size() > 0)
    {
        for (size_t i = 0; i < dependencies.size(); i++)
            dependencies[i]->Realize();
    }
    else
    {
        std::printf("\n  NO DEPENDENCIES\n");
    }
}

void CheckDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings)
{
    std::printf("\n\nChecking dependencies of %s for potential conflicts...\n",
        settings.GetTargetBreadcrumbFilename().c_str());

    if (dependencies.size() > 0)
    {
        bool all_good = DependencyChecker::Check(dependencies, settings);
        std::printf("...done! %s.\n", 
            all_good ? "No issues detected" : "Some issues detected, read the logs for more details");
    }
    else
    {
        std::printf("  NO DEPENDENCIES\n");
    }
}

void PrintDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings)
{
    std::printf("\n\n%s\n", settings.GetTargetBreadcrumbFilename().c_str());

    if (dependencies.size() > 0)
    {
        const std::string prefix = "  |";
        for (size_t i = 0; i < dependencies.size(); i++)
        {
            std::printf(prefix.c_str());	//empty line for spacing
            std::printf("\n");

            dependencies[i]->Print(prefix);
        }
    }
    else
    {
        std::printf("\n  NO DEPENDENCIES\n");
    }
}
