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
 * @file board_control.hpp
 * @brief High-level board control layer for Modbus RTU I/O operations
 *
 * This layer provides a unified API for hardware I/O operations using Modbus RTU backend.
 * 
 * MIGRATION NOTE: This file has been refactored to use Modbus RTU instead of CONTEC CAIO.
 * All Aio* function calls have been removed and replaced with modbus_rtu:: calls.
 * 
 * IMPORTANT TIMING CONSTRAINTS:
 * - All Modbus communication happens ONLY from Timer 1 (100ms interval)
 * - ReadInputRegisters: Timer 1 routine
 * - WriteHoldingRegisters: Timer 1 routine, only if outputs changed
 * - No other timing or calling context is permitted
 */

#pragma once

#include "Board.hpp"
#include <Windows.h>
#include <chrono>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace board_control
{

/**
 * @brief Board initialization result
 */
struct InitResult
{
    std::string modbusPort; // COM port used for Modbus RTU
};

/**
 * @brief Initialize Modbus RTU board communication
 * @param modbusPort COM port name (e.g., "COM1", "COM3")
 * @return Expected with InitResult or error message
 *
 * This replaces the old CONTEC AIO board initialization.
 * Sets up Modbus RTU at fixed 38400bps for 16 AI channels and 8 AO channels.
 */
[[nodiscard]] std::expected<InitResult, std::string> InitializeBoards(const std::string_view modbusPort) noexcept;

/**
 * @brief Reset all analog voltage outputs to 0V
 * @return Expected with void or error message
 *
 * This function sets all DAVout channels to 0.0f and writes them to hardware.
 * Should be called before closing boards to ensure devices stop safely.
 */
[[nodiscard]] std::expected<void, std::string> ResetAllVoltageOutputs() noexcept;

/**
 * @brief Close Modbus RTU connection
 * @return Expected with void or error message
 */
[[nodiscard]] std::expected<void, std::string> CloseBoards() noexcept;

/**
 * @brief Read analog input from Modbus RTU and populate Vout array
 * @return Expected with void or error message
 *
 * Reads 16 input registers via Modbus RTU ReadInputRegister function.
 * - Channels 0-7: HX711 (int16_t)
 * - Channels 8-15: ADS1115 (int16_t)
 * 
 * CRITICAL: This function MUST ONLY be called from Timer 1 (100ms interval).
 * No other calling context is permitted.
 */
[[nodiscard]] std::expected<void, std::string> ReadAnalogInputs() noexcept;

/**
 * @brief Write analog output to Modbus RTU
 * @return Expected with void or error message
 *
 * Writes 8 holding registers via Modbus RTU WriteHoldingRegisters function.
 * - Channels 0-7: GP8403 DAC (uint16_t, 0-10000 mV, clamped)
 * 
 * CRITICAL: This function MUST ONLY be called from Timer 1 (100ms interval),
 * immediately after ReadAnalogInputs() and only if output values have changed.
 */
[[nodiscard]] std::expected<void, std::string> WriteAnalogOutputs() noexcept;

// DEPRECATED FUNCTIONS - FIFO operations not needed with Modbus RTU
// These are kept as stubs for compatibility but log warnings if called.

[[nodiscard]] std::expected<void, std::string> ConfigureFifoSampling(HWND hWnd, float samplingClockUs,
                                                                      unsigned long samplingTimes) noexcept;

[[nodiscard]] std::expected<long, std::string> GetMinimumSamplingCount() noexcept;

[[nodiscard]] std::expected<long, std::string> GetSamplingData(long samplingTimes) noexcept;

[[nodiscard]] std::expected<void, std::string> ResetAndRestartSampling() noexcept;

[[nodiscard]] std::expected<void, std::string> ConfigureInitialSampling(HWND hWnd) noexcept;

} // namespace board_control
