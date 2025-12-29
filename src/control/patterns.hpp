/**
 * @file control_patterns.hpp
 * @brief Pure function declarations for control patterns
 *
 * This file declares pure functions for all control patterns, replacing
 * the global-state-dependent functions in DigitShowDSTDoc.cpp.
 */

#pragma once

#include "chrono_alias.hpp"
#include "control_output.hpp"
#include "ep.hpp"
#include "iis_motor.hpp"
#include "math_constexpr.hpp"
#include "measurement.hpp"
#include <chrono>
#include <unordered_map>

/**
 * @brief Control pattern identifiers
 *
 * This enum class represents all available control patterns in the system.
 * Each pattern corresponds to a specific testing mode (loading, creep, relaxation, etc.).
 * Values map 1:1 to legacy CFNUM codes for backward compatibility.
 */
enum class ControlPattern : size_t
{
    NoControl = 0,
    MonotonicLoadingConstantPressure = 1,
    MonotonicLoadingConstantVolume = 2,
    CyclicLoadingConstantPressure = 3,
    CyclicLoadingConstantVolume = 4,
    CreepConstantPressure = 5,
    CreepConstantVolume = 6,
    RelaxationConstantPressure = 7,
    RelaxationConstantVolume = 8,
    MonotonicLoadingDisplacementConstantPressure = 9,
    MonotonicLoadingDisplacementConstantVolume = 10,
    CyclicLoadingDisplacementConstantPressure = 11,
    CyclicLoadingDisplacementConstantVolume = 12,
    AccelerationConstantPressure = 13,
    AccelerationConstantVolume = 14,
    ConstantTauConsolidation = 15,
    KConsolidation = 16,
    CreepConstantPressureFast = 17,
    CreepConstantPressureFastRef = 18,
    PreConsolidation = 19,
    BeforeConsolidation = 20,
    AfterConsolidation = 21
};

// Map of known patterns to enum values
// Note: "rebase_reference" is the preferred name for the rebase operation (maps to CFNUM 20).
// "before_consolidation" (CFNUM 20) and "after_consolidation" (CFNUM 21) are kept as aliases
// for backward compatibility. Both execute identical code but maintain separate enum values
// for historical reasons. When outputting JSON, get_use_label() returns "rebase_reference" for both.
inline const std::unordered_map<std::string_view, ControlPattern> use_map = {
    {"rebase_reference", ControlPattern::BeforeConsolidation}, // Preferred name (CFNUM 20)
    {"no_control", ControlPattern::NoControl},
    {"monotonic_loading_constant_pressure", ControlPattern::MonotonicLoadingConstantPressure},
    {"monotonic_loading_constant_volume", ControlPattern::MonotonicLoadingConstantVolume},
    {"cyclic_loading_constant_pressure", ControlPattern::CyclicLoadingConstantPressure},
    {"cyclic_loading_constant_volume", ControlPattern::CyclicLoadingConstantVolume},
    {"creep_constant_pressure", ControlPattern::CreepConstantPressure},
    {"creep_constant_volume", ControlPattern::CreepConstantVolume},
    {"relaxation_constant_pressure", ControlPattern::RelaxationConstantPressure},
    {"relaxation_constant_volume", ControlPattern::RelaxationConstantVolume},
    {"monotonic_loading_displacement_constant_pressure", ControlPattern::MonotonicLoadingDisplacementConstantPressure},
    {"monotonic_loading_displacement_constant_volume", ControlPattern::MonotonicLoadingDisplacementConstantVolume},
    {"cyclic_loading_displacement_constant_pressure", ControlPattern::CyclicLoadingDisplacementConstantPressure},
    {"cyclic_loading_displacement_constant_volume", ControlPattern::CyclicLoadingDisplacementConstantVolume},
    {"acceleration_constant_pressure", ControlPattern::AccelerationConstantPressure},
    {"acceleration_constant_volume", ControlPattern::AccelerationConstantVolume},
    {"constant_tau_consolidation", ControlPattern::ConstantTauConsolidation},
    {"k_consolidation", ControlPattern::KConsolidation},
    {"creep_constant_pressure_fast", ControlPattern::CreepConstantPressureFast},
    {"creep_constant_pressure_fast_ref", ControlPattern::CreepConstantPressureFastRef},
    {"pre_consolidation", ControlPattern::PreConsolidation},
    {"before_consolidation", ControlPattern::BeforeConsolidation}, // Alias for backward compatibility
    {"after_consolidation",
     ControlPattern::AfterConsolidation}}; // Alias for backward compatibility (different enum but same behavior)

