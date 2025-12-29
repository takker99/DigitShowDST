#pragma once

#include "chrono_alias.hpp"
#include "control_output.hpp"
#include "math_constexpr.hpp"
#include "measurement.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

// Constants used across multiple patterns
static constexpr double MinStroke = -5.0; // Minimum stroke limit (mm)
static constexpr double MaxStroke = 15.0; // Maximum stroke limit (mm)

/**
 * @brief Convert loading direction to sign value
 * @param loading_dir Loading direction: true=loading (positive), false=unloading (negative)
 * @return 1.0 if loading_dir is true, -1.0 otherwise
 */
inline constexpr double direction_sign(const bool loading_dir) noexcept
{
    return loading_dir ? 1.0 : -1.0;
}

/**
 * @brief Parameters for motor control with tau setpoint
 */
struct MotorTauControlParams
{
    double target_tau; // Target deviator stress (kPa)
    double err_stress; // Stress error tolerance (kPa)
    double max_rpm;    // Motor speed (RPM)
    bool loading_dir;  // Loading direction: true=loading, false=unloading
};

/**
 * @brief Apply motor control with tau as setpoint (P-control on deviator stress)
 *
 * Controls motor clutch based on tau (deviator stress):
 * - If tau > target + ERR: Unload (clutch = 5.0f)
 * - If tau < target - ERR: Load (clutch = 0.0f)
 * - Otherwise: Stop motor (rpm = 0)
 *
 * @param params Control parameters
 * @param davout_motor Output: motor on/off voltage
 * @param davout_motor_speed Output: motor speed voltage
 * @param davout_motor_cruch Output: motor clutch voltage
 */
inline constexpr ControlOutput apply_motor_tau_control(const MotorTauControlParams &params,
                                                       const control::PhysicalInput &input,
                                                       const ControlOutput &output) noexcept
{
    // 位置型P制御で動かす
    const auto tau_error = apply_tolerance(params.target_tau - input.shear_stress_kpa(), params.err_stress);
    const auto max_rpm = math_constexpr::abs(params.max_rpm);
    // 制御偏差がerr_stress * 2以上で最大回転数、err_stress * 0.9で0となるようにKpを設定する
    // 軸対象にするため、rpmは絶対値で計算し、davout_motor_cruchで方向を決定する
    const auto kp = max_rpm / (params.err_stress * 2.0 - params.err_stress * 0.9);
    const auto rpm = tau_error != 0.0 ? std::clamp(kp * (math_constexpr::abs(tau_error) - params.err_stress * 0.9),
                                                   -max_rpm, max_rpm)
                                      : 0.0;

    auto new_output = output;
    const double current_es = input.shear_displacement_mm;
    const bool out_of_stroke = current_es <= MinStroke || MaxStroke <= current_es;
    new_output.motor_rpm = out_of_stroke ? 0.0 : math_constexpr::copysign(rpm, direction_sign(params.loading_dir));
    new_output.step_completed = out_of_stroke && new_output.step_completed;
    if !consteval
    {
        spdlog::debug("Motor Tau Control: target_tau={:.6} kPa, shear_stress={:.6} kPa, "
                      "tau_error={:.6} kPa, kp={:.6} RPM/kPa, computed_rpm={:.6} RPM, next_rpm={:.6} RPM",
                      params.target_tau, input.shear_stress_kpa(), tau_error, kp, rpm, new_output.motor_rpm);
    }
    return new_output;
}

/**
 * @brief Stop motor (turn off motor and set speed to 0)
 *
 * @param davout_motor Motor on/off voltage
 * @param davout_motor_speed Motor speed voltage
 */
inline constexpr void stop_motor(float &davout_motor, float &davout_motor_speed) noexcept
{
    davout_motor = 0.0f;
    davout_motor_speed = 0.0f;
}

/**
 * @brief Parameters for motor acceleration control
 */
struct MotorAccelerationParams
{
    bool loading_dir;                         // Loading direction: true=loading, false=unloading
    double start_rpm;                         // Starting RPM
    double target_rpm;                        // Target RPM
    double acceleration;                      // Acceleration rate (RPM/min)
    double err_rpm;                           // RPM error tolerance
    std::chrono::minutes_d time_interval_min; // Time interval in minutes
};

/**
 * @brief Apply motor acceleration control
 *
 * Ramps motor speed from start_rpm to target_rpm at specified acceleration rate.
 * Returns true when target is reached (indicating step should advance).
 *
 * @param params Control parameters
 * @param davout_motor_speed Motor speed voltage (input/output)
 * @return true if target RPM reached, false otherwise
 */
