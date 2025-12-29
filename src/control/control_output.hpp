/**
 * @file control_state.hpp
 * @brief Output state structure for control patterns
 *
 * This file defines the ControlOutput structure that all control patterns
 * return, containing motor control voltages, EP cell voltages, and state flags.
 */

#pragma once
#include "measurement.hpp"

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

    // Step completion flag
    bool step_completed = false;

    // Cyclic control state (only used by cyclic patterns)
    size_t num_cyclic = 0;    // Cycle counter
    bool flag_cyclic = false; // Cyclic state flag (loading/unloading phase)
};
