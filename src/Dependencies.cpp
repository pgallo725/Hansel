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


bool Hansel::ProjectDependency::Realize() const
{
	bool result = true;
	for (const Dependency* dependency : dependencies)
		result = result && dependency->Realize();
	return result;
}

void Hansel::ProjectDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "PROJECT", name, &dependencies);
}


bool Hansel::LibraryDependency::Realize() const
{
	bool result = true;
	for (const Dependency* dependency : dependencies)
		result = result && dependency->Realize();
	return result;
}

void Hansel::LibraryDependency::Print(const std::string& prefix) const
{
	const std::string name_version_str = name + " " + version.ToString();
	Print_Internal(prefix, "LIBRARY", name_version_str, &dependencies);
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


bool Hansel::CommandDependency::Realize() const
{
	// TODO: implement
	return true;
}

void Hansel::CommandDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "COMMAND", path, nullptr);
}
