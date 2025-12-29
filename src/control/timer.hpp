/**
 * @file control_patterns.hpp
 * @brief Pure function declarations for control patterns
 *
 * This file declares pure functions for all control patterns, replacing
 * the global-state-dependent functions in DigitShowDSTDoc.cpp.
 */

#pragma once

#include "control_output.hpp"
#include "measurement.hpp"
#include <chrono>

/**
 * @brief Parameters for Timer Control
 */
struct TimerControlParams
{
    std::chrono::steady_clock::duration target_duration; // Target duration
    std::chrono::steady_clock::duration elapsed_time;    // Elapsed time
};

constexpr ControlOutput apply_timer_control(const TimerControlParams &params, const ControlOutput &prev_state) noexcept
{
    auto output = prev_state;
    output.step_completed = params.elapsed_time >= params.target_duration && output.step_completed;
    return output;
}