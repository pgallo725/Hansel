#pragma once

#include <memory> 

#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"


#ifdef _DEBUG
#define HANSEL_LOG_LEVEL	spdlog::level::trace
#else
// Remove 'trace' and 'debug' logs from Release builds
#define HANSEL_LOG_LEVEL	spdlog::level::info
#endif


namespace Hansel
{
	class Logger
	{
	public:

		Logger() = delete;
		~Logger() = delete;

		static void Init();

		inline static std::shared_ptr<spdlog::logger> GetLogger() { return s_HanselLogger; }

		inline static bool IsVerbose()              { return s_Verbose; }
		inline static void SetVerbose(bool verbose) { s_Verbose = verbose; }

		template<typename... Args>
		inline static void Trace(const std::string& fmt, Args &&...args) 
		{ s_HanselLogger->trace("[TRACE] " + fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void Debug(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->debug("[DEBUG] " + fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void Info(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->info("[INFO] " + fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void InfoVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Info(fmt, args); }
		template<typename... Args>
		inline static void Warn(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->warn("[WARNING] " + fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void WarnVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Warn(fmt, args); }
		template<typename... Args>
		inline static void Error(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->error("[ERROR] " + fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void ErrorVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Error(fmt, args); }
		template<typename... Args>
		inline static void Critical(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->critical("[CRITICAL] " + fmt, std::forward<Args>(args)...); }

	private:

		static std::shared_ptr<spdlog::logger> s_HanselLogger;
		static bool s_Verbose;
	};
}
