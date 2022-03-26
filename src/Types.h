#pragma once

#include <compare>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <vector>


namespace Hansel
{
    using String = std::string;
    using Path = std::string;
    using Environment = std::map<std::string, std::string>;


    struct Platform
    {
        enum class OperatingSystem : uint16_t
        {
            Windows = (1 << 0),
            Mac     = (1 << 1),
            Linux   = (1 << 2),

            ANY = 0xFFFF
        };

        enum class Architecture : uint16_t
        {
            x86 = (1 << 0),
            x64 = (1 << 1),

            ANY = 0xFFFF
        };

        enum class Configuration : uint16_t
        {
            Debug   = (1 << 0),
            Release = (1 << 1),

            ANY = 0xFFFF
        };


        OperatingSystem	os;
        Architecture arch;
        Configuration config;
    };


    inline Platform::OperatingSystem operator| (Platform::OperatingSystem a, Platform::OperatingSystem b)
    {
        return static_cast<Platform::OperatingSystem>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    inline Platform::Architecture operator| (Platform::Architecture a, Platform::Architecture b)
    {
        return static_cast<Platform::Architecture>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    inline Platform::Configuration operator| (Platform::Configuration a, Platform::Configuration b)
    {
        return static_cast<Platform::Configuration>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }


    struct Settings
    {
        enum class Mode
        {
            Install,
            Check,
            List
        };

        Mode mode;
        Path target;
        Path output;
        Platform platform;
        Environment variables;
        bool verbose = false;

        // TODO(?): should the target directory and the breadcrumb filename be
        // passed separately in the command line to avoid this ? (probably not)
        inline Path GetTargetDirectoryPath() const
        {
            const std::filesystem::path target_file_path(target);
            return target_file_path.parent_path().string();
        }
    };


    struct Version
    {
        uint32_t major = 0;
        uint32_t minor = 0;
        uint32_t patch = 0;


        Version(uint32_t major, uint32_t minor, uint32_t patch)
            : major(major)
            , minor(minor)
            , patch(patch)
        {}

        Version(uint32_t major, uint32_t minor)
            : Version(major, minor, 0)
        {}


        std::strong_ordering operator<=>(const Version& other)
        {
            if (major > other.major)
                return std::strong_ordering::greater;
            else if (major < other.major)
                return std::strong_ordering::less;

            if (minor > other.minor)
                return std::strong_ordering::greater;
            else if (minor < other.minor)
                return std::strong_ordering::less;

            if (patch > other.patch)
                return std::strong_ordering::greater;
            else if (patch < other.patch)
                return std::strong_ordering::less;

            return std::strong_ordering::equal;
        }

        bool operator==(const Version& other) const = default;
        bool operator!=(const Version& other) const = default;


        std::string ToString() const
        {
            std::stringstream stream;

            // TODO: exclude the 'patch' number if it was left unspecified
            stream << major << '.' << minor << '.' << patch;

            return stream.str();
        }
    };
}
