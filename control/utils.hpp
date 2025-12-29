/**
 * @file control_helpers.hpp
 * @brief Helper functions for control logic patterns
 *
 * This file contains extracted common logic from control patterns to reduce code duplication.
 */

#pragma once

#include "math_constexpr.hpp"

/**
 * @brief Apply tolerance to a value
 *
 * Returns 0 if the absolute value of `value` is less than `tolerance`,
 * otherwise returns the original value. This implements the "dead zone"
 * logic used throughout control patterns.
 *
 * @param value The value to check
 * @param tolerance The tolerance threshold
 * @return 0.0 if |value| < tolerance, otherwise value
 */
inline constexpr auto apply_tolerance(const auto value, const auto tolerance) noexcept
{
    return math_constexpr::abs(value) < tolerance ? 0.0 : value;
}

static_assert(apply_tolerance(0.6, 0.5) == 0.6);
static_assert(apply_tolerance(0.5, 0.5) == 0.5);
static_assert(apply_tolerance(0.4, 0.5) == 0.0);
static_assert(apply_tolerance(0.4, 0) == 0.4);
