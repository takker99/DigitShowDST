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
 * @file modbus_rtu.cpp
 * @brief Implementation of Modbus RTU backend
 */

#include "StdAfx.h"

#include "modbus_rtu.hpp"
#include <algorithm>
#include <format>
#include <mutex>
#include <spdlog/spdlog.h>

namespace modbus_rtu
{

namespace
{
// Static state
static bool s_initialized = false;
static std::string s_last_error;
static std::mutex s_mutex;

// TODO: Add actual Modbus RTU communication state here
// - Serial port handle
// - Device address
// - Communication buffers

} // anonymous namespace

std::expected<void, std::string> initialize(const std::string_view portName) noexcept
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_initialized)
    {
        spdlog::warn("Modbus RTU already initialized");
        return std::unexpected("Modbus RTU already initialized");
    }

    spdlog::info("Initializing Modbus RTU on port: {}, baudrate: {}", portName, BAUDRATE);

    // TODO: Implement actual Modbus RTU initialization
    // 1. Open serial port (portName)
    // 2. Configure baudrate to BAUDRATE (38400)
    // 3. Set 8N1 (8 data bits, no parity, 1 stop bit) - typical for Modbus RTU
    // 4. Set timeouts appropriately for 100ms timer cycle
    // 5. Test connection with a read operation

    // For now, just mark as initialized (stub implementation)
    s_initialized = true;
    s_last_error.clear();

    spdlog::info("Modbus RTU initialized successfully (STUB)");
    return {};
}

std::expected<void, std::string> close() noexcept
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_initialized)
    {
        spdlog::debug("Modbus RTU not initialized, nothing to close");
        return {};
    }

    spdlog::info("Closing Modbus RTU connection");

    // TODO: Implement actual Modbus RTU cleanup
    // 1. Close serial port
    // 2. Free any allocated resources

    s_initialized = false;
    s_last_error.clear();

    spdlog::info("Modbus RTU closed successfully");
    return {};
}

std::expected<void, std::string> read_input_registers(std::array<int16_t, MAX_AI_CHANNELS> &values) noexcept
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_initialized)
    {
        s_last_error = "Modbus RTU not initialized";
        return std::unexpected(s_last_error);
    }

    // IMPORTANT: This function should ONLY be called from Timer 1 (100ms)
    // Add runtime check to verify this (optional, but recommended for debugging)
    spdlog::trace("Reading input registers (16 channels)");

    // TODO: Implement actual Modbus RTU read operation
    // Function code 0x04 (Read Input Registers)
    // Start address: 0 (or device-specific)
    // Quantity: 16 registers
    // Expected response: 16 * 2 = 32 bytes of data
    //
    // HX711 channels (0-7): int16_t values
    // ADS1115 channels (8-15): int16_t values

    // Stub: Fill with zeros for now
    values.fill(0);

    spdlog::trace("Input registers read successfully (STUB)");
    return {};
}

std::expected<void, std::string> write_holding_registers(const std::array<uint16_t, MAX_AO_CHANNELS> &values) noexcept
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_initialized)
    {
        s_last_error = "Modbus RTU not initialized";
        return std::unexpected(s_last_error);
    }

    // IMPORTANT: This function should ONLY be called from Timer 1 (100ms)
    // after read_input_registers() and only if AO values changed
    spdlog::trace("Writing holding registers (8 channels)");

    // Clamp values to valid range [0, MAX_OUTPUT_MV]
    std::array<uint16_t, MAX_AO_CHANNELS> clamped_values;
    for (size_t i = 0; i < MAX_AO_CHANNELS; ++i)
    {
        clamped_values[i] = std::clamp(values[i], static_cast<uint16_t>(0), MAX_OUTPUT_MV);
        if (values[i] != clamped_values[i])
        {
            spdlog::debug("Output channel {} clamped from {} to {} mV", i, values[i], clamped_values[i]);
        }
    }

    // TODO: Implement actual Modbus RTU write operation
    // Function code 0x10 (Write Multiple Registers)
    // Start address: 0 (or device-specific for GP8403)
    // Quantity: 8 registers
    // Data: clamped_values (8 * 2 = 16 bytes)
    //
    // GP8403 expects values in millivolts (0-10000 range)

    spdlog::trace("Holding registers written successfully (STUB)");
    return {};
}

constexpr bool is_initialized() noexcept
{
    return s_initialized;
}

std::string get_last_error() noexcept
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_last_error;
}

} // namespace modbus_rtu
