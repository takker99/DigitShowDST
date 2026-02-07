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

#pragma once

#include "../Variables.hpp"
#include "../math_constexpr.hpp"
#include "control.hpp"
#include "measurement.hpp"
#include "params.hpp"
#include "utils.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

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
inline constexpr Output apply_ep_constant_pressure_control(
    const control::ControlParams::VerticalStress &vertical_stress_params,

    const control::ControlParams::Tilt &tilt_params, const control::PhysicalInput &input, const Output &output) noexcept
{
    // sigmaをPVとしてEPの平均圧力を速度型I制御する
    const auto ki_sigma = vertical_stress_params.kp * input.specimen.area_mm2() / variables::CYLINDER_AREA_MM2 *
                          (control::CtrlStepTime / vertical_stress_params.ti);
    const auto sigma_error =
        apply_tolerance(vertical_stress_params.setpoint - input.vertical_stress_kpa(), vertical_stress_params.error);

    // 垂直変位差をPVとしてEPの圧力差を速度型I制御する
    const auto tilt_error = apply_tolerance(tilt_params.setpoint - input.tilt_mm(), tilt_params.error);

    const auto max_pressure = math_constexpr::abs(vertical_stress_params.max_pressure_rate_kpa_per_s *
                                                  std::chrono::seconds_d{control::CtrlStepTime}.count());

    auto front_ep_delta_kpa =
        std::clamp(ki_sigma * sigma_error + tilt_params.ki_kpa_per_mm * tilt_error, -max_pressure, max_pressure);
    auto rear_ep_delta_kpa =
        std::clamp(ki_sigma * sigma_error - tilt_params.ki_kpa_per_mm * tilt_error, -max_pressure, max_pressure);
    // どちらかのEPが飽和していると、垂直変位差制御が逆にノイズになってしまうため、飽和時は制御を無効化する
    const auto can_control_tilt = Output::can_output_front_ep(output.front_ep_kpa + front_ep_delta_kpa) &&
                                  Output::can_output_rear_ep(output.rear_ep_kpa + rear_ep_delta_kpa);

    auto new_output = output;
    new_output.front_ep_kpa +=
        can_control_tilt ? front_ep_delta_kpa : std::clamp(ki_sigma * sigma_error, -max_pressure, max_pressure);
    new_output.rear_ep_kpa +=
        can_control_tilt ? rear_ep_delta_kpa : std::clamp(ki_sigma * sigma_error, -max_pressure, max_pressure);

    if !consteval
    {
        spdlog::trace("EP Constant Pressure Control: ");
        spdlog::trace("  Sigma Error = {:.3f} kPa - {:.3f} kPa = {:.3f} kPa (ignore less than {:.3f} kPa)",
                      vertical_stress_params.setpoint, input.vertical_stress_kpa(), sigma_error,
                      vertical_stress_params.error);
        spdlog::trace("  Tilt Error = {:.3f} mm - {:.3f} mm = {:.3f} mm (ignore less than {:.3f} mm)",
                      tilt_params.setpoint, input.tilt_mm(), tilt_error, tilt_params.error);
        spdlog::trace("  (EP is {}saturated)", can_control_tilt ? "not " : "");
        spdlog::trace("  Front EP Output: {:.3f} kPa", new_output.front_ep_kpa);
        spdlog::trace("  Rear EP Output: {:.3f} kPa", new_output.rear_ep_kpa);
    }

    return new_output;
}

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
inline constexpr Output apply_ep_constant_volume_control(const control::ControlParams::NormalDisplacement &disp_params,
                                                         const control::ControlParams::Tilt &tilt_params,
                                                         const control::PhysicalInput &input,
                                                         const Output &output) noexcept
{
    // 平均垂直変位をPVとしてEPの平均圧力を速度型I制御する
    const auto disp_ave_error =
        apply_tolerance(disp_params.setpoint - input.normal_displacement_mm(), disp_params.error);

    // 垂直変位差をPVとしてEPの圧力差を速度型I制御する
    const auto tilt_error = apply_tolerance(tilt_params.setpoint - input.tilt_mm(), tilt_params.error);

    const auto max_pressure = math_constexpr::abs(disp_params.max_pressure_rate_kpa_per_s *
                                                  std::chrono::seconds_d{control::CtrlStepTime}.count());

    auto front_ep_delta_kpa =
        std::clamp(disp_params.ki_kpa_per_mm * disp_ave_error + tilt_params.ki_kpa_per_mm * tilt_error, -max_pressure, max_pressure);
    auto rear_ep_delta_kpa =
        std::clamp(disp_params.ki_kpa_per_mm * disp_ave_error - tilt_params.ki_kpa_per_mm * tilt_error, -max_pressure, max_pressure);
    // どちらかのEPが飽和していると、垂直変位差制御が逆にノイズになってしまうため、飽和時は制御を無効化する
    const auto can_control_tilt = Output::can_output_front_ep(output.front_ep_kpa + front_ep_delta_kpa) &&
                                  Output::can_output_rear_ep(output.rear_ep_kpa + rear_ep_delta_kpa);

    auto new_output = output;
    new_output.front_ep_kpa +=
        can_control_tilt ? front_ep_delta_kpa : std::clamp(disp_params.ki_kpa_per_mm * disp_ave_error, -max_pressure, max_pressure);
    new_output.rear_ep_kpa +=
        can_control_tilt ? rear_ep_delta_kpa : std::clamp(disp_params.ki_kpa_per_mm * disp_ave_error, -max_pressure, max_pressure);

    if !consteval
    {
        spdlog::trace("EP Constant Volume Control: ");
        spdlog::trace("  Normal Displacement Error = {:.3f} mm - {:.3f} mm = {:.3f} mm (ignore less than {:.3f} mm)",
                      disp_params.setpoint, input.normal_displacement_mm(), disp_ave_error, disp_params.error);
        spdlog::trace("  Tilt Error = {:.3f} mm - {:.3f} mm = {:.3f} mm (ignore less than {:.3f} mm)",
                      tilt_params.setpoint, input.tilt_mm(), tilt_error, tilt_params.error);
        spdlog::trace("  (EP is {}saturated)", can_control_tilt ? "not " : "");
        spdlog::trace("  Front EP Output: {:.3f} kPa", new_output.front_ep_kpa);
        spdlog::trace("  Rear EP Output: {:.3f} kPa", new_output.rear_ep_kpa);
    }

    return new_output;
}
