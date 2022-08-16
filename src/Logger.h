#pragma once

#include <string>
#include <iostream>
#include <format>


namespace Hansel
{
	class Logger
	{
	public:

		Logger() = delete;
		~Logger() = delete;

		inline static bool IsVerbose()              { return s_Verbose; }
		inline static void SetVerbose(bool verbose) { s_Verbose = verbose; }

		template<typename... Args>
		inline static void Trace(const std::string& fmt, Args &&...args) 
		{ std::cout << "[TRACE] " << std::vformat(fmt, std::make_format_args(args...)) << '\n'; }
		template<typename... Args>
		inline static void TraceVerbose(const std::string& fmt, Args &&...args) 
		{ if (s_Verbose) Trace(fmt, std::forward<Args>(args)...); }

#ifdef _DEBUG
		template<typename... Args>
		inline static void Debug(const std::string& fmt, Args &&...args)
		{ std::cout << "[DEBUG] " << std::vformat(fmt, std::make_format_args(args...)) << '\n'; }
#else	// Remove 'DEBUG' logs from Release builds, but keep 'TRACE' enabled since it's used by the parser
		template<typename... Args>
		inline static void Debug(const std::string& fmt, Args &&...args) {}
#endif

		template<typename... Args>
		inline static void Info(const std::string& fmt, Args &&...args)
		{ std::cout << "[INFO] " << std::vformat(fmt, std::make_format_args(args...)) << '\n'; }
		template<typename... Args>
		inline static void InfoVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Info(fmt, std::forward<Args>(args)...); }

		template<typename... Args>
		inline static void Warn(const std::string& fmt, Args &&...args)
		{ std::cout << "[WARNING] " << std::vformat(fmt, std::make_format_args(args...)) << '\n'; }
		template<typename... Args>
		inline static void WarnVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Warn(fmt, std::forward<Args>(args)...); }

		template<typename... Args>
		inline static void Error(const std::string& fmt, Args &&...args)
		{ std::cout << "[ERROR] " << std::vformat(fmt, std::make_format_args(args...)) << '\n'; }
		template<typename... Args>
		inline static void ErrorVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Error(fmt, std::forward<Args>(args)...); }

		template<typename... Args>
		inline static void Critical(const std::string& fmt, Args &&...args)
		{ std::cout << "[CRITICAL] " << std::vformat(fmt, std::make_format_args(args...)) << '\n'; }

	private:

		inline static bool s_Verbose = false;
	};
}
