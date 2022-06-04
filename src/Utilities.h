#pragma once

#include "Types.h"

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
            return std::optional<Path>(std::nullopt);;
        }
	}
}