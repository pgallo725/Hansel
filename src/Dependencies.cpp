#include "Dependencies.h"
#include "Logger.h"
#include "Utilities.h"


static void Print_Internal(const std::string& prefix, const std::string& type, const std::string& value,
	const std::vector<Hansel::Dependency*>* dependencies)
{
	std::printf(prefix.c_str());
	std::printf("-- [%s] %s\n", type.c_str(), value.c_str());

	if (dependencies && dependencies->size() > 0)
	{
		// Construct the prefix for the sub-dependencies list
		const std::string next_prefix = std::string(prefix) + "      |";

		for (size_t i = 0; i < dependencies->size(); i++)
		{
			std::printf(next_prefix.c_str());	//empty line for spacing
			std::printf("\n");

			(*dependencies)[i]->Print(next_prefix.c_str());
		}
	}
}


std::vector<Hansel::Dependency*> Hansel::ProjectDependency::GetAllDependencies() const
{
	// Initialize the array with the direct dependencies of this entry
	std::vector<Hansel::Dependency*> all_dependencies(dependencies);
	for (const Dependency* dependency : dependencies)
	{
		// Get the indirect dependencies from each of the children and add them to the list
		std::vector<Hansel::Dependency*> indirect_dependencies = dependency->GetAllDependencies();
		all_dependencies.insert(all_dependencies.end(), indirect_dependencies.begin(), indirect_dependencies.end());
	}
	return all_dependencies;
}

bool Hansel::ProjectDependency::Realize(bool debug, bool verbose) const
{
	bool result = true;
	for (const Dependency* dependency : dependencies)
	{
		// Realize sub-dependencies first
		if (dependency->GetType() == Dependency::Type::Library ||
			dependency->GetType() == Dependency::Type::Project)
		{
			if (!dependency->Realize(debug, verbose))
				result = false;
		}
	}

	if (debug || verbose)
		std::printf("**** PROJECT: %s\n", name.c_str());

	for (const Dependency* dependency : dependencies)
	{
		// Realize direct dependencies last
		if (dependency->GetType() != Dependency::Type::Library &&
			dependency->GetType() != Dependency::Type::Project)
		{
			if (!dependency->Realize(debug, verbose))
				result = false;
		}
	}
	return result;
}

void Hansel::ProjectDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "PROJECT", name, &dependencies);
}


std::vector<Hansel::Dependency*> Hansel::LibraryDependency::GetAllDependencies() const
{
	// Initialize the array with the direct dependencies of this entry
	std::vector<Hansel::Dependency*> all_dependencies(dependencies);
	for (const Dependency* dependency : dependencies)
	{
		// Get the indirect dependencies from each of the children and add them to the list
		std::vector<Hansel::Dependency*> indirect_dependencies = dependency->GetAllDependencies();
		all_dependencies.insert(all_dependencies.end(), indirect_dependencies.begin(), indirect_dependencies.end());
	}
	return all_dependencies;
}

bool Hansel::LibraryDependency::Realize(bool debug, bool verbose) const
{
	bool result = true;
	for (const Dependency* dependency : dependencies)
	{
		// Realize sub-dependencies first
		if (dependency->GetType() == Dependency::Type::Library ||
			dependency->GetType() == Dependency::Type::Project)
		{
			if (!dependency->Realize(debug, verbose))
				result = false;
		}
	}

	if (debug || verbose)
		std::printf("**** LIBRARY: %s %s\n", name.c_str(), version.ToString().c_str());

	for (const Dependency* dependency : dependencies)
	{
		// Realize direct dependencies last
		if (dependency->GetType() != Dependency::Type::Library &&
			dependency->GetType() != Dependency::Type::Project)
		{
			if (!dependency->Realize(debug, verbose))
				result = false;
		}
	}
	return result;
}

void Hansel::LibraryDependency::Print(const std::string& prefix) const
{
	const std::string name_version_str = name + " " + version.ToString();
	Print_Internal(prefix, "LIBRARY", name_version_str, &dependencies);
}


std::vector<Hansel::Dependency*> Hansel::FileDependency::GetAllDependencies() const
{
	// No sub-dependencies
	return {};
}

