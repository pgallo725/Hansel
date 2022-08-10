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
            Command,
            Script
        };

        Type GetType() const { return type; }
        Path GetParentBreadcrumbPath() const { return parent_breadcrumb_path; }

        virtual std::vector<Dependency*> GetDirectDependencies() const = 0;
        virtual std::vector<Dependency*> GetAllDependencies() const = 0;

        virtual bool Realize(bool debug = false, bool verbose = false) const = 0;
        virtual void Print(const std::string& prefix) const = 0;

    protected:

        Dependency(const Path& parent_breadcrumb, Type type)
            : parent_breadcrumb_path(parent_breadcrumb), type(type)
        {};

        Type type;
        Path parent_breadcrumb_path;
    };


    class ProjectDependency : public Dependency
    {
        friend class DependencyChecker;

    private:

        String  name;
        Path    path;
        Path    destination;

        std::vector<Dependency*> dependencies;

    public:

        ProjectDependency(const Path& parent_breadcrumb, const String& name, const Path& path, const Path& destination,
            const std::vector<Dependency*>& dependencies)
            : Dependency(parent_breadcrumb, Type::Project)
            , name(name), path(path), destination(destination), dependencies(dependencies)
        {};

        std::vector<Dependency*> GetDirectDependencies() const override;
        std::vector<Dependency*> GetAllDependencies() const override;

        bool Realize(bool debug = false, bool verbose = false) const override;
        void Print(const std::string& prefix) const override;
    };


    class LibraryDependency : public Dependency
    {
        friend class DependencyChecker;

    private:

        String   name;
        Version  version;
        Path     path;
        Path     destination;

        std::vector<Dependency*> dependencies;

    public:

        LibraryDependency(const Path& parent_breadcrumb, const String& name, const Version& version, const Path& path,
            const Path& destination, const std::vector<Dependency*>& dependencies)
            : Dependency(parent_breadcrumb, Type::Library)
            , name(name), version(version), path(path), destination(destination), dependencies(dependencies)
        {};

        std::vector<Dependency*> GetDirectDependencies() const override;
        std::vector<Dependency*> GetAllDependencies() const override;

        bool Realize(bool debug = false, bool verbose = false) const override;
        void Print(const std::string& prefix) const override;
    };


    class FileDependency : public Dependency
    {
        friend class DependencyChecker;

    private:

        Path  path;
        Path  destination;

    public:

        FileDependency(const Path& parent_breadcrumb, const Path& path, const Path& destination)
            : Dependency(parent_breadcrumb, Type::File)
            , path(path), destination(destination)
        {};

        std::vector<Dependency*> GetDirectDependencies() const override;
        std::vector<Dependency*> GetAllDependencies() const override;

        bool Realize(bool debug = false, bool verbose = false) const override;
        void Print(const std::string& prefix) const override;
    };


    class FilesDependency : public Dependency
    {
        friend class DependencyChecker;

    private:

        Path  path;
        Path  destination;

    public:

        FilesDependency(const Path& parent_breadcrumb, const Path& path, const Path& destination)
            : Dependency(parent_breadcrumb, Type::Files)
            , path(path), destination(destination)
        {};

        std::vector<Dependency*> GetDirectDependencies() const override;
        std::vector<Dependency*> GetAllDependencies() const override;

        bool Realize(bool debug = false, bool verbose = false) const override;
        void Print(const std::string& prefix) const override;
    };


    class DirectoryDependency : public Dependency
    {
        friend class DependencyChecker;

    private:

        Path  path;
        Path  destination;

    public:

        DirectoryDependency(const Path& parent_breadcrumb, const Path& path, const Path& destination)
            : Dependency(parent_breadcrumb, Type::Directory)
            , path(path), destination(destination)
        {};

        std::vector<Dependency*> GetDirectDependencies() const override;
        std::vector<Dependency*> GetAllDependencies() const override;

        bool Realize(bool debug = false, bool verbose = false) const override;
        void Print(const std::string& prefix) const override;
    };


    class CommandDependency : public Dependency
    {
        friend class DependencyChecker;

    private:

        String  code;

    public:

        CommandDependency(const Path& parent_breadcrumb, const String& code)
            : Dependency(parent_breadcrumb, Type::Command)
            , code(code)
        {};

        std::vector<Dependency*> GetDirectDependencies() const override;
        std::vector<Dependency*> GetAllDependencies() const override;

        bool Realize(bool debug = false, bool verbose = false) const override;
        void Print(const std::string& prefix) const override;
    };


    class ScriptDependency : public Dependency
    {
        friend class DependencyChecker;

    private:

        Path    interpreter;
        String  name;
        Path    path;
        String  arguments;

    public:

        ScriptDependency(const Path& parent_breadcrumb, const Path& interpreter, const String& name,
            const Path& path, const String& arguments)
            : Dependency(parent_breadcrumb, Type::Script)
            , interpreter(interpreter), name(name), path(path), arguments(arguments)
        {};

        std::vector<Dependency*> GetDirectDependencies() const override;
        std::vector<Dependency*> GetAllDependencies() const override;

        bool Realize(bool debug = false, bool verbose = false) const override;
        void Print(const std::string& prefix) const override;
    };
}
