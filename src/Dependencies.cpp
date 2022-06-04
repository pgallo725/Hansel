#include "Dependencies.h"
#include "Logger.h"

#include <iostream>

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

// TODO: implement missing

bool Hansel::ProjectDependency::Realize() const
{
	return false;
}

void Hansel::ProjectDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "PROJECT", name, &dependencies);
}


bool Hansel::LibraryDependency::Realize() const
{
	return false;
}

void Hansel::LibraryDependency::Print(const std::string& prefix) const
{
	const std::string name_version_str = name + " " + version.ToString();
	Print_Internal(prefix, "LIBRARY", name_version_str, &dependencies);
}


bool Hansel::FileDependency::Realize() const
{
	return false;
}

void Hansel::FileDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "FILE", path, nullptr);
}


bool Hansel::FilesDependency::Realize() const
{
	return false;
}

void Hansel::FilesDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "FILES", path, nullptr);
}


bool Hansel::DirectoryDependency::Realize() const
{
	return false;
}

void Hansel::DirectoryDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "DIRECTORY", path, nullptr);
}


bool Hansel::CommandDependency::Realize() const
{
	return false;
}

void Hansel::CommandDependency::Print(const std::string& prefix) const
{
	Print_Internal(prefix, "COMMAND", path, nullptr);
}
