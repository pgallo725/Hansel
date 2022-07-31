#pragma once

#include "Dependencies.h"

namespace Hansel
{
	class DependencyChecker
	{
	public:

		static bool Check(const std::vector<Dependency*>& dependencies, const Settings& settings);

	private:

		struct StringIgnoreCaseLess
		{
			// case-independent (ci) compare_less binary function
			struct nocase_compare
			{
				bool operator() (const unsigned char& c1, const unsigned char& c2) const 
				{
					return tolower(c1) < tolower(c2);
				}
			};

			bool operator() (const std::string& s1, const std::string& s2) const 
			{
				return std::lexicographical_compare
				(s1.begin(), s1.end(),		// source range
					s2.begin(), s2.end(),   // dest range
					nocase_compare());		// comparison
			}
		};

		struct LibraryDependencyEntry
		{
			Hansel::String depender_name;
			Hansel::Version library_version;

			LibraryDependencyEntry(const Hansel::String& name, const Hansel::Version& version)
				: depender_name(name), library_version(version)
			{}
		};

		static bool CheckLibraryVersions(const Hansel::String& depender,
			const std::vector<Hansel::Dependency*>& dependencies,
			std::map<Hansel::String, LibraryDependencyEntry, StringIgnoreCaseLess>& libraries);


		struct FileDependencyEntry
		{
			Hansel::String depender_name;
			Hansel::Path file_path;

			FileDependencyEntry(const Hansel::String& name, const Hansel::Path& path)
				: depender_name(name), file_path(path)
			{}
		};

		static bool CheckFileOverwrites(const Hansel::String& depender,
			const std::vector<Hansel::Dependency*>& dependencies,
			std::map<Hansel::Path, FileDependencyEntry, StringIgnoreCaseLess>& files);
	};
}