inline constexpr ControlOutput apply_motor_acceleration(const MotorAccelerationParams &params,
                                                        const control::PhysicalInput &,
                                                        const ControlOutput &prev_state) noexcept
{
    const auto rpm_diff = apply_tolerance(params.target_rpm - prev_state.motor_rpm, params.err_rpm);

    auto output = prev_state;
    if (
        // Check if accelerating upward
        (params.start_rpm < params.target_rpm && params.acceleration > 0.0 && rpm_diff >= 0.0) ||
        // Check if decelerating downward
        (params.start_rpm > params.target_rpm && params.acceleration < 0.0 && rpm_diff <= 0.0))
    {
        // Still accelerating upward/downward
        output.motor_rpm += math_constexpr::copysign(params.acceleration * params.time_interval_min.count(),
                                                     direction_sign(params.loading_dir));
        output.step_completed = false;
        return output;
    }

    // Target reached
    output.step_completed = true;
    return output;
}

inline constexpr auto fallback(auto a, auto b) noexcept
{
    return a > 0 ? a : b;
}

/**
 * @brief Parameters for monotonic motor control
 */
struct MonotonicMotorControlParams
{
    double target_tau; // Target deviator stress (kPa)
    double motor_rpm;  // Motor speed (RPM)
    bool loading_dir;  // Loading direction: true=loading, false=unloading
};

/**
 * @brief Apply monotonic motor control (run motor until target stress is reached)
 *
 * Controls motor to monotonically move until target tau is reached:
 * - Checks stroke limits and stops if out of range
 * - Returns true if target is reached
 * - Otherwise runs motor at specified RPM in loading/unloading direction
 *
 * @param params Control parameters
 * @param davout_motor Output: motor on/off voltage
 * @param davout_motor_speed Output: motor speed voltage
 * @param davout_motor_cruch Output: motor clutch voltage
 * @return true if target reached or out of stroke range, false otherwise
 */
inline constexpr ControlOutput apply_monotonic_motor_control(const MonotonicMotorControlParams &params,
                                                             const control::PhysicalInput &input,
                                                             const ControlOutput &prev_state) noexcept
{

    const double current_es = input.shear_displacement_mm;
    const bool out_of_stroke = current_es <= MinStroke || MaxStroke <= current_es;
    auto state = prev_state;
    state.motor_rpm =
        out_of_stroke ? 0.0 : math_constexpr::copysign(params.motor_rpm, direction_sign(params.loading_dir));
    const bool target_reached = (params.loading_dir && input.shear_stress_kpa() > params.target_tau) ||
                                (!params.loading_dir && input.shear_stress_kpa() < params.target_tau);
    state.step_completed = !out_of_stroke && target_reached;
    return state;
}

/**
 * @brief Parameters for displacement-controlled monotonic motor control
 */
struct MonotonicMotorControlDispParams
{
    double target_es; // Target shear displacement (mm)
    double motor_rpm; // Motor speed (RPM)
    bool loading_dir; // Loading direction: true=loading, false=unloading
};

/**
 * @brief Apply displacement-controlled monotonic motor control
 *
 * Controls motor to monotonically move until target displacement is reached:
 * - Checks stroke limits and stops if out of range
 * - Returns true if target displacement is reached
 * - Otherwise runs motor at specified RPM in loading/unloading direction
 *
 * @param params Control parameters
 * @param davout_motor Output: motor on/off voltage
 * @param davout_motor_speed Output: motor speed voltage
 * @param davout_motor_cruch Output: motor clutch voltage
 * @return true if target reached or out of stroke range, false otherwise
 */
inline constexpr ControlOutput apply_monotonic_motor_control_disp(const MonotonicMotorControlDispParams &params,
                                                                  const control::PhysicalInput &input,
                                                                  const ControlOutput &prev_state) noexcept
{
    const double current_es = input.shear_displacement_mm;
    const bool out_of_stroke = current_es <= MinStroke || MaxStroke <= current_es;
    auto state = prev_state;
    state.motor_rpm =
        out_of_stroke ? 0.0 : math_constexpr::copysign(params.motor_rpm, direction_sign(params.loading_dir));
    const bool target_reached =
        (params.loading_dir && current_es > params.target_es) || (!params.loading_dir && current_es < params.target_es);
    state.step_completed = !out_of_stroke && target_reached;
    return state;
}

/**
 * @brief Parameters for cyclic motor control (stress-based)
 */
struct CyclicMotorControlParams
{
    bool loading_dir;  // Loading direction: true=start unloading, false=start loading
    double motor_rpm;  // Motor speed (RPM)
    double tau_lower;  // Lower tau limit (kPa)
    double tau_upper;  // Upper tau limit (kPa)
    size_t num_cycles; // Number of cycles
};

