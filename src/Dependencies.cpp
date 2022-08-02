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

bool Hansel::ProjectDependency::Realize() const
{
	bool result = true;
	for (const Dependency* dependency : dependencies)
	{
		if (!dependency->Realize())
			result = false;
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

bool Hansel::LibraryDependency::Realize() const
{
	bool result = true;
	for (const Dependency* dependency : dependencies)
	{
		if (!dependency->Realize())
			result = false;
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

bool Hansel::FileDependency::Realize() const
{
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

bool Hansel::FilesDependency::Realize() const
{
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

bool Hansel::DirectoryDependency::Realize() const
{
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

bool Hansel::CommandDependency::Realize() const
{
	// Print command execution trace statements in verbose mode
	Logger::TraceVerbose("Executing command >{}", code);
	
	// An explicit flush of std::cout is necessary before a call to std::system,
	//  if the spawned process performs any screen I/O. 
	std::cout.flush();

	// Check the system command processor availability
	if (!std::system(nullptr))
		return false;

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

bool Hansel::ScriptDependency::Realize() const
{
	// Print script execution trace statements in verbose mode
	Logger::TraceVerbose("Executing script: '{}'", path);
	
	// An explicit flush of std::cout is necessary before a call to std::system,
	//  if the spawned process performs any screen I/O. 
	std::cout.flush();

	// Check the system command processor availability
	if (!std::system(nullptr))
		return false;

	const std::string& script_cmd = interpreter + " \"" + path + "\" " + arguments;
	std::system(script_cmd.c_str());
	return true;
}

void Hansel::ScriptDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "SCRIPT", path, nullptr);
}
