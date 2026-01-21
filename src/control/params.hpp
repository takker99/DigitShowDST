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
 * @file control/params.hpp
 * @brief Named parameter structure for control steps
 *
 * This file defines ControlParams, which replaces the legacy CFPARA array-based
 * representation with named fields for better maintainability and type safety.
 * Conversion functions provide backward compatibility with existing code.
 */

#pragma once

#include "../chrono_alias.hpp"
#include "patterns.hpp"
#include <string>

namespace control
{

/**
 * @brief Named parameters for a control step
 *
 * This structure represents all control parameters with meaningful names
 * instead of array indices. Fields correspond to the CFPARA[0..17] array
 * as documented in knowledge/control_specifications.md.
 *
 * Usage across different control patterns:
 * - motor_rpm: CFPARA[0] - Motor speed in RPM with sign indicating direction: positive=load/compression,
 * negative=unload/dilation
 * - target_tau_kpa: CFPARA[1] - Target shear stress (kPa)
 * - target_sigma_kpa: CFPARA[2] - Target confining pressure (kPa)
 * - duration: CFPARA[3] or CFPARA[0] - Duration for creep/relaxation (minutes, as minutes_d)
 * - cycles: CFPARA[4] - Number of cycles for cyclic loading
 * - Sensitivity overrides: CFPARA[10..17]
 */
struct ControlParams
{
    // Control pattern identifier
    ControlPattern pattern = ControlPattern::NoControl;

    // Primary control parameters (CFPARA[0..9])
    std::chrono::minutes_d duration{0.0}; // [2] or [0] Duration for time-based patterns (minutes)

    // Additional pattern-specific parameters
    double consolidation_rate_kpa_per_min = 0.0; // [3] Consolidation rate

    struct MonotonicLoading
    {
        double motor_rpm = 0.0;              // [0] Motor speed in RPM
        double target_tau_kpa = 0.0;         // [1] Target shear stress (kPa)
        double target_displacement_mm = 0.0; // [1] Target displacement (mm)
    } monotonic_loading;

    /**
     * @brief Parameters for cyclic motor control (stress-based)
     */
    struct CyclicMotorControlParams
    {
        double motor_rpm = 0.0; // Motor speed in RPM, sign indicates direction: positive=loading, negative=unloading
        double tau_lower_kpa = 0.0;         // [2] Tau lower bound for cyclic patterns
        double tau_upper_kpa = 0.0;         // [3] Tau upper bound for cyclic patterns
        double displacement_lower_mm = 0.0; // [2] Displacement lower bound for cyclic displacement patterns
        double displacement_upper_mm = 0.0; // [3] Displacement upper bound for cyclic displacement patterns
        size_t cycles;                      // Number of cycles
    } cyclic_loading;

    /**
     * @brief Parameters for motor acceleration control
     */
    struct MotorAccelerationParams
    {
        double start_rpm;                         // Starting RPM
        double target_rpm;                        // Target RPM
        double acceleration;                      // Acceleration rate (RPM/min), sign indicates direction
        double err_rpm = 1.0;                     // RPM error tolerance
        std::chrono::minutes_d time_interval_min; // Time interval in minutes
    } motor_acceleration;

    /**
     * @brief Parameters for K-consolidation (Linear stress path) (CFNUM=16)
     */
    struct PathMLoadingParams
    {
        double target_tau{};  // [2] Target tau (kPa)
        double sigma_start{}; // [3] Initial sigma (kPa)
        double sigma_end{};   // [4] Final sigma (kPa)
        double k_value{};     // [5] K value (stress path slope) TODO: 計算中で使っていないバグあり、後日直す
        double motor_rpm{};   // [6] Motor speed (RPM)
    } path_mloading;

    struct VerticalStress
    {
        double setpoint = 0.0;                      // [10] Stress setpoint (kPa)
        double error = DEFAULT_ERROR;               // [11] Stress error (kPa)
        double kp = DEFAULT_KP;                     // Proportional gain
        std::chrono::seconds_d ti = DEFAULT_TI;     // Integral time
        double cv_limit_kpa = DEFAULT_CV_LIMIT_KPA; // EP output (control variable) limit a tick (kPa)

        // Default parameter values
        static constexpr double DEFAULT_ERROR = 0.5;
        static constexpr double DEFAULT_KP = 0.5;
        static constexpr auto DEFAULT_TI = std::chrono::seconds_d{0.1};
        static constexpr double DEFAULT_CV_LIMIT_KPA = 1.2;
    } vertical_stress_kpa;

    struct ShearStress
    {
        double setpoint = 0.0;
        double error = DEFAULT_ERROR;
        double kp_rpm_per_kpa = DEFAULT_KP;
        double cv_limit_rpm = DEFAULT_CV_LIMIT_RPM; // Motor output (control variable) limit a tick (RPM)

        // Default parameter values
        static constexpr double DEFAULT_ERROR = 0.5;
        static constexpr double DEFAULT_KP = 0.5;
        static constexpr double DEFAULT_CV_LIMIT_RPM = 3000.0;
    } shear_stress_kpa;

    struct NormalDisplacement
    {
        double setpoint = 0.0; // [2] Target displacement for displacement-based patterns
        double error = DEFAULT_ERROR;
        double ki_kpa_per_mm = DEFAULT_KI;
        double cv_limit_kpa = DEFAULT_CV_LIMIT_KPA; // EP output (control variable) limit a tick (kPa)

        // Default parameter values
        static constexpr double DEFAULT_ERROR = 0.002;
        static constexpr double DEFAULT_KI = 17.0;
        static constexpr double DEFAULT_CV_LIMIT_KPA = 1.2;
    } normal_displacement_mm;

    struct Tilt
    {
        double setpoint = 0.0;
        double error = DEFAULT_ERROR;
        double ki_kpa_per_mm = DEFAULT_KI;
        double cv_limit_kpa = DEFAULT_CV_LIMIT_KPA; // EP output (control variable) limit a tick (kPa)

        // Default parameter values
        static constexpr double DEFAULT_ERROR = 0.002;
        static constexpr double DEFAULT_KI = 50;
        static constexpr double DEFAULT_CV_LIMIT_KPA = 1.2;
    } tilt_mm;

    // Name field for step identification
    std::string name;
};

} // namespace control
