/*
 * DigitShowDST - Direct Shear Test Machine Control Software
 * Copyright (C) 2025 Takuto ISHII
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

/**
 * @file lpf.hpp
 * @brief Display-only Low-Pass Filter (LPF) for the main screen.
 *
 * Applies a first-order IIR (exponential moving average) filter to the
 * voltage input, physical input, and physical input parameters shown on
 * the main screen. This filter is for display purposes only and does not
 * affect control, data logging, or API output.
 *
 * Usage:
 *   1. Call display_lpf::update(para_raw, dt_s) once per Timer 1 tick.
 *   2. Read display values from vout_filtered, phyout_filtered, para_filtered.
 */

#pragma once

#include "Variables.hpp"
#include <array>
#include <cmath>
#include <numbers>

/// @brief Display-only Low-Pass Filter namespace.
namespace display_lpf
{

/// @brief LPF enabled flag.
inline bool enabled = false;

/// @brief Cutoff frequency in Hz (must be >= 0; 0 disables filtering).
inline double cutoff_hz = 1.0;

/// @brief Filtered Vout values (one per A/D channel), updated by update().
inline std::array<double, variables::MAX_AI_CHANNELS> vout_filtered = {};

/// @brief Filtered Phyout values (one per A/D channel), updated by update().
inline std::array<double, variables::MAX_AI_CHANNELS> phyout_filtered = {};

/// @brief Filtered physical-input parameter values (indices 0..4), updated by update().
///
/// Index mapping:
///   [0] tau (shear stress, kPa)
///   [1] shear displacement (mm)
///   [2] sigma (vertical stress, kPa)
///   [3] normal displacement (mm)
///   [4] tilt (mm)
inline std::array<double, 5> para_filtered = {};

/// @brief True once the filter arrays have been seeded from raw values at least once.
inline bool initialized = false;

/// @brief Compute the IIR smoothing factor alpha from cutoff frequency and sample interval.
/// @param fc Cutoff frequency [Hz]; must be > 0.
/// @param dt Sample interval [s]; must be > 0.
/// @return Alpha in (0, 1]: 1 means no filtering (passthrough).
[[nodiscard]] inline double compute_alpha(const double fc, const double dt) noexcept
{
    if (fc <= 0.0 || dt <= 0.0)
        return 1.0;
    return 1.0 - std::exp(-2.0 * std::numbers::pi * fc * dt);
}

/// @brief Update all filtered arrays for one Timer 1 tick.
///
/// When @p enabled is false, filtered arrays are seeded from raw input values
/// (passthrough) so that enabling the filter later starts from the current
/// reading rather than zero-initialized arrays.
/// When enabled, a first-order IIR filter is applied using the configured cutoff_hz.
/// On the first enabled call (before any passthrough), the arrays are seeded from
/// raw values to avoid a transient ramp from zero.
///
/// @param para_raw  Raw physical-input parameter values (5 elements: tau, shear_disp,
///                  sigma, normal_disp, tilt).
/// @param dt_s      Timer 1 interval in seconds (typically 0.05).
inline void update(const std::array<double, 5> &para_raw, const double dt_s) noexcept
{
    // Helper: copy raw values into the filter state arrays.
    const auto seed_from_raw = [&]() noexcept {
        for (size_t i = 0; i < variables::MAX_AI_CHANNELS; ++i)
        {
            vout_filtered[i] = static_cast<double>(variables::Vout[i]);
            phyout_filtered[i] = variables::Phyout[i];
        }
        for (size_t i = 0; i < para_filtered.size(); ++i)
            para_filtered[i] = para_raw[i];
        initialized = true;
    };

    if (!enabled)
    {
        seed_from_raw();
        return;
    }

    // On the first enabled call, seed from raw to avoid a ramp from zero.
    if (!initialized)
    {
        seed_from_raw();
        return;
    }

    const double alpha = compute_alpha(cutoff_hz, dt_s);
    for (size_t i = 0; i < variables::MAX_AI_CHANNELS; ++i)
    {
        vout_filtered[i] = alpha * static_cast<double>(variables::Vout[i]) + (1.0 - alpha) * vout_filtered[i];
        phyout_filtered[i] = alpha * variables::Phyout[i] + (1.0 - alpha) * phyout_filtered[i];
    }
    for (size_t i = 0; i < para_filtered.size(); ++i)
        para_filtered[i] = alpha * para_raw[i] + (1.0 - alpha) * para_filtered[i];
}

} // namespace display_lpf
