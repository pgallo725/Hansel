#include "DependencyChecker.h"

#include "Logger.h"
#include "Types.h"
#include "Utilities.h"


bool Hansel::DependencyChecker::Check(const std::vector<Hansel::Dependency*>& dependencies, const Hansel::Settings& settings)
{
	// Check library dependencies for potential version conflicts
	std::map<String, LibraryDependencyEntry, StringIgnoreCaseLess> libraries;
	bool librariesOk = CheckLibraryVersions(settings.target, dependencies, libraries);

	// Check file dependencies for potential overwrite conflicts
	std::map<Path, FileDependencyEntry, StringIgnoreCaseLess> files;
	bool filesOk = CheckFileOverwrites(settings.target, dependencies, files);

	return librariesOk && filesOk;
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
						Logger::Error("{} library major version number conflict:\n\t (v{}) required by '{}'\n\t (v{}) required by '{}'",
							library->name, library->version.ToString(), depender, other.library_version.ToString(), other.depender_name);
					}
					// Emit a warning if the libraries differ by their minor version number
					else if (library->version.minor != other.library_version.minor)
					{
						Logger::Error("{} library minor version number conflict:\n\t (v{}) required by '{}'\n\t (v{}) required by '{}'",
							library->name, library->version.ToString(), depender, other.library_version.ToString(), other.depender_name);
					}
					// Notify the user if the libraries differ by their patch number
					else if (library->version.patch != other.library_version.patch)
					{
						Logger::Warn("{} library patch number conflict:\n\t (v{}) required by '{}'\n\t (v{}) required by '{}'",
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
				bool subresult = CheckLibraryVersions(library_breadcrumb_path, library->dependencies, libraries);
				if (!subresult)
					result = false;
			}
		}

		if (dependency->GetType() == Hansel::Dependency::Type::Project)
		{
			const auto* project = dynamic_cast<const Hansel::ProjectDependency*>(dependency);

			// Check sub-dependencies of projects as well
			if (project->dependencies.size() > 0)
			{
				// Workaround to get the project breadcrumb file path without storing it in the ProjectDependency class
				const Hansel::Path& project_breadcrumb_path = project->dependencies[0]->GetParentBreadcrumbPath();
				bool subresult = CheckLibraryVersions(project_breadcrumb_path, project->dependencies, libraries);
				if (!subresult)
					result = false;
			}
		}
	}

	return result;
}


bool Hansel::DependencyChecker::CheckFileOverwrites(const Hansel::String& depender,
	const std::vector<Hansel::Dependency*>& dependencies,
	std::map<Hansel::Path, FileDependencyEntry, StringIgnoreCaseLess>& files_copied)
{
	bool result = true;

	for (const auto* dependency : dependencies)
	{
		if (dependency->GetType() == Hansel::Dependency::Type::File)
		{
			const auto* file = dynamic_cast<const Hansel::FileDependency*>(dependency);

			// Construct the destination file path
			Hansel::Path file_destination = Utilities::CombinePath(file->destination,
				std::filesystem::path(file->path).filename().string());

			if (files_copied.contains(file_destination))
			{
				FileDependencyEntry other = files_copied.at(file_destination);
				if (file->path != other.file_path)
				{
					result = false;

					// Notify the user about a potentially dangerous file overwrite
					Logger::Warn("Different files are written to the same output location '{}':\n\t ({}) required by '{}'\n\t ({}) required by '{}'",
						file_destination, file->path, depender, other.file_path, other.depender_name);
				}
			}
			else
			{
				files_copied.emplace(file_destination, FileDependencyEntry{ depender, file->path });
			}
		}
		else if (dependency->GetType() == Hansel::Dependency::Type::Files)
		{
			const auto* files = dynamic_cast<const Hansel::FilesDependency*>(dependency);

			const Hansel::Path from_directory = std::filesystem::path(files->path).parent_path().string();

			std::vector<Hansel::Path> glob_files = Utilities::GlobFiles(files->path);
			for (const auto& file_path : glob_files)
			{
				// Construct the destination file path
				Hansel::Path file_subpath = file_path;
				file_subpath.erase(size_t(0), from_directory.size());
				Hansel::Path file_destination = Utilities::CombinePath(files->destination, file_subpath);

				if (files_copied.contains(file_destination))
				{
					FileDependencyEntry other = files_copied.at(file_destination);
					if (file_path != other.file_path)
					{
						result = false;

						// Notify the user about a potentially dangerous file overwrite
						Logger::Warn("Different files are written to the same output location '{}':\n\t ({}) required by '{}'\n\t ({}) required by '{}'",
							file_destination, file_path, depender, other.file_path, other.depender_name);
					}
				}
				else
				{
					files_copied.emplace(file_destination, FileDependencyEntry{ depender, file_path });
				}
			}
		}
		else if (dependency->GetType() == Hansel::Dependency::Type::Directory)
		{
			const auto* directory = dynamic_cast<const Hansel::DirectoryDependency*>(dependency);

			std::vector<Hansel::Path> directory_files = Utilities::GetAllFilesInDirectory(directory->path);
			for (const auto& file_path : directory_files)
			{
				// Construct the destination file path
				Hansel::Path file_subpath = file_path;
				file_subpath.erase(size_t(0), directory->path.size());
				Hansel::Path file_destination = Utilities::CombinePath(directory->destination, file_subpath);

				if (files_copied.contains(file_destination))
				{
					FileDependencyEntry other = files_copied.at(file_destination);
					if (file_path != other.file_path)
					{
						result = false;

						// Notify the user about a potentially dangerous file overwrite
						Logger::Warn("Different files are written to the same output location '{}':\n\t ({}) required by '{}'\n\t ({}) required by '{}'",
							file_destination, file_path, depender, other.file_path, other.depender_name);
					}
				}
				else
				{
					files_copied.emplace(file_destination, FileDependencyEntry{ depender, file_path });
				}
			}
		}
		else if (dependency->GetType() == Hansel::Dependency::Type::Library)
		{
			const auto* library = dynamic_cast<const Hansel::LibraryDependency*>(dependency);

			// Check sub-dependencies of projects as well
			if (library->dependencies.size() > 0)
			{
				// Workaround to get the library breadcrumb file path without storing it in the LibraryDependency class
				const Hansel::Path& project_breadcrumb_path = library->dependencies[0]->GetParentBreadcrumbPath();
				bool subresult = CheckFileOverwrites(project_breadcrumb_path, library->dependencies, files_copied);
				if (!subresult)
					result = false;
			}
		}
		else if (dependency->GetType() == Hansel::Dependency::Type::Project)
		{
			const auto* project = dynamic_cast<const Hansel::ProjectDependency*>(dependency);

			// Check sub-dependencies of projects as well
			if (project->dependencies.size() > 0)
			{
				// Workaround to get the project breadcrumb file path without storing it in the ProjectDependency class
				const Hansel::Path& project_breadcrumb_path = project->dependencies[0]->GetParentBreadcrumbPath();
				bool subresult = CheckFileOverwrites(project_breadcrumb_path, project->dependencies, files_copied);
				if (!subresult)
					result = false;
			}
		}
	}

	return result;
}
