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

#include "StdAfx.h"

#include "physical_variables.hpp"

namespace variables::physical
{
void update() noexcept
{
    const control::PhysicalInput initial_based_input{.specimen = SpecimenData,
                                                     .shear_force_N = Phyout[CH_SHEAR_LC],
                                                     .front_vertical_force_N = Phyout[CH_FRONT_VERTICAL_LC],
                                                     .rear_vertical_force_N = Phyout[CH_REAR_VERTICAL_LC],
                                                     .shear_displacement_mm = Phyout[CH_SHEAR_DISP],
                                                     .front_vertical_disp_mm = Phyout[CH_FRONT_VERTICAL_DISP],
                                                     .rear_vertical_disp_mm = Phyout[CH_REAR_VERTICAL_DISP],
                                                     .front_friction_force_N = Phyout[CH_FRONT_FRICTION_LC],
                                                     .rear_friction_force_N = Phyout[CH_REAR_FRICTION_LC]};

    auto expected = latest_physical_input.load();
    while (!latest_physical_input.compare_exchange_weak(expected, rebase(initial_based_input, expected.specimen)))
    {
    }

    latest_physical_output.store(
        {control::fromVoltage(static_cast<double>(DAVout[CH_FRONT_EP_CELL]), DA_Cal[CH_FRONT_EP_CELL]),
         control::fromVoltage(static_cast<double>(DAVout[CH_REAR_EP_CELL]), DA_Cal[CH_REAR_EP_CELL]),
         control::fromIISMotorVoltage(DAVout[CH_Motor], DAVout[CH_MotorCruch], DAVout[CH_MotorSpeed],
                                      DA_Cal[CH_MotorSpeed])});
}

std::expected<void, std::string> set_output(const control::PhysicalOutput<> &physical) noexcept
{
    // Convert physical values to voltages
    const auto [motor_on_voltage, motor_clutch_voltage, motor_speed_voltage] =
        control::toIISMotorVoltage(physical.motor_rpm, DA_Cal[CH_MotorSpeed]);

    // クラッチの消耗を抑えるため、モーターが回転していないときはクラッチを操作しない
    if (motor_speed_voltage > 0.f)
    {
        DAVout[CH_Motor] = motor_on_voltage;
        DAVout[CH_MotorCruch] = motor_clutch_voltage;
    }
    DAVout[CH_MotorSpeed] = motor_speed_voltage;

    DAVout[CH_FRONT_EP_CELL] = static_cast<float>(control::toVoltage(physical.front_ep_kpa, DA_Cal[CH_FRONT_EP_CELL]));
    DAVout[CH_REAR_EP_CELL] = static_cast<float>(control::toVoltage(physical.rear_ep_kpa, DA_Cal[CH_REAR_EP_CELL]));

    if (auto result = digitshow::write_analog_outputs(); !result)
    {
        return std::unexpected(std::format("Hardware write failed: {}", result.error()));
    }

    update();
    return {};
}
} // namespace variables::physical