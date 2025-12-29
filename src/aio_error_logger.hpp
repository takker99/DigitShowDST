/**
 * @file aio_error_logger.hpp
 * @brief Error logging utilities for CONTEC AIO operations using spdlog
 *
 * Provides consistent formatting for AIO error logging without tight coupling
 * to the wrapper implementation.
 */

#pragma once

#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace aio
{

/**
 * @brief Log an AIO error with context
 * @param context Description of the operation that failed
 * @param error_message Error message from AIO operation
 */
inline void log_error(const std::string_view context, const std::string &error_message) noexcept
{
    spdlog::error("AIO [{}]: {}", context, error_message);
}

/**
 * @brief Log an AIO warning with context
 * @param context Description of the operation
 * @param warning_message Warning message
 */
inline void log_warning(const std::string_view context, const std::string &warning_message) noexcept
{
    spdlog::warn("AIO [{}]: {}", context, warning_message);
}

/**
 * @brief Log an AIO info message with context
 * @param context Description of the operation
 * @param info_message Info message
 */
inline void log_info(const std::string_view context, const std::string &info_message) noexcept
{
    spdlog::info("AIO [{}]: {}", context, info_message);
}

} // namespace aio
