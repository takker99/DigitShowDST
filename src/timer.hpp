#pragma once

#include <chrono>
#include <cstdint>

namespace timer
{
/** @brief Timer ID for UI refresh (AD/Cal/Show) */
inline constexpr std::uintptr_t kTimerId_UI = 1;

/** @brief Timer ID for closed-loop control tick */
inline constexpr std::uintptr_t kTimerId_Control = 2;

/** @brief Timer ID for periodic logging tick */
inline constexpr std::uintptr_t kTimerId_Log = 3;

// Timer intervals using std::chrono for type safety
inline constexpr std::chrono::milliseconds TimeInterval_1{50};   // Time interval to display output data
inline constexpr std::chrono::milliseconds TimeInterval_2{500};  // Time interval to feed back
inline constinit std::chrono::milliseconds TimeInterval_3{1000}; // Time interval to save the data

} // namespace timer
