#include <Config/Config.hpp>
#include <Debug/Logger.hpp>
#include <System/Loaders.hpp>
#include <Utils/FileUtils.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#if defined(_WIN32) || defined(_WIN64)
#include <spdlog/sinks/wincolor_sink.h>
#elif defined(__ANDROID__)
#include <spdlog/sinks/android_sink.h>
#else
#include <spdlog/sinks/ansicolor_sink.h>
#endif

namespace obe::Debug
{
    std::shared_ptr<spdlog::logger> Log;
    void InitLogger()
    {
        Utils::File::deleteFile("debug.log");
        auto dist_sink = std::make_shared<spdlog::sinks::dist_sink_st>();
#if defined(_WIN32) || defined(_WIN64)
        const auto sink1 = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#elif defined(__ANDROID__)
        const auto sink1
            = std::make_shared<spdlog::sinks::android_sink_mt>("obengineplayer");
#else
        auto sink1 = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
        const auto sink2
            = std::make_shared<spdlog::sinks::basic_file_sink_st>("debug.log");

        dist_sink->add_sink(sink1);
        dist_sink->add_sink(sink2);
        Log = std::make_shared<spdlog::logger>("Log", dist_sink);
        Log->set_pattern("[%H:%M:%S.%e]<%l> : %v");
        Log->set_level(spdlog::level::info);
        Log->flush_on(spdlog::level::trace);
        Log->info("Logger initialised");
    }
} // namespace obe::Debug
