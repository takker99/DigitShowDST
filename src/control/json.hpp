#pragma once

#pragma warning(push)
#pragma warning(disable : 4800 4866)

#include "control.hpp"

#include <format>

#include "chrono_alias.hpp"
#include "ui_helpers.hpp"
#include <fstream>
#include <optional>
#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>
#include <span>
#include <spdlog/spdlog.h>
#include <sstream>

// Helper function to get optional string value with default
std::string get_string(const ryml::ConstNodeRef &node, const char *key, const char *default_val = "")
{
    if (!node.has_child(ryml::to_csubstr(key)))
        return default_val;
    auto child = node[ryml::to_csubstr(key)];
    if (!child.has_val())
        return default_val;
    std::string result;
    child >> result;
    return result;
}

// Helper function to get optional double value with default
double get_double(const ryml::ConstNodeRef &node, const char *key, double default_val = 0.0)
{
    if (!node.has_child(ryml::to_csubstr(key)))
        return default_val;
    auto child = node[ryml::to_csubstr(key)];
    if (!child.has_val())
        return default_val;
    double result = NAN;
    child >> result;
    return result;
}

// Helper function to get optional int value with default
int get_int(const ryml::ConstNodeRef &node, const char *key, int default_val = 0)
{
    if (!node.has_child(ryml::to_csubstr(key)))
        return default_val;
    auto child = node[ryml::to_csubstr(key)];
    if (!child.has_val())
        return default_val;
    int result = 0;
    child >> result;
    return result;
}

// Helper function to get optional size_t value with default
size_t get_size_t(const ryml::ConstNodeRef &node, const char *key, size_t default_val = 0)
{
    if (!node.has_child(ryml::to_csubstr(key)))
        return default_val;
    auto child = node[ryml::to_csubstr(key)];
    if (!child.has_val())
        return default_val;
    size_t result = 0;
    child >> result;
    return result;
}

