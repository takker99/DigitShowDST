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
 * @file board_control.cpp
 * @brief Implementation of Modbus RTU board control layer
 */

#include "StdAfx.h"

#include "Constants.h"
#include "DataConvert.h"
#include "Variables.hpp"
#include "board_control.hpp"
#include "modbus_rtu.hpp"
#include <algorithm>
#include <array>
#include <format>
#include <spdlog/spdlog.h>

namespace board_control
{

namespace
{
// Previous output values for change detection
static std::array<uint16_t, modbus_rtu::MAX_AO_CHANNELS> s_previous_ao_values = {};
static bool s_first_write = true;

// Conversion factors (these would be calibration-dependent in real system)
// For now using simple linear mapping
constexpr float VOLTAGE_TO_MV_FACTOR = 1000.0f; // Volts to millivolts

} // anonymous namespace

std::expected<InitResult, std::string> InitializeBoards(const std::string_view modbusPort) noexcept
{
    using namespace board;

    spdlog::info("InitializeBoards called with Modbus port: {}", modbusPort);

    if (Flag_SetBoard)
    {
        spdlog::warn("Board already initialized");
        return std::unexpected("Board already initialized");
    }

    // Initialize Modbus RTU
    if (auto result = modbus_rtu::initialize(modbusPort); !result)
    {
        spdlog::error("Failed to initialize Modbus RTU: {}", result.error());
        return std::unexpected(std::format("Modbus RTU initialization failed: {}", result.error()));
    }

    // Set up board configuration for Modbus RTU
    // 16 AI channels (HX711 + ADS1115), 8 AO channels (GP8403)
    AdMaxCH = modbus_rtu::MAX_AI_CHANNELS;
    AvSmplNum = 1; // No averaging with Modbus RTU

    Flag_SetBoard = true;
    s_first_write = true;
    s_previous_ao_values.fill(0);

    InitResult initResult;
    initResult.modbusPort = std::string(modbusPort);

    spdlog::info("Modbus RTU board initialization completed successfully");
    return initResult;
}

std::expected<void, std::string> ResetAllVoltageOutputs() noexcept
{
    using namespace board;
    using namespace variables;

    spdlog::info("Resetting all analog voltage outputs to 0V");

    if (!Flag_SetBoard)
    {
        spdlog::debug("Board not initialized (Flag_SetBoard is false), skipping voltage reset");
        return {};
    }

    // Reset all DAVout channels to 0.0f
    for (size_t i = 0; i < MAX_DA_CHANNELS; ++i)
    {
        DAVout[i] = 0.0f;
    }

    // Force write to hardware
    s_first_write = true;
    const auto result = WriteAnalogOutputs();
    if (!result)
    {
        spdlog::error("Failed to reset voltage outputs: {}", result.error());
        return result;
    }

    spdlog::info("All analog voltage outputs reset to 0V successfully");
    return {};
}

std::expected<void, std::string> CloseBoards() noexcept
{
    using namespace board;

    spdlog::info("CloseBoards called");

    if (!Flag_SetBoard)
    {
        spdlog::debug("No boards to close (Flag_SetBoard is false)");
        return {};
    }

    // Close Modbus RTU connection
    if (auto result = modbus_rtu::close(); !result)
    {
        spdlog::warn("Error closing Modbus RTU: {}", result.error());
    }

    Flag_SetBoard = false;
    s_first_write = true;
    s_previous_ao_values.fill(0);

    spdlog::info("Modbus RTU boards closed successfully");
    return {};
}

std::expected<void, std::string> ReadAnalogInputs() noexcept
{
    using namespace board;
    using namespace variables;

    // Read raw values from Modbus RTU
    std::array<int16_t, modbus_rtu::MAX_AI_CHANNELS> raw_values;
    if (auto result = modbus_rtu::read_input_registers(raw_values); !result)
    {
        spdlog::error("Modbus RTU read failed: {}", result.error());
        return std::unexpected(result.error());
    }

    // Convert raw int16_t values to voltages
    // Note: Actual conversion depends on sensor calibration
    // For HX711 and ADS1115, conversion formula may differ
    // This is a placeholder - real implementation needs proper calibration
    constexpr float RAW_TO_VOLT_SCALE = 10.0f / 32768.0f; // ±10V range for 16-bit signed

    for (size_t i = 0; i < std::min(raw_values.size(), Vout.size()); ++i)
    {
        Vout[i] = static_cast<float>(raw_values[i]) * RAW_TO_VOLT_SCALE;
    }

    spdlog::trace("Read {} analog input channels successfully", raw_values.size());
    return {};
}

std::expected<void, std::string> WriteAnalogOutputs() noexcept
{
    using namespace board;
    using namespace variables;

    // Convert DAVout voltages to millivolts
    std::array<uint16_t, modbus_rtu::MAX_AO_CHANNELS> ao_values_mv;
    bool values_changed = s_first_write;

    for (size_t i = 0; i < modbus_rtu::MAX_AO_CHANNELS; ++i)
    {
        // Convert voltage to millivolts and clamp
        const uint16_t mv = static_cast<uint16_t>(
            std::clamp(DAVout[i] * VOLTAGE_TO_MV_FACTOR, 0.0f, static_cast<float>(modbus_rtu::MAX_OUTPUT_MV)));

        ao_values_mv[i] = mv;

        // Check if value changed
        if (mv != s_previous_ao_values[i])
        {
            values_changed = true;
            spdlog::trace("AO channel {} changed: {} -> {} mV", i, s_previous_ao_values[i], mv);
        }
    }

    // Only write if values changed
    if (values_changed)
    {
        if (auto result = modbus_rtu::write_holding_registers(ao_values_mv); !result)
        {
            spdlog::error("Modbus RTU write failed: {}", result.error());
            return std::unexpected(result.error());
        }

        // Update previous values
        s_previous_ao_values = ao_values_mv;
        s_first_write = false;

        spdlog::trace("Wrote {} analog output channels successfully", ao_values_mv.size());
    }
    else
    {
        spdlog::trace("AO values unchanged, skipping Modbus write");
    }

    return {};
}

// DEPRECATED FUNCTIONS - Kept as stubs for compatibility

std::expected<void, std::string> ConfigureFifoSampling(HWND hWnd, const float samplingClockUs,
                                                       const unsigned long samplingTimes) noexcept
{
    spdlog::warn("ConfigureFifoSampling called but FIFO not used with Modbus RTU - ignoring");
    return {};
}

std::expected<long, std::string> GetMinimumSamplingCount() noexcept
{
    spdlog::warn("GetMinimumSamplingCount called but not applicable to Modbus RTU - returning 0");
    return 0;
}

std::expected<long, std::string> GetSamplingData(const long samplingTimes) noexcept
{
    spdlog::warn("GetSamplingData called but not applicable to Modbus RTU - ignoring");
    return samplingTimes;
}

std::expected<void, std::string> ResetAndRestartSampling() noexcept
{
    spdlog::warn("ResetAndRestartSampling called but not applicable to Modbus RTU - ignoring");
    return {};
}

std::expected<void, std::string> ConfigureInitialSampling(HWND hWnd) noexcept
{
    spdlog::warn("ConfigureInitialSampling called but not applicable to Modbus RTU - ignoring");
    return {};
}

} // namespace board_control