bool Hansel::FileDependency::Realize(bool debug, bool verbose) const
{
	if (debug || verbose)
	{
		const Path file_destination = Utilities::GetDestinationPath(destination, path);
		std::printf("Copy file '%s' ==> '%s'\n", path.c_str(), file_destination.c_str());

		if (debug)	// in Debug mode, return without copying the file
			return true;
	}

	const std::error_code err = Utilities::CopySingleFile(path, destination);
	if (err.value() != 0)
	{
		Logger::Error(err.message());
		return false;
	}
	return true;
}

void Hansel::FileDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "FILE", path, nullptr);
}


std::vector<Hansel::Dependency*> Hansel::FilesDependency::GetAllDependencies() const
{
	// No sub-dependencies
	return {};
}

bool Hansel::FilesDependency::Realize(bool debug, bool verbose) const
{
	if (debug || verbose)
	{
		const std::vector<Path> files = Utilities::GlobFiles(path);
		for (const auto file_path : files)
		{
			const Path file_destination = Utilities::GetDestinationPath(destination, file_path);
			std::printf("Copy file '%s' ==> '%s'\n", file_path.c_str(), file_destination.c_str());
		}

		if (debug)	// in Debug mode, return without copying the files
			return true;
	}

	const std::error_code err = Utilities::CopyMultipleFiles(path, destination);
	if (err.value() != 0)
	{
		Logger::Error(err.message());
		return false;
	}
	return true;
}

void Hansel::FilesDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "FILES", path, nullptr);
}


std::vector<Hansel::Dependency*> Hansel::DirectoryDependency::GetAllDependencies() const
{
	// No sub-dependencies
	return {};
}

bool Hansel::DirectoryDependency::Realize(bool debug, bool verbose) const
{
	if (debug || verbose)
	{
		std::printf("Copy directory '%s' ==> '%s'\n", path.c_str(), destination.c_str());

		if (debug)	// in Debug mode, return without copying the directory
			return true;
	}

	const std::error_code err = Utilities::CopyDirectory(path, destination);
	if (err.value() != 0)
	{
		Logger::Error(err.message());
		return false;
	}
	return true;
}

void Hansel::DirectoryDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "DIRECTORY", path, nullptr);
}


std::vector<Hansel::Dependency*> Hansel::CommandDependency::GetAllDependencies() const
{
	// No sub-dependencies
	return {};
}

bool Hansel::CommandDependency::Realize(bool debug, bool verbose) const
{
	// Check the system command processor availability
	if (!std::system(nullptr))
		return false;

	if (debug || verbose)
	{
		std::printf("Execute command '%s'\n", code.c_str());

		if (debug)	// in Debug mode, return without executing the command
			return true;
	}

	// An explicit flush of std::cout is necessary before a call to std::system,
	//  if the spawned process performs any screen I/O. 
	std::cout.flush();

	std::system(code.c_str());
	return true;
}

void Hansel::CommandDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "COMMAND", code, nullptr);
}


std::vector<Hansel::Dependency*> Hansel::ScriptDependency::GetAllDependencies() const
{
	// No sub-dependencies
	return {};
}

bool Hansel::ScriptDependency::Realize(bool debug, bool verbose) const
{
	// Check the system command processor availability
	if (!std::system(nullptr))
		return false;

	if (debug || verbose)
	{
		std::printf("Execute script '%s'", path.c_str());
		if (!arguments.empty())
			std::printf(" with args: '%s'\n", arguments.c_str());
		else std::printf("\n");

		if (debug)	// in Debug mode, return without executing the command
			return true;
	}

	// An explicit flush of std::cout is necessary before a call to std::system,
	//  if the spawned process performs any screen I/O. 
	std::cout.flush();

	// Build the command line string for executing the script
	std::stringstream script_command_line;
	if (!interpreter.empty())
		script_command_line << interpreter << ' ';
	script_command_line << '"' << path << '"';
	if (!arguments.empty())
		script_command_line << ' ' << arguments;
	
	std::system(script_command_line.str().c_str());
	return true;
}

void Hansel::ScriptDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "SCRIPT", path, nullptr);
}