// Convert JSON step with "use" field to ControlParams
// New format: { "id"?: string, "name"?: string, "use": string, "with"?: { ... pattern props } }
std::optional<control::ControlParams> JsonStepToControlParams(const ryml::ConstNodeRef &step)
{
    using namespace std::chrono;

    if (!step.has_child("use"))
        return std::nullopt;

    std::string use = get_string(step, "use");
    if (use.empty())
        return std::nullopt;

    control::ControlParams params{};

    auto it = use_map.find(use);
    if (it == use_map.end())
    {
        return std::nullopt; // Unknown pattern
    }
    else
    {
        params.pattern = it->second;
    }

    // Get the 'with' object for pattern-specific properties
    // If 'with' is not present, create an empty node reference to use defaults
    const bool has_with = step.has_child("with");
    ryml::ConstNodeRef with_node = has_with ? step["with"] : step; // fallback to step for backward compat reading

    // For patterns that use 'with', read from the 'with' object
    // Direction values: "load"/"unload" for most patterns, "compression"/"dilation" for consolidation patterns
    switch (params.pattern)
    {
    case ControlPattern::NoControl:
        break;

    case ControlPattern::MonotonicLoadingConstantPressure:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_tau_kpa = get_double(with_node, "tau_kPa", 0.0);
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        break;

    case ControlPattern::MonotonicLoadingConstantVolume:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_tau_kpa = get_double(with_node, "tau_kPa", 0.0);
        break;

    case ControlPattern::CyclicLoadingConstantPressure:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.tau_lower_kpa = get_double(with_node, "tau_lower_kPa", 0.0);
        params.tau_upper_kpa = get_double(with_node, "tau_upper_kPa", 0.0);
        params.cycles = get_size_t(with_node, "num_cycles", 0);
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        break;

    case ControlPattern::CyclicLoadingConstantVolume:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.tau_lower_kpa = get_double(with_node, "tau_lower_kPa", 0.0);
        params.tau_upper_kpa = get_double(with_node, "tau_upper_kPa", 0.0);
        params.cycles = get_size_t(with_node, "num_cycles", 0);
        break;

    case ControlPattern::CreepConstantPressure:
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_tau_kpa = get_double(with_node, "tau_kPa", 0.0);
        params.duration = minutes_d{get_double(with_node, "time_min", 0.0)};
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        break;

    case ControlPattern::CreepConstantVolume:
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_tau_kpa = get_double(with_node, "tau_kPa", 0.0);
        params.duration = minutes_d{get_double(with_node, "time_min", 0.0)};
        break;

    case ControlPattern::RelaxationConstantPressure:
        params.duration = minutes_d{get_double(with_node, "time_min", 0.0)};
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        break;

    case ControlPattern::RelaxationConstantVolume:
        params.duration = minutes_d{get_double(with_node, "time_min", 0.0)};
        break;

    case ControlPattern::MonotonicLoadingDisplacementConstantPressure:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_displacement_mm = get_double(with_node, "target_displacement_mm", 0.0);
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        break;

    case ControlPattern::MonotonicLoadingDisplacementConstantVolume:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_displacement_mm = get_double(with_node, "target_displacement_mm", 0.0);
        break;

    case ControlPattern::CyclicLoadingDisplacementConstantPressure:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.displacement_lower_mm = get_double(with_node, "displacement_lower_mm", 0.0);
        params.displacement_upper_mm = get_double(with_node, "displacement_upper_mm", 0.0);
        params.cycles = get_size_t(with_node, "num_cycles", 0);
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        break;

    case ControlPattern::CyclicLoadingDisplacementConstantVolume:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.displacement_lower_mm = get_double(with_node, "displacement_lower_mm", 0.0);
        params.displacement_upper_mm = get_double(with_node, "displacement_upper_mm", 0.0);
        params.cycles = get_size_t(with_node, "num_cycles", 0);
        break;

    case ControlPattern::AccelerationConstantPressure:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.acceleration_rate_rpm_per_min = get_double(with_node, "acceleration_rate_rpm_per_min", 0.0);
        params.target_tau_kpa = get_double(with_node, "target_tau_kPa", 0.0);
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        break;

    case ControlPattern::AccelerationConstantVolume:
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.acceleration_rate_rpm_per_min = get_double(with_node, "acceleration_rate_rpm_per_min", 0.0);
        params.target_tau_kpa = get_double(with_node, "target_tau_kPa", 0.0);
        break;

    case ControlPattern::ConstantTauConsolidation:
        params.loading_dir = get_string(with_node, "direction", "compression") == "compression";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_tau_kpa = get_double(with_node, "tau_kPa", 0.0);
        params.consolidation_rate_kpa_per_min = get_double(with_node, "consolidation_rate_kPa_per_min", 0.0);
        params.target_sigma_kpa = get_double(with_node, "target_sigma_kPa", 0.0);
        break;

    case ControlPattern::KConsolidation:
        params.loading_dir = get_string(with_node, "direction", "compression") == "compression";
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.tau_start_kpa = get_double(with_node, "tau_start_kPa", 0.0);
        params.tau_end_kpa = get_double(with_node, "tau_end_kPa", 0.0);
        params.sigma_start_kpa = get_double(with_node, "sigma_start_kPa", 0.0);
        params.k_value = get_double(with_node, "k_value", 0.0);
        break;

    case ControlPattern::CreepConstantPressureFast:
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_tau_kpa = get_double(with_node, "tau_kPa", 0.0);
        params.duration = minutes_d{get_double(with_node, "time_min", 0.0)};
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        break;

    case ControlPattern::CreepConstantPressureFastRef:
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        params.target_tau_kpa = get_double(with_node, "tau_kPa", 0.0);
        params.duration = minutes_d{get_double(with_node, "time_min", 0.0)};
        params.target_sigma_kpa = get_double(with_node, "sigma_kPa", 0.0);
        params.loading_dir = get_string(with_node, "direction", "load") == "load";
        break;

    case ControlPattern::PreConsolidation:
        params.target_tau_kpa = get_double(with_node, "target_tau_kPa", 0.0);
        params.motor_rpm = get_double(with_node, "motor_rpm", 0.0);
        break;

    case ControlPattern::BeforeConsolidation:
        // No parameters needed for rebase_reference (alias: before_consolidation)
        break;

    case ControlPattern::AfterConsolidation:
        // No parameters needed for rebase_reference (alias: after_consolidation)
        break;

    default:
        return std::nullopt;
    }

    // Note: 'overrides' property is no longer supported in the new format.
    // If present, it will be ignored (no backward compat merging).

    // Load optional name field from top-level step
    // Backwards compatibility: older files used `description` (string) instead of `name`.
    // Prefer `name` if present, otherwise fall back to `description`.
    params.name = get_string(step, "name", "");
    if (params.name.empty())
    {
        params.name = get_string(step, "description", "");
    }

    return params;
}