/**
 * @brief Apply cyclic motor control (stress-based)
 *
 * Controls motor to cycle between tau_lower and tau_upper for specified number of cycles.
 * - Manages cycle counter (num_cyclic) and direction flag (flag_cyclic)
 * - Returns true when all cycles are completed
 *
 * @param params Control parameters
 * @param num_cyclic Input/Output: current cycle counter
 * @param flag_cyclic Input/Output: current cycle direction flag
 * @param davout_motor Output: motor on/off voltage
 * @param davout_motor_speed Output: motor speed voltage
 * @param davout_motor_cruch Output: motor clutch voltage
 * @return true if all cycles completed, false otherwise
 */
inline constexpr ControlOutput apply_cyclic_motor_control(const CyclicMotorControlParams &params,
                                                          const control::PhysicalInput &input,
                                                          const ControlOutput &prev_state) noexcept
{
    const double current_es = input.shear_displacement_mm;
    const bool out_of_stroke = current_es <= MinStroke || MaxStroke <= current_es;
    auto state = prev_state;
    state.motor_rpm = out_of_stroke ? 0.0 : params.motor_rpm;
    state.step_completed = !out_of_stroke && state.num_cyclic > params.num_cycles;

    if (state.step_completed)
        return state;

    if (state.num_cyclic == 0)
    {
        state.num_cyclic++;
        state.flag_cyclic = params.loading_dir;
    }

    if (!state.flag_cyclic)
    {
        state.motor_rpm = -math_constexpr::abs(params.motor_rpm); // Unloading
        if (input.shear_stress_kpa() <= params.tau_lower)
        {

            state.flag_cyclic = !state.flag_cyclic;
            if (params.loading_dir)
                state.num_cyclic++;
        }
    }
    else
    {
        state.motor_rpm = math_constexpr::abs(params.motor_rpm); // Loading
        if (input.shear_stress_kpa() >= params.tau_upper)
        {
            state.flag_cyclic = !state.flag_cyclic;
            if (!params.loading_dir)
                state.num_cyclic++;
        }
    }
    return state;
}

/**
 * @brief Parameters for cyclic motor control (displacement-based)
 */
struct CyclicMotorControlDispParams
{
    bool loading_dir;          // Loading direction: true=start unloading, false=start loading
    double motor_rpm;          // Motor speed (RPM)
    double displacement_lower; // Lower displacement limit (mm)
    double displacement_upper; // Upper displacement limit (mm)
    size_t num_cycles;         // Number of cycles
};

/**
 * @brief Apply cyclic motor control (displacement-based)
 *
 * Controls motor to cycle between displacement_lower and displacement_upper for specified number of cycles.
 * - Manages cycle counter (num_cyclic) and direction flag (flag_cyclic)
 * - Returns true when all cycles are completed
 *
 * @param params Control parameters
 * @param num_cyclic Input/Output: current cycle counter
 * @param flag_cyclic Input/Output: current cycle direction flag
 * @param davout_motor Output: motor on/off voltage
 * @param davout_motor_speed Output: motor speed voltage
 * @param davout_motor_cruch Output: motor clutch voltage
 * @return true if all cycles completed, false otherwise
 */
inline constexpr ControlOutput apply_cyclic_motor_control_disp(const CyclicMotorControlDispParams &params,
                                                               const control::PhysicalInput &input,
                                                               const ControlOutput &prev_state) noexcept
{
    const double current_es = input.shear_displacement_mm;
    const bool out_of_stroke = current_es <= MinStroke || MaxStroke <= current_es;
    auto state = prev_state;
    state.motor_rpm = out_of_stroke ? 0.0 : params.motor_rpm;
    state.step_completed = !out_of_stroke && state.num_cyclic > params.num_cycles;

    if (state.step_completed)
        return state;

    if (state.num_cyclic == 0)
    {
        state.num_cyclic++;
        state.flag_cyclic = params.loading_dir;
    }

    if (!state.flag_cyclic)
    {
        state.motor_rpm = -math_constexpr::abs(params.motor_rpm); // Unloading
        if (current_es <= params.displacement_lower)
        {

            state.flag_cyclic = !state.flag_cyclic;
            if (params.loading_dir)
                state.num_cyclic++;
        }
    }
    else
    {
        state.motor_rpm = math_constexpr::abs(params.motor_rpm); // Loading
        if (current_es >= params.displacement_upper)
        {
            state.flag_cyclic = !state.flag_cyclic;
            if (!params.loading_dir)
                state.num_cyclic++;
        }
    }
    return state;
}
