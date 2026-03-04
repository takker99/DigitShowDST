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
 * The filtered values are also used by calibration dialogs to capture
 * noise-free readings when the user presses a capture button.
 *
 * Usage:
 *   1. Call display_lpf::filter.set_enabled() / set_cutoff_hz() to configure.
 *   2. Call display_lpf::filter.update(para_raw, dt_s) once per Timer 1 tick.
 *   3. Read display values via filter.vout_filtered(), phyout_filtered(), para_filtered().
 */

#pragma once

#include "Variables.hpp"
#include <array>
#include <cmath>
#include <numbers>

/// @brief Display-only Low-Pass Filter namespace.
namespace display_lpf
{

/**
 * @class DisplayLpf
 * @brief First-order IIR low-pass filter for display and calibration capture.
 *
 * Encapsulates all filter state. Access via the global @c display_lpf::filter instance.
 */
class DisplayLpf
{
  public:
    /// @brief Enable or disable filtering.
    void set_enabled(const bool enabled) noexcept
    {
        enabled_ = enabled;
    }

    /// @brief Set cutoff frequency [Hz]. Values <= 0 act as passthrough.
    void set_cutoff_hz(const double hz) noexcept
    {
        cutoff_hz_ = hz;
    }

    /// @brief Returns true once the filter arrays have been seeded from raw values.
    [[nodiscard]] bool is_initialized() const noexcept
    {
        return initialized_;
    }

    /// @brief Filtered Vout values (one per A/D channel).
    [[nodiscard]] const std::array<double, variables::MAX_AI_CHANNELS> &vout_filtered() const noexcept
    {
        return vout_filtered_;
    }

    /// @brief Filtered Phyout values (one per A/D channel).
    [[nodiscard]] const std::array<double, variables::MAX_AI_CHANNELS> &phyout_filtered() const noexcept
    {
        return phyout_filtered_;
    }

    /// @brief Filtered physical-input parameter values (indices 0..4).
    ///
    /// Index mapping:
    ///   [0] tau (shear stress, kPa)
    ///   [1] shear displacement (mm)
    ///   [2] sigma (vertical stress, kPa)
    ///   [3] normal displacement (mm)
    ///   [4] tilt (mm)
    [[nodiscard]] const std::array<double, 5> &para_filtered() const noexcept
    {
        return para_filtered_;
    }

    /// @brief Update all filtered arrays for one Timer 1 tick.
    ///
    /// When disabled, arrays are kept in sync with raw input values (passthrough)
    /// so that enabling the filter later starts from the current reading rather
    /// than zero-initialized arrays.
    /// When enabled, a first-order IIR filter is applied using the configured cutoff_hz.
    /// On the first enabled call (before any passthrough), arrays are seeded from raw
    /// values to avoid a transient ramp from zero.
    ///
    /// @param para_raw  Raw physical-input parameter values (5 elements: tau, shear_disp,
    ///                  sigma, normal_disp, tilt).
    /// @param dt_s      Timer 1 interval in seconds (typically 0.05).
    void update(const std::array<double, 5> &para_raw, const double dt_s) noexcept
    {
        const auto seed_from_raw = [&]() noexcept {
            for (size_t i = 0; i < variables::MAX_AI_CHANNELS; ++i)
            {
                vout_filtered_[i] = static_cast<double>(variables::Vout[i]);
                phyout_filtered_[i] = variables::Phyout[i];
            }
            for (size_t i = 0; i < para_filtered_.size(); ++i)
                para_filtered_[i] = para_raw[i];
            initialized_ = true;
        };

        if (!enabled_)
        {
            seed_from_raw();
            return;
        }

        if (!initialized_)
        {
            seed_from_raw();
            return;
        }

        const double alpha = compute_alpha(cutoff_hz_, dt_s);
        for (size_t i = 0; i < variables::MAX_AI_CHANNELS; ++i)
        {
            vout_filtered_[i] = alpha * static_cast<double>(variables::Vout[i]) + (1.0 - alpha) * vout_filtered_[i];
            phyout_filtered_[i] = alpha * variables::Phyout[i] + (1.0 - alpha) * phyout_filtered_[i];
        }
        for (size_t i = 0; i < para_filtered_.size(); ++i)
            para_filtered_[i] = alpha * para_raw[i] + (1.0 - alpha) * para_filtered_[i];
    }

  private:
    bool enabled_ = false;
    double cutoff_hz_ = 1.0;
    std::array<double, variables::MAX_AI_CHANNELS> vout_filtered_ = {};
    std::array<double, variables::MAX_AI_CHANNELS> phyout_filtered_ = {};
    std::array<double, 5> para_filtered_ = {};
    bool initialized_ = false;

    /// @brief Compute IIR smoothing factor alpha from cutoff frequency and sample interval.
    /// @return Alpha in (0, 1]: 1 means no filtering (passthrough).
    [[nodiscard]] static double compute_alpha(const double fc, const double dt) noexcept
    {
        if (fc <= 0.0 || dt <= 0.0)
            return 1.0;
        return 1.0 - std::exp(-2.0 * std::numbers::pi * fc * dt);
    }
};

/// @brief Global filter instance. Configure and update this from Timer 1.
inline DisplayLpf filter;

} // namespace display_lpf
