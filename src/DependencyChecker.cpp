#include "DependencyChecker.h"

#include "Logger.h"
#include "Types.h"
#include "Utilities.h"


bool Hansel::DependencyChecker::Check(const std::vector<Hansel::Dependency*>& dependencies, const Hansel::Settings& settings)
{
	bool result = true;
	std::map<String, LibraryDependencyEntry, StringIgnoreCaseLess> libraries;
	result = result && CheckLibraryVersions(settings.target, dependencies, libraries);

	// TODO: check files and directories that get copied as well, for potential overwrites

	return result;
}


bool Hansel::DependencyChecker::CheckLibraryVersions(const Hansel::String& depender,
	const std::vector<Hansel::Dependency*>& dependencies,
	std::map<Hansel::String, LibraryDependencyEntry, StringIgnoreCaseLess>& libraries)
{
	bool result = true;

	for (const auto* dependency : dependencies)
	{
		if (dependency->GetType() == Hansel::Dependency::Type::Library)
		{
			const auto* library = dynamic_cast<const Hansel::LibraryDependency*>(dependency);

			// Check if there's an existing dependency to this library
			if (libraries.contains(library->name))
			{
				LibraryDependencyEntry other = libraries.at(library->name);
				if (library->version != other.library_version)
				{
					result = false;

					// Emit an error if the libraries differ by their major version number
					if (library->version.major != other.library_version.major)
					{
						Logger::Error("{} library major version number conflict:\n\t (v{}) '{}'\n\t (v{}) '{}'",
							library->name, library->version.ToString(), depender, other.library_version.ToString(), other.depender_name);
					}
					// Emit a warning if the libraries differ by their minor version number
					else if (library->version.minor != other.library_version.minor)
					{
						Logger::Error("{} library minor version number conflict:\n\t (v{}) '{}'\n\t (v{}) '{}'",
							library->name, library->version.ToString(), depender, other.library_version.ToString(), other.depender_name);
					}
					// Notify the user if the libraries differ by their patch number
					else if (library->version.patch != other.library_version.patch)
					{
						Logger::Warn("{} library patch number conflict:\n\t (v{}) '{}'\n\t (v{}) '{}'",
							library->name, library->version.ToString(), depender, other.library_version.ToString(), other.depender_name);
					}

					// Update the entry in the map to keep the highest library version of the two
					if (library->version > other.library_version)
						libraries.insert_or_assign(library->name, LibraryDependencyEntry{ depender, library->version });
				}
			}
			else
			{
				libraries.emplace(library->name, LibraryDependencyEntry{ depender, library->version });
			}

			// Recursively check this library's sub-dependencies
			if (library->dependencies.size() > 0)
			{
				// Workaround to get the library breadcrumb file path without storing it in the LibraryDependency class
				const Hansel::Path& library_breadcrumb_path = library->dependencies[0]->GetParentBreadcrumbPath();
				result = result && CheckLibraryVersions(library_breadcrumb_path, library->dependencies, libraries);
			}
		}

		if (dependency->GetType() == Hansel::Dependency::Type::Project)
		{
			const auto* project = dynamic_cast<const Hansel::ProjectDependency*>(dependency);

			// Check sub-dependencies of projects as well
			if (project->dependencies.size() > 0)
			{
				// Workaround to get the project breadcrumb file path without storing it in the LibraryDependency class
				const Hansel::Path& project_breadcrumb_path = project->dependencies[0]->GetParentBreadcrumbPath();
				result = result && CheckLibraryVersions(project_breadcrumb_path, project->dependencies, libraries);
			}
		}
	}

	return result;
}
