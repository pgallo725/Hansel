#pragma once

#include <compare>
#include <concepts>
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

    template<class E>
    concept Enum = std::is_enum<E>::value;


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

        std::string ToString() const
        {
            std::stringstream stream;

            switch (os)
            {
                case OperatingSystem::Windows: stream << "win"; break;
                case OperatingSystem::Linux: stream << "linux"; break;
                case OperatingSystem::Mac: stream << "macosx"; break;
                default: return "undefined";
            }

            switch (arch)
            {
                case Architecture::x86: stream << "32"; break;
                case Architecture::x64: stream << "64"; break;
                default: return "undefined";
            }

            switch (config)
            {
                case Configuration::Debug:   stream << 'd'; break;
                case Configuration::Release: break;
                default: return "undefined";
            }

            return stream.str();
        }
    };


    inline Platform::OperatingSystem operator| (Platform::OperatingSystem a, Platform::OperatingSystem b)
    {
        return static_cast<Platform::OperatingSystem>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    inline Platform::OperatingSystem operator& (Platform::OperatingSystem a, Platform::OperatingSystem b)
    {
        return static_cast<Platform::OperatingSystem>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
    }

    inline Platform::Architecture operator| (Platform::Architecture a, Platform::Architecture b)
    {
        return static_cast<Platform::Architecture>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    inline Platform::Architecture operator& (Platform::Architecture a, Platform::Architecture b)
    {
        return static_cast<Platform::Architecture>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
    }

    inline Platform::Configuration operator| (Platform::Configuration a, Platform::Configuration b)
    {
        return static_cast<Platform::Configuration>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    inline Platform::Configuration operator& (Platform::Configuration a, Platform::Configuration b)
    {
        return static_cast<Platform::Configuration>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
    }


    struct Settings
    {
        enum class Mode
        {
            Help,
            Install,
            Debug,
            Check,
            List
        };

        Mode mode;
        Path target;
        Path output;
        Platform platform;
        Environment variables;
        bool verbose = false;

        inline String GetTargetBreadcrumbFilename() const
        {
            const std::filesystem::path target_file_path = std::filesystem::canonical(target);
            return target_file_path.filename().string();
        }

        inline Path GetTargetDirectoryPath() const
        {
            const std::filesystem::path target_file_path = std::filesystem::canonical(target);
            return target_file_path.parent_path().string();
        }
    };


    struct Version
    {
        static constexpr auto NoValue{ static_cast<uint32_t>(-1) };

        uint32_t major = 0;
        uint32_t minor = 0;
        uint32_t patch = 0;


        Version(uint32_t major, uint32_t minor, uint32_t patch)
            : major(major)
            , minor(minor)
            , patch(patch)
        {}

        Version(uint32_t major, uint32_t minor)
            : Version(major, minor, NoValue)
        {}

        std::strong_ordering operator<=>(const Version& other) const
        {
            if (major > other.major)
                return std::strong_ordering::greater;
            else if (major < other.major)
                return std::strong_ordering::less;

            if (minor > other.minor)
                return std::strong_ordering::greater;
            else if (minor < other.minor)
                return std::strong_ordering::less;

            if (patch == other.patch)
                return std::strong_ordering::equal;
            else if (patch == NoValue)
                if (other.patch == 0)
                    return std::strong_ordering::equivalent;
                else return std::strong_ordering::less;
            else if (other.patch == NoValue)
                if (other.patch == 0)
                    return std::strong_ordering::equivalent;
                else return std::strong_ordering::greater;
            else if (patch > other.patch)
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

            if (patch == NoValue) // exclude the 'patch' number if it was left unspecified
                stream << major << '.' << minor;
            else stream << major << '.' << minor << '.' << patch;

            return stream.str();
        }
    };
}
