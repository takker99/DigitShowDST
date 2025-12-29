# Control Parameter Migration (PR#A)

This directory contains the new control parameter infrastructure that replaces the legacy CFPARA array-based system with named parameters.

## Overview

The legacy control system used a `CFPARA[18]` array to store control parameters, where each index had a specific meaning depending on the control pattern (CFNUM). This made the code difficult to understand and maintain.

The new `ControlParams` struct provides:
- **Named fields** instead of array indices
- **Type safety** (e.g., `int` for cycles instead of `double`)
- **Self-documenting code** through descriptive field names
- **Backward compatibility** through conversion functions

## Files

- **params.hpp**: Main header defining `ControlParams` struct and conversion functions
- **params_example.cpp**: Example usage demonstrating conversions (not compiled)

## Usage

### Converting from Legacy CFPARA

```cpp
#include "control/params.hpp"

// Existing code has CFPARA array
std::array<double, 18> cfpara = control::control_steps[step].parameters;

// Convert to named parameters
control::ControlParams params = control::ControlParams::FromCFPARA(cfpara);

// Now access by name instead of index
double rpm = params.motor_rpm;           // Instead of cfpara[1]
double tau = params.target_tau_kpa;      // Instead of cfpara[2]
int cycles = params.cycles;              // Instead of (int)cfpara[4]
```

### Converting to Legacy CFPARA

```cpp
// Create parameters with named fields
control::ControlParams params;
params.loading_dir = 0.0;
params.motor_rpm = 1200.0;
params.target_tau_kpa = 50.0;
params.target_sigma_kpa = 100.0;
params.cycles = 10;
params.description = "Cyclic loading";

// Convert to legacy array for existing control code
std::array<double, 18> cfpara = params.ToCFPARA();

// Use with existing control logic
control::control_steps[step].parameters = cfpara;
```

## Field Mapping

| ControlParams Field      | CFPARA Index | Description                                  | Units       |
|--------------------------|--------------|----------------------------------------------|-------------|
| `loading_dir`            | `[0]`        | Direction (0.0=load, 1.0=unload)             | -           |
| `motor_rpm`              | `[1]`        | Motor speed                                  | RPM         |
| `target_tau_kpa`         | `[2]`        | Target shear stress                          | kPa         |
| `target_sigma_kpa`       | `[3]`        | Target confining pressure                    | kPa         |
| `duration_min`           | `[2]/[0]`    | Duration (pattern-dependent)                 | minutes     |
| `cycles`                 | `[4]`        | Number of cycles                             | -           |
| `err_stress_kpa`         | `[10]`       | Stress tolerance override                    | kPa         |
| `err_disp_mm`            | `[11]`       | Displacement tolerance override              | mm          |
| `amp_v_per_kpa_m2`       | `[12]`       | Stress loop gain override                    | V/(kPa·m²)  |
| `amp2_f_v_per_mm`        | `[13]`       | Front displacement gain override             | V/mm        |
| `amp2_r_v_per_mm`        | `[14]`       | Rear displacement gain override              | V/mm        |
| `dmax_v`                 | `[15]`       | EP slew limit override                       | V           |
| `name` (legacy: `description`) | -            | Human-readable description (not in CFPARA)   | -           |

Note: CFPARA indices [5-9], [16-17] are currently unused or pattern-specific.

Compatibility: The JSON loader accepts a legacy `description` string for older control scripts and maps it to `name`. When saving, the UI now emits `name` and a top-level `with` object for pattern parameters (previous files with top-level keys are still supported on load).

## UI Changes

The Control dialog (IDD_Control_File) has been updated to show parameter names instead of CFPARA indices:

**Before:**
```
CFPARA[0]  CFPARA[1]  CFPARA[2]  CFPARA[3]  CFPARA[4]  CFPARA[5]
```

**After:**
```
loading_dir  motor_rpm  target_tau  target_sigma  duration_min  cycles
```

## Migration Strategy

This PR introduces the `ControlParams` struct and conversion functions as a **compatibility layer**. Existing code continues to use CFPARA arrays internally. Future PRs will:

1. Update control pattern functions to accept `ControlParams` directly
2. Refactor control dispatch logic to use named parameters
3. Remove or minimize direct CFPARA array usage
4. Eventually deprecate CFPARA in favor of ControlParams throughout

## Related Documentation

- `knowledge/control_specifications.md` - Authoritative CFPARA index mapping
- `knowledge/control_parameter_inventory.md` - Control pattern catalog
- `knowledge/refactor_plan_2025-10-29.md` - Phase 2 strategy extraction plan

## Context

This is part of Phase 2 (Strategy & Separation) from the modernization roadmap. The goal is to make the control system more maintainable while preserving backward compatibility during the transition.