// Convert ControlParams to ryml tree for JSON emission
// New format: { "name"?: string, "use": string, "with"?: { ... pattern props } }
ryml::Tree ControlParamsToJsonStep(const control::ControlParams &params)
{
    ryml::Tree tree;
    ryml::NodeRef step = tree.rootref();
    step |= ryml::MAP;

    // Write 'use' field first
    switch (params.pattern)
    {
    case ControlPattern::NoControl:
        step["use"] << "no_control";
        break;
    case ControlPattern::MonotonicLoadingConstantPressure:
        step["use"] << "monotonic_loading_constant_pressure";
        break;
    case ControlPattern::MonotonicLoadingConstantVolume:
        step["use"] << "monotonic_loading_constant_volume";
        break;
    case ControlPattern::CyclicLoadingConstantPressure:
        step["use"] << "cyclic_loading_constant_pressure";
        break;
    case ControlPattern::CyclicLoadingConstantVolume:
        step["use"] << "cyclic_loading_constant_volume";
        break;
    case ControlPattern::CreepConstantPressure:
        step["use"] << "creep_constant_pressure";
        break;
    case ControlPattern::CreepConstantVolume:
        step["use"] << "creep_constant_volume";
        break;
    case ControlPattern::RelaxationConstantPressure:
        step["use"] << "relaxation_constant_pressure";
        break;
    case ControlPattern::RelaxationConstantVolume:
        step["use"] << "relaxation_constant_volume";
        break;
    case ControlPattern::MonotonicLoadingDisplacementConstantPressure:
        step["use"] << "monotonic_loading_displacement_constant_pressure";
        break;
    case ControlPattern::MonotonicLoadingDisplacementConstantVolume:
        step["use"] << "monotonic_loading_displacement_constant_volume";
        break;
    case ControlPattern::CyclicLoadingDisplacementConstantPressure:
        step["use"] << "cyclic_loading_displacement_constant_pressure";
        break;
    case ControlPattern::CyclicLoadingDisplacementConstantVolume:
        step["use"] << "cyclic_loading_displacement_constant_volume";
        break;
    case ControlPattern::AccelerationConstantPressure:
        step["use"] << "acceleration_constant_pressure";
        break;
    case ControlPattern::AccelerationConstantVolume:
        step["use"] << "acceleration_constant_volume";
        break;
    case ControlPattern::ConstantTauConsolidation:
        step["use"] << "constant_tau_consolidation";
        break;
    case ControlPattern::KConsolidation:
        step["use"] << "k_consolidation";
        break;
    case ControlPattern::CreepConstantPressureFast:
        step["use"] << "creep_constant_pressure_fast";
        break;
    case ControlPattern::CreepConstantPressureFastRef:
        step["use"] << "creep_constant_pressure_fast_ref";
        break;
    case ControlPattern::PreConsolidation:
        step["use"] << "pre_consolidation";
        break;
    case ControlPattern::BeforeConsolidation:
        step["use"] << "rebase_reference";
        break;
    case ControlPattern::AfterConsolidation:
        step["use"] << "rebase_reference";
        break;
    default:
        step["use"] << "no_control";
        break;
    }

    // Save name field at top level if not empty
    if (!params.name.empty())
    {
        step["name"] << params.name;
    }

    // Create 'with' object for pattern-specific properties
    ryml::NodeRef with_node = step["with"];
    with_node |= ryml::MAP;

    switch (params.pattern)
    {
    case ControlPattern::NoControl:
        // No 'with' properties for no_control
        break;

    case ControlPattern::MonotonicLoadingConstantPressure:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_kPa"] << params.target_tau_kpa;
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::MonotonicLoadingConstantVolume:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_kPa"] << params.target_tau_kpa;
        break;

    case ControlPattern::CyclicLoadingConstantPressure:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_lower_kPa"] << params.tau_lower_kpa;
        with_node["tau_upper_kPa"] << params.tau_upper_kpa;
        with_node["num_cycles"] << params.cycles;
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::CyclicLoadingConstantVolume:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_lower_kPa"] << params.tau_lower_kpa;
        with_node["tau_upper_kPa"] << params.tau_upper_kpa;
        with_node["num_cycles"] << params.cycles;
        break;

    case ControlPattern::CreepConstantPressure:
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_kPa"] << params.target_tau_kpa;
        with_node["time_min"] << params.duration.count();
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::CreepConstantVolume:
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_kPa"] << params.target_tau_kpa;
        with_node["time_min"] << params.duration.count();
        break;

    case ControlPattern::RelaxationConstantPressure:
        with_node["time_min"] << params.duration.count();
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::RelaxationConstantVolume:
        with_node["time_min"] << params.duration.count();
        break;

    case ControlPattern::MonotonicLoadingDisplacementConstantPressure:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["target_displacement_mm"] << params.target_displacement_mm;
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::MonotonicLoadingDisplacementConstantVolume:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["target_displacement_mm"] << params.target_displacement_mm;
        break;

    case ControlPattern::CyclicLoadingDisplacementConstantPressure:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["displacement_lower_mm"] << params.displacement_lower_mm;
        with_node["displacement_upper_mm"] << params.displacement_upper_mm;
        with_node["num_cycles"] << params.cycles;
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::CyclicLoadingDisplacementConstantVolume:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["displacement_lower_mm"] << params.displacement_lower_mm;
        with_node["displacement_upper_mm"] << params.displacement_upper_mm;
        with_node["num_cycles"] << params.cycles;
        break;

    case ControlPattern::AccelerationConstantPressure:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["acceleration_rate_rpm_per_min"] << params.acceleration_rate_rpm_per_min;
        with_node["target_tau_kPa"] << params.target_tau_kpa;
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::AccelerationConstantVolume:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["acceleration_rate_rpm_per_min"] << params.acceleration_rate_rpm_per_min;
        with_node["target_tau_kPa"] << params.target_tau_kpa;
        break;

    case ControlPattern::ConstantTauConsolidation:
        with_node["direction"] << (params.loading_dir ? "compression" : "dilation");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_kPa"] << params.target_tau_kpa;
        with_node["consolidation_rate_kPa_per_min"] << params.consolidation_rate_kpa_per_min;
        with_node["target_sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::KConsolidation:
        with_node["direction"] << (params.loading_dir ? "compression" : "dilation");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_start_kPa"] << params.tau_start_kpa;
        with_node["tau_end_kPa"] << params.tau_end_kpa;
        with_node["sigma_start_kPa"] << params.sigma_start_kpa;
        with_node["k_value"] << params.k_value;
        break;

    case ControlPattern::CreepConstantPressureFast:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_kPa"] << params.target_tau_kpa;
        with_node["time_min"] << params.duration.count();
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::CreepConstantPressureFastRef:
        with_node["direction"] << (params.loading_dir ? "load" : "unload");
        with_node["motor_rpm"] << params.motor_rpm;
        with_node["tau_kPa"] << params.target_tau_kpa;
        with_node["time_min"] << params.duration.count();
        with_node["sigma_kPa"] << params.target_sigma_kpa;
        break;

    case ControlPattern::PreConsolidation:
        with_node["target_tau_kPa"] << params.target_tau_kpa;
        with_node["motor_rpm"] << params.motor_rpm;
        break;

    case ControlPattern::BeforeConsolidation:
        // No 'with' properties for rebase_reference
        break;

    case ControlPattern::AfterConsolidation:
        // No 'with' properties for rebase_reference
        break;

    default:
        break;
    }

    // Remove empty 'with' node for patterns that don't need it
    if (params.pattern == ControlPattern::NoControl || params.pattern == ControlPattern::BeforeConsolidation ||
        params.pattern == ControlPattern::AfterConsolidation)
    {
        if (step.has_child("with"))
            step.remove_child("with");
    }

    // Note: 'overrides' property is no longer emitted in the new format

    return tree;
}
#pragma warning(pop)