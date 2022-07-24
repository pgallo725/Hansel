#pragma once

#include <memory> 

#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"


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
#ifdef _DEBUG
		template<typename... Args>
		inline static void Debug(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->debug("[DEBUG] " + fmt, std::forward<Args>(args)...); }
#else	// Remove 'DEBUG' logs from Release builds, but keep 'TRACE' enabled since it's used by the parser
		template<typename... Args>
		inline static void Debug(const std::string& fmt, Args &&...args) {}
#endif
		template<typename... Args>
		inline static void Info(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->info("[INFO] " + fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void InfoVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Info(fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void Warn(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->warn("[WARNING] " + fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void WarnVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Warn(fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void Error(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->error("[ERROR] " + fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void ErrorVerbose(const std::string& fmt, Args &&...args)
		{ if (s_Verbose) Error(fmt, std::forward<Args>(args)...); }
		template<typename... Args>
		inline static void Critical(const std::string& fmt, Args &&...args)
		{ s_HanselLogger->critical("[CRITICAL] " + fmt, std::forward<Args>(args)...); }

	private:

		static std::shared_ptr<spdlog::logger> s_HanselLogger;
		static bool s_Verbose;
	};
}
