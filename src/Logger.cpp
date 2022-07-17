#include "Logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"


namespace Hansel
{
	std::shared_ptr<spdlog::logger> Logger::s_HanselLogger;
	bool Logger::s_Verbose = false;

	void Logger::Init()
	{
		spdlog::set_pattern("%^%v%$");
		spdlog::set_level(HANSEL_LOG_LEVEL);

		s_HanselLogger = spdlog::stdout_color_mt("HANSEL");
	}
}