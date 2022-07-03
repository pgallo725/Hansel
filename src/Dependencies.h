#pragma once

#include "Types.h"


namespace Hansel
{
    class Dependency
    {
    public:

        enum class Type
        {
            Project,
            Library,
            File,
            Files,
            Directory,
            Command
        };

        virtual bool Realize() const = 0;
        // TODO: Add Check() function for --check mode (verify correctness and highlight issues)
        virtual void Print(const std::string& prefix) const = 0;

    protected:

        Dependency(Type type) 
            : type(type) 
        {};

        Type type;
    };


    class ProjectDependency : public Dependency
    {
    private:

        String  name;
        Path    path;
        Path    destination;

        std::vector<Dependency*> dependencies;

    public:

        ProjectDependency(const std::string& name, const Path& path, const Path& destination, const std::vector<Dependency*>& dependencies)
            : Dependency(Type::Project)
            , name(name), path(path), destination(destination), dependencies(dependencies)
        {};

        bool Realize() const override;
        void Print(const std::string& prefix) const override;
    };


    class LibraryDependency : public Dependency
    {
    private:

        String   name;
        Version  version;
        Path     path;
        Path     destination;

        std::vector<Dependency*> dependencies;

    public:

        LibraryDependency(const std::string& name, const Version& version, const Path& path, const Path& destination, const std::vector<Dependency*>& dependencies)
            : Dependency(Type::Library)
            , name(name), version(version), path(path), destination(destination), dependencies(dependencies)
        {};

        bool Realize() const override;
        void Print(const std::string& prefix) const override;
    };


    class FileDependency : public Dependency
    {
    private:

        Path  path;
        Path  destination;

    public:

        FileDependency(const Path& path, const Path& destination)
            : Dependency(Type::File)
            , path(path), destination(destination)
        {};

        bool Realize() const override;
        void Print(const std::string& prefix) const override;
    };


    class FilesDependency : public Dependency
    {
    private:

        Path  path;
        Path  destination;

    public:

        FilesDependency(const Path& path, const Path& destination)
            : Dependency(Type::Files)
            , path(path), destination(destination)
        {};

        bool Realize() const override;
        void Print(const std::string& prefix) const override;
    };


    class DirectoryDependency : public Dependency
    {
    private:

        Path  path;
        Path  destination;

    public:

        DirectoryDependency(const Path& path, const Path& destination)
            : Dependency(Type::Directory)
            , path(path), destination(destination)
        {};

        bool Realize() const override;
        void Print(const std::string& prefix) const override;
    };


    class CommandDependency : public Dependency
    {
    private:

        String  name;
        Path    path;
        String  arguments;

    public:

        CommandDependency(const std::string& name, const Path& path, const std::string& arguments)
            : Dependency(Type::Command)
            , name(name), path(path), arguments(arguments)
        {};

        bool Realize() const override;
        void Print(const std::string& prefix) const override;
    };
}