// Get use label from ControlPattern enum
inline std::string_view get_use_label(ControlPattern pattern) noexcept
{
    // Explicitly return preferred name for consolidated patterns
    if (pattern == ControlPattern::BeforeConsolidation || pattern == ControlPattern::AfterConsolidation)
    {
        return "rebase_reference";
    }

    for (const auto &[label, pat] : use_map)
    {
        if (pat == pattern)
            return label;
    }
    return "unknown";
}

/**
 * @brief Parameters for Constant Tau Consolidation (CFNUM=15)
 */
struct ConstantTauConsolidationParams
{
    bool direction{};                    // [0] Compression or Dilation: true=compression, false=dilation
    double consolidation_rate{};         // [3] Consolidation rate (kPa/min)
    double target_sigma{};               // [4] Target stress after consolidation (kPa)
    double initial_sigma{};              // Initial confining pressure (kPa)
    std::chrono::minutes_d elapsed_time; // TotalStepTime

    // Sensitivity parameters
    double err_stress_p = 0.5;
    double err_disp = 0.004;
    double amp = 0.5;
    double amp2_f = 0.5;
    double amp2_r = 0.5;
    double dmax = 1.2;
};

/**
 * @brief Consolidation under constant stress ratio (constant tau)
 * @param params Input parameters
 * @param prev_state Previous control state
 * @return Updated control output
 */
constexpr ControlOutput Constant_Tau_Consolidation(const ConstantTauConsolidationParams &params,
                                                   const MotorTauControlParams &motor_params,
                                                   const control::PhysicalInput &input,
                                                   const ControlOutput &prev_state) noexcept
{
    auto output = prev_state;

    using math_constexpr::abs;

    // Check if target sigma reached
    if (abs(input.vertical_stress_kpa() - params.target_sigma) < params.err_stress_p)
    {
        output.step_completed = true;
        return output;
    }

    const auto sigma_rate = params.direction ? abs(params.consolidation_rate) : -abs(params.consolidation_rate);

    // target sigma rateを加味したsigmaの目標値を計算する
    // step開始からparams.consolidation_rateで変化したと仮定した場合のsigmaを目標値とする
    const auto target_sigma_at_the_step = params.initial_sigma + sigma_rate * params.elapsed_time.count();

    return apply_motor_tau_control(motor_params, input,
                                   // 定圧制御を転用し、目標値を変動させることで応力速度制御を実現する
                                   apply_ep_constant_pressure_control({.target_sigma = target_sigma_at_the_step,
                                                                       .err_stress = params.err_stress_p,
                                                                       .err_disp = params.err_disp,
                                                                       .amp = params.amp,
                                                                       .amp2_f = params.amp2_f,
                                                                       .amp2_r = params.amp2_r,
                                                                       .dmax = params.dmax},
                                                                      input, prev_state));
}

/**
 * @brief Parameters for K-consolidation (Linear stress path) (CFNUM=16)
 */
struct PathMLoadingConstantPressureParams
{
    bool loading_dir{};   // Loading direction: true=loading, false=unloading
    double target_tau{};  // [2] Target tau (kPa)
    double sigma_start{}; // [3] Initial sigma (kPa)
    double sigma_end{};   // [4] Final sigma (kPa)
    double k_value{};     // [5] K value (stress path slope) TODO: 計算中で使っていないバグあり、後日直す

    // Sensitivity parameters
    double err_stress_p = 0.5;
    double err_disp = 0.004;
    double amp = 0.5;
    double amp2_f = 0.5;
    double amp2_r = 0.5;
    double dmax = 1.2;
};

/**
 * @brief Linear stress path loading (K-consolidation)
 * @param params Input parameters
 * @param prev_state Previous control state
 * @return Updated control output
 */
constexpr ControlOutput Path_MLoading_Constant_Pressure(const PathMLoadingConstantPressureParams &params,
                                                        const MonotonicMotorControlParams &motor_params,
                                                        const control::PhysicalInput &input,
                                                        const ControlOutput &prev_state) noexcept
{
    const auto tau_progress = input.shear_stress_kpa() / params.target_tau;
    const auto target_sigma_at_the_step = params.sigma_start + (params.sigma_end - params.sigma_start) * tau_progress;

    return apply_monotonic_motor_control(motor_params, input,
                                         apply_ep_constant_pressure_control({.target_sigma = target_sigma_at_the_step,
                                                                             .err_stress = params.err_stress_p,
                                                                             .err_disp = params.err_disp,
                                                                             .amp = params.amp,
                                                                             .amp2_f = params.amp2_f,
                                                                             .amp2_r = params.amp2_r,
                                                                             .dmax = params.dmax},
                                                                            input, prev_state));
}
