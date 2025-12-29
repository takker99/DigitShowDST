#pragma once

#include "chrono_alias.hpp"
#include "control/params.hpp"
#include <chrono>
#include <vector>

namespace control
{

/**
 * @brief Struct representing a single control step
 *
 * Each step contains a ControlParams instance which includes both the control pattern
 * and all associated parameters.
 */
struct ControlStep
{
    ControlParams parameters; // Control parameters (includes pattern field)
};

inline std::chrono::seconds SequentTime1{0};

// Time elapsed in current control tick (steady_clock duration)
inline std::chrono::steady_clock::duration CtrlStepTime{0};

inline size_t CURNUM = 0;
inline std::vector<ControlStep> control_steps; // Flexible-size array of control steps (128-step limit removed)
inline size_t NUM_Cyclic = 0;

// Total elapsed time in current control step (steady_clock duration)
inline std::chrono::steady_clock::duration TotalStepTime{0};

} // namespace control
