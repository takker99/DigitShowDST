/**
 * @file control/params.hpp
 * @brief Named parameter structure for control steps
 *
 * This file defines ControlParams, which replaces the legacy CFPARA array-based
 * representation with named fields for better maintainability and type safety.
 * Conversion functions provide backward compatibility with existing code.
 */

#pragma once

#include "chrono_alias.hpp"
#include "patterns.hpp"
#include <array>
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
 * - loading_dir: CFPARA[0] - Direction (0.0=load/compression, 1.0=unload/dilation)
 * - motor_rpm: CFPARA[1] - Motor speed in RPM (or CFPARA[0] for some creep modes)
 * - target_tau_kpa: CFPARA[2] - Target shear stress (kPa)
 * - target_sigma_kpa: CFPARA[3] - Target confining pressure (kPa)
 * - duration: CFPARA[2] or CFPARA[0] - Duration for creep/relaxation (minutes, as minutes_d)
 * - cycles: CFPARA[4] - Number of cycles for cyclic loading
 * - Sensitivity overrides: CFPARA[10..17]
 */
struct ControlParams
{
    // Control pattern identifier
    ControlPattern pattern = ControlPattern::NoControl;
    // Primary control parameters (CFPARA[0..9])
    bool loading_dir = true;              // [0] Direction: true=load/compression, false=unload/dilation
    double motor_rpm = 0.0;               // [1] Motor speed (RPM), or [0] for creep patterns
    double target_tau_kpa = 0.0;          // [2] Target shear stress (kPa)
    double target_sigma_kpa = 0.0;        // [3] Target confining pressure (kPa)
    std::chrono::minutes_d duration{0.0}; // [2] or [0] Duration for time-based patterns (minutes)
    size_t cycles = 0;                    // [4] Number of cycles for cyclic loading

    // Additional pattern-specific parameters
    double tau_lower_kpa = 0.0;                  // [2] Lower tau for cyclic patterns
    double tau_upper_kpa = 0.0;                  // [3] Upper tau for cyclic patterns
    double target_displacement_mm = 0.0;         // [2] Target displacement for displacement-based patterns
    double displacement_lower_mm = 0.0;          // [2] Lower displacement for cyclic displacement patterns
    double displacement_upper_mm = 0.0;          // [3] Upper displacement for cyclic displacement patterns
    double acceleration_rate_rpm_per_min = 0.0;  // [2] Acceleration rate
    double consolidation_rate_kpa_per_min = 0.0; // [3] Consolidation rate
    double tau_start_kpa = 0.0;                  // [2] Starting tau for k_consolidation
    double tau_end_kpa = 0.0;                    // [3] Ending tau for k_consolidation
    double sigma_start_kpa = 0.0;                // [4] Starting sigma for k_consolidation
    double k_value = 0.0;                        // [5] K value for k_consolidation

    // Sensitivity overrides (CFPARA[10..17])
    double err_stress_kpa = 0.0;   // [10] Stress tolerance (kPa)
    double err_disp_mm = 0.0;      // [11] Displacement balance tolerance (mm)
    double amp_v_per_kpa_m2 = 0.0; // [12] Stress loop gain (V/(kPa·m²))
    double amp2_f_v_per_mm = 0.0;  // [13] Front displacement gain (V/mm)
    double amp2_r_v_per_mm = 0.0;  // [14] Rear displacement gain (V/mm)
    double dmax_v = 0.0;           // [15] EP slew limit (V)
    double err_disp_cv_mm = 0.0;   // [16] CV displacement tolerance (mm)
    double amp_cv_v_per_mm = 0.0;  // [17] CV displacement gain (V/mm)

    // Name field for step identification
    std::string name;
};

} // namespace control
