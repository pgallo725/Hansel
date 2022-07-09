#pragma once

#include "Types.h"

#include "glob/glob.hpp"

#include <algorithm>
#include <cctype>


namespace Hansel
{
	namespace Utilities
	{
        /* Returns a copy of the provided string, with all alphabetic characters
            converted to their lowercase form. */
        static std::string LowerString(const std::string& str)
        {
            std::string str_lower;
            std::transform(str.begin(), str.end(), std::back_inserter(str_lower),
                [](unsigned char c) { return std::tolower(c); });

            return str_lower;
        }

        /* Returns a copy of the provided string, with all alphabetic characters
            converted to their uppercase form.  */
        static std::string UpperString(const std::string& str)
        {
            std::string str_upper;
            std::transform(str.begin(), str.end(), std::back_inserter(str_upper),
                [](unsigned char c) { return std::toupper(c); });

            return str_upper;
        }

        /* Returns a copy of the provided string, with all leading and trailing
            whitespaces removed. */
        static std::string TrimString(const std::string& str)
        {
            std::string str_copy = str;

            // Remove leading whitespaces
            auto first_nonspace_pos = std::find_if(str_copy.begin(), str_copy.end(),
                [](char c) {
                    return !std::isspace<char>(c, std::locale::classic());
                });
            str_copy.erase(str_copy.begin(), first_nonspace_pos);

            // Remove trailing whitespaces
            auto last_nonspace_pos = std::find_if(str_copy.rbegin(), str_copy.rend(),
                [](char c) {
                    return !std::isspace<char>(c, std::locale::classic());
                });
            str_copy.erase(last_nonspace_pos.base(), str_copy.end());

            return str_copy;
        }

        /* Returns an array of sub-strings, obtained by splitting the provided
            string at every occurrence of the <delimiter> character.
           The delimiter is not included in the sub-strings. */
        static std::vector<std::string> SplitString(const std::string& str, const char delimiter = ' ')
        {
            std::vector<std::string> strings;
            size_t start = 0;
            size_t end = str.find(delimiter);
            while (end != std::string::npos) {
                strings.push_back(str.substr(start, end - start));
                start = end + 1;
                end = str.find(delimiter, start);
            }
            strings.push_back(str.substr(start, end - start));
            return strings;
        }


        /* Returns a path obtained by concatenating the provided paths with a '/',
            after trimming them and removing additional '/' characters. 
           The resulting path is normalized according to std::path::lexically_normal(). */
        static Path CombinePath(const Path& left, const Path& right)
        {
            // Trim left path and strip trailing '/'
            Path trimmed_left_path = Utilities::TrimString(left);
            while (trimmed_left_path.ends_with('/'))
                trimmed_left_path.pop_back();

            // Trim right path and strip leading '/'
            Path trimmed_right_path = Utilities::TrimString(right);
            while (trimmed_right_path.starts_with('/'))
                trimmed_right_path.erase(trimmed_right_path.begin());

            // Concatenate paths together with a '/'
            std::filesystem::path path = std::filesystem::path(trimmed_left_path) / std::filesystem::path(trimmed_right_path);

            // Canonicalize path
            return path.lexically_normal().string();
        }

        /* Combines the given relative path with the list of provided roots, and returns
            the first path (if any) that matches an actually existing filesystem path. 
           Root paths are tried in the same order that they are specified in the 'root_paths'
            vector, which therefore determines the lookup priority. */
        static std::optional<Path> ResolvePath(const Path& relative_path, const std::vector<Path>& root_paths)
        {
            // Attempt all provided root paths, returning the first match with an existing filesystem path
            for (size_t i = 0; i < root_paths.size(); i++)
            {
                Path combined_path = Utilities::CombinePath(root_paths[i], relative_path);
                if (std::filesystem::exists(combined_path))
                    return combined_path;
            }
            return std::optional<Path>(std::nullopt);
        }

        /* Returns the input path as absolute and in lexically normal form.
           If the original path is already in absolute form it is only normalized,
            otherwise the relative path is combined with the given root to create the absolute path. */
        static Path MakeAbsolutePath(const Path& path, const Path& root)
        {
            const std::filesystem::path original_path(TrimString(path));
            return original_path.is_absolute() ?
                original_path.lexically_normal().string() :
                CombinePath(root, path);
        }


        /* Recursively copies the source directory and all of its contents into the target directory path. 
           The copy operation overwrites any existing file or directory in the target path. */
        static std::error_code CopyDirectory(const Path& from, const Path& to)
        {
            const std::filesystem::copy_options options =
                std::filesystem::copy_options::overwrite_existing |
                std::filesystem::copy_options::recursive;

            // Make sure that the target path exists before copying to it
            std::error_code err;
            std::filesystem::create_directories(std::filesystem::path(to), err);
            if (err.value() != 0)
                return err;

            std::filesystem::copy(std::filesystem::path(from), std::filesystem::path(to), options, err);
            return err;
        }

        /* Recursively copies the specified file into the target directory path.
           The copy operation overwrites any existing file with the same name in the target path. */
        static std::error_code CopySingleFile(const Path& from, const Path& to)
        {
            const std::filesystem::copy_options options =
                std::filesystem::copy_options::overwrite_existing;

            // Make sure that the target path exists before copying to it
            std::error_code err;
            std::filesystem::create_directories(std::filesystem::path(to), err);
            if (err.value() != 0)
                return err;

            std::filesystem::copy(std::filesystem::path(from), std::filesystem::path(to), options, err);
            return err;
        }

        /* Copy all of the files that match with the given pattern into the specified target.
           The pattern matching is done according to Unix-style pathname pattern expansion, also known as globbing.
           For more details about the pattern syntax and glob wildcards refer to:
            https://github.com/p-ranav/glob/blob/master/README.md#wildcards
           The copy operation overwrites any existing file with the same name in the target path. */
        static std::error_code CopyMultipleFiles(const String& from_pattern, const Path& to)
        {
            // Make sure that the target path exists before copying to it
            std::error_code err;
            std::filesystem::create_directories(std::filesystem::path(to), err);
            if (err.value() != 0)
                return err;

            const std::filesystem::path from_directory = std::filesystem::path(from_pattern).parent_path();
            const std::string glob_pattern = std::filesystem::path(from_pattern).filename().string();

            for (auto const& dir_entry : std::filesystem::directory_iterator{ from_directory })
            {
                if (glob::fnmatch_case(dir_entry.path(), glob_pattern))
                {
                    if (dir_entry.is_regular_file())
                    {
                        const std::filesystem::path& file_path = dir_entry.path();
                        err = CopySingleFile(file_path.string(), to);
                    }
                    else if (dir_entry.is_directory())
                    {
                        const std::filesystem::path& directory_path = dir_entry.path();
                        const std::filesystem::path& to_directory_path = std::filesystem::path(to) / dir_entry.path().filename();
                        err = CopyDirectory(dir_entry.path().string(), to_directory_path.string());
                    }
                    if (err.value() != 0)
                        return err;
                }
            }

            return err;
        }
	}
}