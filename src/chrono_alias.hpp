#pragma once

#include <chrono>

namespace std::chrono
{
using namespace std::literals::chrono_literals;

using microseconds_d = decltype(1.0us);
using seconds_d = decltype(1.0s);
using minutes_d = decltype(1.0min);
} // namespace std::chrono
