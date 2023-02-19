#include "Log.h"

#pragma warning(push, 0)
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#pragma warning(pop)

std::shared_ptr<spdlog::logger> Log::s_Logger;

void Log::Init()
{
    // Core and User loggers
    {
        std::vector<spdlog::sink_ptr> LogSinks;
        LogSinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        for (spdlog::sink_ptr &Sink : LogSinks)
        {
            Sink->set_pattern("%^[%D %T][%l] %n: %v%$"); // COLOR([MM/DD/YY HH:MM:SS][loglevel] LogName: Message)
        }

        s_Logger = std::make_shared<spdlog::logger>("VKL", LogSinks.begin(), LogSinks.end());
        s_Logger->set_level(spdlog::level::trace);
        s_Logger->flush_on(spdlog::level::trace);
        spdlog::register_logger(s_Logger);
    }

    VKL_INFO("Log successfully initialized!");
}
