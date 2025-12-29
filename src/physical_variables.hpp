#pragma once

#include "control/measurement.hpp"
#include <array>
#include <atomic>

namespace variables::physical
{

inline std::atomic<control::PhysicalInput> latest_physical_input{};
inline std::atomic<control::PhysicalOutput<>> latest_physical_output{};
} // namespace variables::physical