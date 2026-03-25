/*
 * DigitShowBasic - Triaxial Test Machine Control Software
 * Copyright (C) 2025 Makoto KUNO
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace logging
{
inline constexpr auto LOGROTATE_MAX_SIZE = 2 * 1024 * 1024; // 2 MiB
inline constexpr auto LOGROTATE_MAX_FILES = 128;

inline spdlog::level::level_enum default_log_level() noexcept
{
#ifdef _DEBUG
    return spdlog::level::debug;
#else
    return spdlog::level::info;
#endif
}

inline bool try_parse_log_level(std::string value, spdlog::level::level_enum &level) noexcept
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (value == "trace")
    {
        level = spdlog::level::trace;
        return true;
    }
    if (value == "debug")
    {
        level = spdlog::level::debug;
        return true;
    }
    if (value == "info")
    {
        level = spdlog::level::info;
        return true;
    }
    if (value == "warn" || value == "warning")
    {
        level = spdlog::level::warn;
        return true;
    }
    if (value == "error")
    {
        level = spdlog::level::err;
        return true;
    }
    if (value == "critical")
    {
        level = spdlog::level::critical;
        return true;
    }
    if (value == "off")
    {
        level = spdlog::level::off;
        return true;
    }

    return false;
}

inline void apply_log_level_from_config(const std::filesystem::path &config_path) noexcept
{
    const auto fallback = default_log_level();

    if (!std::filesystem::exists(config_path))
    {
        try
        {
            nlohmann::json initial_config = {{"log_level", spdlog::level::to_string_view(fallback).data()}};
            std::ofstream ofs(config_path, std::ios::trunc);
            if (ofs)
            {
                ofs << initial_config.dump(2) << '\n';
                spdlog::info("Config file created: {}", config_path.string());
            }
            else
            {
                spdlog::warn("Failed to create config file: {}", config_path.string());
            }
        }
        catch (const std::exception &ex)
        {
            spdlog::warn("Failed to create config file {}: {}", config_path.string(), ex.what());
        }

        spdlog::set_level(fallback);
        spdlog::info("Config file not found: {} (using default log level: {})", config_path.string(),
                     spdlog::level::to_string_view(fallback));
        return;
    }

    try
    {
        std::ifstream ifs(config_path);
        if (!ifs)
        {
            spdlog::set_level(fallback);
            spdlog::warn("Failed to open config file: {} (using default log level: {})", config_path.string(),
                         spdlog::level::to_string_view(fallback));
            return;
        }

        nlohmann::json config;
        ifs >> config;

        if (!config.is_object())
        {
            spdlog::set_level(fallback);
            spdlog::warn("Config file is not a JSON object: {} (using default log level: {})", config_path.string(),
                         spdlog::level::to_string_view(fallback));
            return;
        }

        auto it = config.find("log_level");
        if (it == config.end())
        {
            spdlog::set_level(fallback);
            spdlog::info("log_level not found in {} (using default: {})", config_path.string(),
                         spdlog::level::to_string_view(fallback));
            return;
        }

        if (!it->is_string())
        {
            spdlog::set_level(fallback);
            spdlog::warn("log_level must be a string in {} (using default: {})", config_path.string(),
                         spdlog::level::to_string_view(fallback));
            return;
        }

        const auto level_str = it->get<std::string>();
        spdlog::level::level_enum parsed_level{};
        if (!try_parse_log_level(level_str, parsed_level))
        {
            spdlog::set_level(fallback);
            spdlog::warn("Invalid log_level '{}' in {} (using default: {})", level_str, config_path.string(),
                         spdlog::level::to_string_view(fallback));
            return;
        }

        spdlog::set_level(parsed_level);
        spdlog::info("Log level set from {}: {}", config_path.string(), level_str);
    }
    catch (const std::exception &ex)
    {
        spdlog::set_level(fallback);
        spdlog::warn("Failed to parse {}: {} (using default: {})", config_path.string(), ex.what(),
                     spdlog::level::to_string_view(fallback));
    }
}

inline void initialize() noexcept
{
    try
    {
        std::filesystem::create_directories("logs");

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/digitshowbasic.log", LOGROTATE_MAX_SIZE, LOGROTATE_MAX_FILES);

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("digitshowbasic", sinks.begin(), sinks.end());

        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_default_logger(logger);

        apply_log_level_from_config(std::filesystem::current_path() / "dsb.json");

        spdlog::flush_every(std::chrono::seconds{1});
        spdlog::info("Logging initialized");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        spdlog::error("Log initialization failed: {}", ex.what());
    }
}

} // namespace logging
