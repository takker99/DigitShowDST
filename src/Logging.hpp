#pragma once

/**
 * @file Logging.hpp
 * @brief spdlog initialization for DigitShowBasic
 *
 * Provides initialization function for the global spdlog logger.
 * Use spdlog::* functions directly instead of macros.
 */

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace logging
{

/**
 * @brief Initialize spdlog with rotating file and console sinks
 *
 * Sets up logging with:
 * - Console output (stdout with colors)
 * - Rotating log file: logs/digitshowbasic.log
 * - Max file size: 10 MiB per file
 * - Max files: 5 rotating files
 * - Default level: debug
 * - Pattern: [timestamp] [level] message
 */
inline void initialize() noexcept
{
    try
    {
        // Create sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/digitshowbasic.log",
                                                                                10 * 1024 * 1024, // 10MB
                                                                                5);               // 5 files
        file_sink->set_level(spdlog::level::trace);

        // Combine sinks
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("digitshowbasic", sinks.begin(), sinks.end());

        // Set pattern and level
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);

        // Register as default logger
        spdlog::set_default_logger(logger);

        spdlog::info("Logging initialized");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        // If initialization fails, spdlog will use its default logger
        spdlog::error("Log initialization failed: {}", ex.what());
    }
}

} // namespace logging
