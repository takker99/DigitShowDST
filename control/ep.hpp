#pragma once

#include "math_constexpr.hpp"
#include "measurement.hpp"
#include "utils.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

/**
 * @brief Parameters for EP constant pressure control
 */
struct EPConstantPressureParams
{
    double target_sigma = 0.0; // Target confining pressure (kPa)
    double err_stress = 0.5;   // Stress error tolerance (kPa)
    double err_disp = 0.004;   // Displacement error tolerance (mm)
    double amp = 0.5;          // Stress control gain
    double amp2_f = 0.5;       // Front displacement gain
    double amp2_r = 0.5;       // Rear displacement gain
    double dmax = 1.2;         // Maximum output change per step (kPa)
};

/**
 * @brief Apply EP constant pressure control (P-control for confining pressure)
 *
 * This implements the three-state control logic:
 * 1. If sigma > target + ERR: Unload (reduce EP pressure)
 * 2. If sigma < target - ERR: Load (increase EP pressure)
 * 3. Otherwise: Neutral (balance front/rear displacement difference)
 *
 * @param params Control parameters
 * @param input Physical input state
 * @param output Physical output state
 * @return Updated physical output state
 */
template <control::PhysicalOutputLike Output>
inline constexpr Output apply_ep_constant_pressure_control(const EPConstantPressureParams &params,
                                                           const control::PhysicalInput &input,
                                                           const Output &output) noexcept
{
    // sigmaをPVとしてEPの平均圧力を速度型I制御する
    const auto ki_sigma = params.amp * input.specimen.area_mm2() / 1000.0;
    const auto sigma_error = apply_tolerance(params.target_sigma - input.vertical_stress_kpa(), params.err_stress);

    // 垂直変位差をPVとしてEPの圧力差を速度型I制御する
    const auto ki_disp_diff = 0.5 * (params.amp2_f + params.amp2_r);
    const auto disp_diff_error = apply_tolerance(0 - input.tilt_mm(), params.err_disp);

    const auto dmax = math_constexpr::abs(params.dmax);
    auto new_output = output;
    new_output.front_ep_kpa += std::clamp(ki_sigma * sigma_error + ki_disp_diff * disp_diff_error, -dmax, dmax);
    new_output.rear_ep_kpa += std::clamp(ki_sigma * sigma_error - ki_disp_diff * disp_diff_error, -dmax, dmax);

    if !consteval
    {
        spdlog::debug("EP Constant Pressure Control: target_sigma={:.6} kPa, current_sigma={:.6} kPa, "
                      "sigma_error={:.6} kPa, tilt={:.6} mm, disp_diff_error={:.6} mm, "
                      "ki_sigma={:.6} kPa/mm, ki_disp_diff={:.6} kPa/mm, "
                      "dmax={:.6} kPa, "
                      "delta_front_ep={:.6} kPa, delta_rear_ep={:.6} kPa",
                      params.target_sigma, input.vertical_stress_kpa(), sigma_error, input.tilt_mm(), disp_diff_error,
                      ki_sigma, ki_disp_diff, dmax, ki_sigma * sigma_error + ki_disp_diff * disp_diff_error,
                      ki_sigma * sigma_error - ki_disp_diff * disp_diff_error

        );
    }
    return new_output;
}

/**
 * @brief Parameters for EP constant volume control
 */
struct EPConstantVolumeParams
{
    double err_disp_cv = 0.002; // Constant volume displacement tolerance (mm)
    double amp_cv = 17.0;       // Constant volume control gain
    double dmax = 1.2;          // Maximum output change per step (kPa)
};

/**
 * @brief Apply EP constant volume control (maintain zero volumetric strain)
 *
 * Controls front and rear EP cells to keep displacements near zero,
 * implementing constant volume condition.
 *
 * @param params Control parameters
 * @param input Physical input state
 * @param output Physical output state
 * @return Updated physical output state
 */
template <control::PhysicalOutputLike Output>
inline constexpr Output apply_ep_constant_volume_control(const EPConstantVolumeParams &params,
                                                         const control::PhysicalInput &input,
                                                         const Output &output) noexcept
{
    // 平均垂直変位をPVとしてEPの平均圧力を速度型I制御する
    const auto ki_disp_ave = params.amp_cv;
    const auto disp_ave_error = apply_tolerance(0 - input.normal_displacement_mm(), params.err_disp_cv);

    // 垂直変位差をPVとしてEPの圧力差を速度型I制御する
    const auto ki_disp_diff = params.amp_cv;
    const auto disp_diff_error = apply_tolerance(0 - input.tilt_mm(), params.err_disp_cv);

    const auto dmax = math_constexpr::abs(params.dmax);
    auto new_output = output;
    new_output.front_ep_kpa += std::clamp(ki_disp_ave * disp_ave_error + ki_disp_diff * disp_diff_error, -dmax, dmax);
    new_output.rear_ep_kpa += std::clamp(ki_disp_ave * disp_ave_error - ki_disp_diff * disp_diff_error, -dmax, dmax);
    return new_output;
}
