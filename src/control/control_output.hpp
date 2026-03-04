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
 * @file control_state.hpp
 * @brief Output state structure for control patterns
 *
 * This file defines the ControlOutput structure that all control patterns
 * return, containing motor control voltages, EP cell voltages, and state flags.
 */

#pragma once
#include "../Variables.hpp"
#include "../math_constexpr.hpp"
#include "measurement.hpp"
#include <algorithm>

/**
 * @brief Output state from control pattern execution
 *
 * This structure encapsulates all outputs and state changes from a control
 * pattern function, making the functions pure and testable.
 */
struct ControlOutput
{
    double front_ep_kpa = 0.0;
    double rear_ep_kpa = 0.0;
    double motor_rpm = 0.0;

    // Cyclic control state (only used by cyclic patterns)
    size_t num_cyclic = 0;    // Cycle counter
    bool flag_cyclic = false; // Cyclic state flag (loading/unloading phase)

    static bool can_output_front_ep(double pressure_kpa) noexcept
    {
        return pressure_kpa >= min_front_ep_kpa() && pressure_kpa <= max_front_ep_kpa();
    }

    static bool can_output_rear_ep(double pressure_kpa) noexcept
    {
        return pressure_kpa >= min_rear_ep_kpa() && pressure_kpa <= max_rear_ep_kpa();
    }

    bool is_motor_saturated() const noexcept
    {
        return math_constexpr::abs(motor_rpm) >= max_motor_rpm();
    }

    static double max_front_ep_kpa() noexcept
    {
        using namespace variables;
        const auto min_value = control::fromVoltage(MIN_VOLTAGE_OUTPUT, DA_Cal[CH_FRONT_EP_CELL]);
        const auto max_value = control::fromVoltage(MAX_VOLTAGE_OUTPUT, DA_Cal[CH_FRONT_EP_CELL]);
        return std::max(min_value, max_value);
    }
    static double min_front_ep_kpa() noexcept
    {
        using namespace variables;
        const auto min_value = control::fromVoltage(MIN_VOLTAGE_OUTPUT, DA_Cal[CH_FRONT_EP_CELL]);
        const auto max_value = control::fromVoltage(MAX_VOLTAGE_OUTPUT, DA_Cal[CH_FRONT_EP_CELL]);
        return std::min(min_value, max_value);
    }
    static double max_rear_ep_kpa() noexcept
    {
        using namespace variables;
        const auto min_value = control::fromVoltage(MIN_VOLTAGE_OUTPUT, DA_Cal[CH_REAR_EP_CELL]);
        const auto max_value = control::fromVoltage(MAX_VOLTAGE_OUTPUT, DA_Cal[CH_REAR_EP_CELL]);
        return std::max(min_value, max_value);
    }
    static double min_rear_ep_kpa() noexcept
    {
        using namespace variables;
        const auto min_value = control::fromVoltage(MIN_VOLTAGE_OUTPUT, DA_Cal[CH_REAR_EP_CELL]);
        const auto max_value = control::fromVoltage(MAX_VOLTAGE_OUTPUT, DA_Cal[CH_REAR_EP_CELL]);
        return std::min(min_value, max_value);
    }
    static double max_motor_rpm() noexcept
    {
        using namespace variables;
        return control::fromIISMotorVoltage(5.0f, 0.0f, MAX_VOLTAGE_OUTPUT, DA_Cal[CH_MotorSpeed]);
    }
};