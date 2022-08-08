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

    2) hansel.exe --debug <path-to-breadcrumb> <install-dir> <platform-specifier> [--env <variables>] [-v,--verbose]

    In debug mode, Hansel will 'simulate' the --install mode execution,
    printing all the operations that it would normally perform, to let
    the user observe its behaviour, without actually modifying the filesystem.

    3) hansel.exe --check <path-to-breadcrumb> <install-dir> <platform-specifier> [--env <variables>] [-v,--verbose]

    When launched with the 'check' option, Hansel does not perform any
    build step but is able to analyze the dependency tree of the target
    and detect any error of missing library folders, conflicts between
    library versions, wrong paths and so on...

    4) hansel.exe --list <path-to-breadcrumb> <platform-specifier> [--env <variables>] [-v,--verbose]

    In 'list' mode, Hansel traverses the dependency tree of the specified
    target and prints it in a clear and understandable format in the
    output console.

    5) hansel.exe --help

    Additionally, the 'help' command prints the instructions for using 
    the application and all the available command line options.
*/


void ShowHelp();
void RealizeDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings);
void DebugRealizeDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings);
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
        Logger::Error("{}", e.what());

        ShowHelp();

        return -1;
    }

    std::vector<Dependency*> dependencies;
    if (settings.mode != Settings::Mode::Help)
    {
        try
        {
            dependencies = Parser::ParseBreadcrumb(settings.target, settings);
        }
        catch (std::exception e)
        {
            Logger::Error("{}", e.what());

            return -1;
        }
    }

    switch (settings.mode)
    {
        case Settings::Mode::Help:
        {
            ShowHelp();
            break;
        }

        case Settings::Mode::Install:
        {
            RealizeDependencies(dependencies, settings);
            break;
        }

        case Settings::Mode::Debug:
        {
            DebugRealizeDependencies(dependencies, settings);
            break;
        }

        case Settings::Mode::Check:
        {
            CheckDependencies(dependencies, settings);
            break;
        }

        case Settings::Mode::List:
        {
            PrintDependencies(dependencies, settings);
            break;
        }

        default:
            throw std::exception("Unknown execution mode");
    }

    std::printf("\n");

    return 0;
}



void ShowHelp()
{
    std::printf("\nUsage:  Hansel --help"
                "\n        Hansel --install <path-to-breadcrumb> <install-dir> <platform> [-e <variables>] [-v]"
                "\n        Hansel --debug <path-to-breadcrumb> <install-dir> <platform> [-e <variables>] [-v]"
                "\n        Hansel --check <path-to-breadcrumb> <install-dir> <platform> [-e <variables>] [-v]"
                "\n        Hansel --list <path-to-breadcrumb> <platform> [-e <variables>] [-v]"
                "\n"
                "\nModes:"
                "\n"
                "\n  -h / --help             Shows this help message"
                "\n  -i / --install          Realize (copy / execute) all dependencies of the target breadcrumb"
                "\n  -d / --debug            Simulate --install mode and print all actions that would be performed"
                "\n  -c / --check            Analyze the dependency tree and detect issues such as library or file conflicts"
                "\n  -l / --list             Visualize the entire depedency tree of the target breadcrumb"
                "\n"
                "\nRequired:"
                "\n"
                "\n  <path-to-breadcrumb>    Path of the target Hansel breadcrumb file (*.hbc)"
                "\n  <install-dir>           [INSTALL / CHECK] Output path of the installation process"
                "\n  <platform>              Target platform for which dependencies will be processed"
                "\n                           The platform specifier must be in the format xxxYY[d] where:"
                "\n                             xxx = { win, linux, macosx }  (OS)"
                "\n                              YY = { 32, 64 }              (Architecture)"
                "\n                               d = Debug flag              (Configuration)"
                "\nOptional:"
                "\n"
                "\n  -e / --env <variables>  Set of environment variable definitions."
                "\n                           A variable definition is in the format VARIABLE_NAME=value"
                "\n  -v / --verbose          Enable additional program outputs (verbose)"
                "\n"
    );
}

void RealizeDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings)
{
    std::printf("\nCopying dependencies of %s to '%s'...\n", 
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

void DebugRealizeDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings)
{
    std::printf("\nCopying dependencies of %s to '%s'...\n\n",
        settings.GetTargetBreadcrumbFilename().c_str(), settings.output.c_str());

    if (dependencies.size() > 0)
    {
        for (size_t i = 0; i < dependencies.size(); i++)
            dependencies[i]->DebugRealize();
    }
    else
    {
        std::printf("\n  NO DEPENDENCIES\n");
    }
}

void CheckDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings)
{
    std::printf("\nChecking dependencies of %s for potential conflicts...\n",
        settings.GetTargetBreadcrumbFilename().c_str());

    if (dependencies.size() > 0)
    {
        bool all_good = DependencyChecker::Check(dependencies, settings);
        std::printf("...done! %s.\n", 
            all_good ? "No issues detected" : "Some issues detected, read the logs for more details");
    }
    else
    {
        std::printf("\n  NO DEPENDENCIES\n");
    }
}

void PrintDependencies(const std::vector<Dependency*>& dependencies, const Settings& settings)
{
    std::printf("\n%s\n", settings.GetTargetBreadcrumbFilename().c_str());

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
