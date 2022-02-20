#include "Settings.h"

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


// DEV NOTE: the way of specifying the destination path must take into accout
// both the case when a project/library is installed by itself and when it is built
// as a dependency of another project/library


int main(int argc, char* argv[])
{
    // Usage example:
    //  hansel.exe ./application.hbc win64d --env PLATFORM_DIR=win64d HW_ROOTDIR=./hw --verbose

    Settings settings;
    try
    {
        settings.ParseCommandLine(argc, argv);
    }
    catch (std::exception e)
    {
        std::cerr << "ERROR: " << e.what() << '\n'
            << "Usage: " << argv[0] << " TO BE DEFINED "                     // Required parameters
            << "[-e / --env <variable-definitions...>] [-v / --verbose]"     // Optional parameters
            << std::endl;;

        return -1;
    }

    return 0;
}
