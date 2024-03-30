# About `Hansel`

`Hansel` is the prototype for a post-build system to manage application dependencies.
I've designed and implemented it after getting sick of the sub-optimal process that I had to deal with at my job, therefore it is specifically tailored to solve the issues that I was personally facing.

At the time, I showed this to my manager and colleagues but, even though the key idea and my work were very much appreciated, the system was never adopted because it would require a significant effort and many workflow changes at a company level.

# Overview of the problem

Here's my definition of "*dependency*" in the context of an application's post-build stage:
> **Dependency:** anything required by a program or a library to function properly.

Following this definition, some examples of post-build dependencies would be:
- Dynamically linked library files (.dll)
- Icons
- Assets (textures, data files)
- Environment variables
- etc...

This is as relevant during development (to have runnable and testable dev builds) as it is for creating distributable release packages.

Typically, developers create some kind of automation in order to place DLL files in the same folder as the executable, copy resources to their target folders, set up environment variables required for execution, etc... in the post-build stage.

More often than not, though, this is done by putting together a bunch of poorly-written scripts that are modified as needed during development, which (at least in my experience) only create chaos and confusion when some of these dependencies change.
In addition, this approach is not really scalable to large modular applications where it is critical to maintain clear relationships between each module and its dependencies.

# The proposed solution

The main idea behind `Hansel` is to move away from a *script-based* approach (which is a poor alternative of doing the operations manually) and instead adopt a **data-oriented approach** where dependencies are described as data and can therefore be handled programmatically.

Therefore I propose a data model for describing dependencies with a declarative syntax that can be parsed and processed by automated tools. This also has the added benefit of making these things more human readable and easily maintanable.

## A data model for dependencies

As already mentioned, one of the key goals is to create a system capable of scaling up to handle the dependencies of large applications.
Such applications are made of multiple modules, each of those modules might require some resources and depends on several libraries, which in turn might depend on other libraries and so on...

If we plot these relationships, we get what is usually called a *dependency tree*.

![Dependency_Tree_Image](https://github.com/pgallo725/Hansel/assets/45040214/63a96fd5-32b3-4c89-8085-6076061f1105)

My idea for describing this effectively is to basically attach an XML file to each of those modules and libraries, where only the *direct* dependencies are listed in a declarative form. I call these files **breadcrumbs**.

These files must be placed in each module or library directory, creating a completely modular dependency structure.
The job of `Hansel` will then be to reconstruct the entire dependency tree starting from a user-specified root (e.g. the main module of an application) and following the breadcrumbs.

## Breadcrumbs

As already mentioned, each breadcrumb is essentially an XML file (with the **.hbc** extension) containing the following elements:

- **\<Breadcrumb\>** header
  - Hansel version
- **\<Restrict\>** nodes
  - Provide support for conditional evaluation
  - Their content is processed only if the conditions are satisfied
- **&(VARIABLE)** placeholders
  - Replaced by the parser with the corresponding values when processing the file
  - Variable values are passed to `Hansel` as command line arguments
  - `$(PLATFORM_DIR)` and `$(OUTPUT_DIR)` are automatically defined
- **\<Dependencies\>** node
  - Parent node for the list of dependencies
  - Defines search paths for libraries, projects and scripts
  - `Hansel` will take some action for each of the items listed in this section
- **Dependency** nodes (7 types available)
  - **\<File\>** node (`<File Path=”./bin/a.dll” Destination=”$(OUTPUT_DIR)” />`)
    - Describes a dependency from a single file
    - Action: Copy the specified file into the destination folder
  - **\<Files\>** node (`<Files Path=”./bin/*.dll” Destination=”$(OUTPUT_DIR)” />`)
    - Describes a dependency from multiple files
    - Action: Copy all files matching the given pattern into the destination folder
    - Uses UNIX globbing syntax for pattern matching
  - **\<Directory\>** node (`<Directory Path=”../Plugins” Destination=”$(OUTPUT_DIR)” />`)
    - Describes a dependency from the contents of a directory
    - Action: Copy the directory and all of its contents to the destination path
  - **\<Library\>** node (`<Library Name=”openvdb” Version=”9.1.0” Destination=”$(OUTPUT_DIR)” />`)
    - Describes a dependency from a dynamic library using clear semantics (name and version)
    - Action: Recursively parse the library's breadcrumb file
      - In the example, the location of the library is determined by looking up "*/openvdb/9.1.0/openvdb.hbc*" in the provided library search paths
  - **\<Project\>** node (`<Project Name=”Test_Project” Destination=”$(OUTPUT_DIR)” />`)
    - Describes a dependency from another module (the VS "project" terminology is adopted)
    - Action: Recursively parse the module's breadcrumb file
      - In the example, the location of the project is determined by looking up "*/Test_Project/Test_Project.hbc*" in the provided project search paths
  - **\<Command\>** node (`<Command Code=”ECHO Current platform: $(PLATFORM_DIR)” />`)
    - Describes a dependency from the execution of a shell command
    - Action: Runs the provided command in the system shell
  - **\<Script\>** node (`<Script Interpreter=”./php.exe” Name=”test.php” Args=”$(MY_VAR)” />`)
    - Describes a dependency from the execution of a script
    - Action: Invokes the interpreter to run the given script with arguments
        - In the example, the location of the script is determined by looking up "*/test.php*" in the provided script search paths

  **NOTE:** the *\<Command\>* and *\<Script\>* dependency types are provided for maximum flexibility and to ease the process of transitioning from a script-based system to `Hansel`, but they should be used with extreme care to avoid falling back into the same problems as before.

Here's an example of what the breadcrumb file would look like for a library like `OpenVDB`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Breadcrumb FormatVersion="0.1">

	<Dependencies LibraryPath="../..">
	
		<!-- OpenVDB library dependencies -->
		<Library Name="Blosc" Version="1.21.0" Destination="$(OUTPUT_DIR)" />
		<Library Name="Tbb" Version="2020.1" Destination="$(OUTPUT_DIR)" />
		<Library Name="ZLib" Version="1.2.8" Destination="$(OUTPUT_DIR)" />
		<Library Name="Boost" Version="1.78.0" Destination="$(OUTPUT_DIR)" />
		
		<!-- Copy the openvdb.dll file to the destination folder -->
		<File Path="$(PLATFORM_DIR)/bin/openvdb.dll" Destination="$(OUTPUT_DIR)"/>
		
	</Dependencies>
	
</Breadcrumb>
```

## Using `Hansel`

`Hansel` is a command line tool which is given some arguments like the breadcrumb root, the output directory, the current platform (e.g. Windows or Linux) and optionally a bunch of user-defined variables.

> hansel.exe \<--mode\> \<path-to-breadcrumb\> \<install-dir\> \<platform-specifier\> [--env <variables>] [-v,--verbose]

As already mentioned, `Hansel` will start parsing from the root and recursively follow the breadcrumbs of all dependencies in order to reconstruct the entire dependency tree.
Once it has gathered all this information, it can perform several tasks depending on which of the **4 execution modes** was specified:

- **LIST**: shows the dependency tree to the user in a very clear and readable form
- **INSTALL**: realizes the dependency tree by copying files to their target locations and executing scripts with the given arguments, achieving post-build automation
- **DEBUG**: prints out all the operations that it would execute in *INSTALL* mode, without actually performing them
- **CHECK**: performs sanity checks on the dependency tree, including:
  - Detect library version conflicts (e.g. oneTBB 2020.2 and 2021.1)
  - Detect file destination path conflicts (e.g. files that would overwrite each other)
