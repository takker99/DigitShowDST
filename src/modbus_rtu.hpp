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
 * @file modbus_rtu.hpp
 * @brief Modbus RTU backend for analog I/O operations
 *
 * This module replaces CONTEC AIO (CAIO) driver with Modbus RTU communication.
 * 
 * Hardware configuration:
 * - Input: 16 channels int16_t
 *   - HX711 (channels 0-7): int16_t
 *   - ADS1115 (channels 8-15): int16_t
 * - Output: 8 channels uint16_t (0-10000 mV range)
 *   - GP8403 (channels 0-7): uint16_t clamped to [0, 10000]
 * - Baudrate: Fixed 38400bps
 * 
 * Communication timing:
 * - ReadInputRegister: Called only from 100ms Timer (Timer 1)
 * - WriteHoldingRegisters: Called only after ReadInputRegister if AO changed
 * - No buffering/averaging (assumes noise-free Modbus)
 */

#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace modbus_rtu
{

/** @brief Maximum number of analog input channels (HX711 + ADS1115) */
inline constexpr size_t MAX_AI_CHANNELS = 16;

/** @brief Maximum number of analog output channels (GP8403) */
inline constexpr size_t MAX_AO_CHANNELS = 8;

/** @brief Maximum output voltage in millivolts */
inline constexpr uint16_t MAX_OUTPUT_MV = 10000;

/** @brief Fixed baudrate for Modbus RTU communication */
inline constexpr uint32_t BAUDRATE = 38400;

/**
 * @brief Initialize Modbus RTU connection
 * @param portName COM port name (e.g., "COM1", "COM3")
 * @return Expected with void or error message
 * 
 * This function must be called before any AI/AO operations.
 */
[[nodiscard]] std::expected<void, std::string> initialize(const std::string_view portName) noexcept;

/**
 * @brief Close Modbus RTU connection
 * @return Expected with void or error message
 */
[[nodiscard]] std::expected<void, std::string> close() noexcept;

/**
 * @brief Read input registers (analog inputs)
 * @param[out] values Array to store 16 int16_t values
 * @return Expected with void or error message
 * 
 * This function reads 16 input registers:
 * - Channels 0-7: HX711 sensors (int16_t)
 * - Channels 8-15: ADS1115 sensors (int16_t)
 * 
 * IMPORTANT: This function must ONLY be called from Timer 1 (100ms interval).
 * No other timing or calling context is permitted.
 */
[[nodiscard]] std::expected<void, std::string> read_input_registers(
    std::array<int16_t, MAX_AI_CHANNELS> &values) noexcept;

/**
 * @brief Write holding registers (analog outputs)
 * @param values Array of 8 uint16_t values (in millivolts, 0-10000)
 * @return Expected with void or error message
 * 
 * This function writes 8 holding registers for GP8403 output:
 * - Channels 0-7: GP8403 DAC outputs
 * - Values are automatically clamped to [0, 10000] mV range
 * 
 * IMPORTANT: This function must ONLY be called from Timer 1 (100ms interval),
 * immediately after read_input_registers() if AO values have changed.
 */
[[nodiscard]] std::expected<void, std::string> write_holding_registers(
    const std::array<uint16_t, MAX_AO_CHANNELS> &values) noexcept;

/**
 * @brief Check if Modbus RTU is initialized
 * @return true if initialized, false otherwise
 */
[[nodiscard]] constexpr bool is_initialized() noexcept;

/**
 * @brief Get last error message
 * @return Last error message string
 */
[[nodiscard]] std::string get_last_error() noexcept;

} // namespace modbus_rtu
