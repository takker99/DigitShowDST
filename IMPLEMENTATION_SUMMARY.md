# D/A Calibration Factor Implementation Summary

## Overview

This implementation adds support for persisting D/A (Digital-to-Analog)
calibration factors in YAML configuration files, solving the problem where users
had to manually reconfigure D/A settings on every application startup.

## Problem Solved

**Before this change:**

- Users had to open "D/A Channels" dialog and manually set calibration factors
  on every app startup
- OR modify Variables.hpp and recompile the application
- Settings were lost when the application closed

**After this change:**

- D/A calibration factors are saved to YAML files along with A/D calibration
  data
- Settings persist across application restarts
- No recompilation needed to change calibration factors
- Users can share calibration files between machines/users

## Implementation Details

### 1. Schema Changes

**File:** `schemas/calibration_factor.schema.json`

Added new `da_calibration_data` field:

```json
"da_calibration_data": {
  "type": "array",
  "description": "Array of D/A (output) calibration coefficients",
  "minItems": 0,
  "maxItems": 8,
  "items": {
    "type": "object",
    "required": ["channel", "factors"],
    "properties": {
      "channel": { "type": "integer", "minimum": 0, "maximum": 7 },
      "factors": {
        "type": "array",
        "minItems": 2,
        "maxItems": 2,
        "items": { "type": "number" },
        "description": "Polynomial-ordered factors: [f0 (constant b), f1 (linear a)]"
      }
    }
  }
}
```

**Formula:** `Output_Voltage = a * Physical_Value + b` (stored in files as
`factors: [b, a]`, i.e. `f0 = b (constant)`, `f1 = a (linear)`)

### 2. Code Changes

#### CalibrationFactor.h

- Added `CHANNELS_DA` constant (8)
- Added member variables:
  - `std::array<std::array<double, 3>, CHANNELS_CAL> m_Cal{}` — per-channel A/D
    polynomial factors (`f[0]=const, f[1]=linear, f[2]=quadratic`)
  - `std::array<std::array<double, 2>, CHANNELS_DA> m_DACal{}` — per-channel D/A
    factors (`f[0]=const (b), f[1]=linear (a)`)

#### CalibrationFactor.cpp

**CF_Load() function:**

- Loads D/A calibration factors from the consolidated global `DA_Cal` (pairs
  `{b,a}`)
- Populates dialog member variables `m_DACal` (mapping: `m_DACal[i][0]=b`,
  `m_DACal[i][1]=a`)

**Update() function:**

- Saves D/A calibration factors from `m_DACal` back to global `DA_Cal`
- Called when user clicks "Update" button

**OnBUTTONCFSaveConfig() function:**

- Creates `da_calibration_data` YAML node
- Saves D/A factors for channels where
  `DA_Cal[i][0] != 0.0 || DA_Cal[i][1] != 0.0` (i.e., `b` or `a` non-zero)
- Writes to YAML/JSON file along with A/D calibration data

**OnBUTTONCFLoadConfig() function:**

- Initializes D/A arrays to zero (default for omitted channels)
- Loads `da_calibration_data` if present in file
- Applies loaded values to member variables
- Calls `Update()` to apply to global arrays

### 3. File Format Example

**Example YAML file:**

```yaml
calibration_data:
  - channel: 0
    factors: [0.0, 1.0, 0.0] # c, b, a (polynomial order f0,f1,f2)

da_calibration_data:
  - channel: 2
    factors: [0.0, 0.0033333] # b, a
  - channel: 3
    factors: [-0.286962967, 0.017854906]
  - channel: 4
    factors: [-0.335375138, 0.018384256]

initial_specimen:
  height_mm: 120.0
  area_mm2: 14400.0
  weight_g: 0.0
  box_weight_g: 10000.0
```

## User Workflow

### Current Workflow (Implemented)

1. **Edit D/A Calibration:**
   - Open "D/A Channels" from menu
   - Set calibration factors for channels 0-7
   - Click OK

2. **Save to File:**
   - Open "Calibration Factors" dialog
   - Click "Save" button
   - Choose filename (e.g., `my_setup.yml`)
   - Both A/D and D/A factors are saved

3. **Load from File:**
   - Open "Calibration Factors" dialog
   - Click "Load" button
   - Select saved file
   - Both A/D and D/A factors are loaded
   - Click "Update" to apply

4. **Result:**
   - No need to reconfigure D/A factors on next startup
   - Settings persist across sessions

### Future Workflow (After UI Integration)

1. Open "Calibration Factors" dialog
2. Edit both A/D and D/A factors in same dialog
3. Click "Save" to persist
4. No separate "D/A Channels" dialog needed

## Backward Compatibility

- ✅ Old calibration files without `da_calibration_data` still work
- ✅ Missing D/A data defaults to zero (a=0, b=0)
- ✅ Existing A/D calibration functionality unchanged
- ✅ New files can be read by old versions (they'll ignore da_calibration_data)

## Testing

- ✅ Schema validation passes with test YAML file
- ✅ JSON schema is valid (Draft 7)
- ✅ Code follows existing patterns from A/D calibration
- ⏳ Manual testing required on Windows (build/runtime)

## Known Limitations

1. **UI Integration Pending:**
   - D/A edit controls not yet in CalibrationFactor dialog
   - Users must use separate "D/A Channels" dialog to edit
   - See `UI_INTEGRATION_GUIDE.md` for integration steps

2. **No Validation UI:**
   - No real-time feedback if D/A values are out of range
   - Users must know appropriate values for their hardware

## Security Considerations

- ✅ File format is validated against JSON schema
- ✅ Invalid YAML/JSON rejected with error message
- ✅ Out-of-range channel numbers (>7) ignored
- ✅ Type validation ensures numeric coefficients

## Performance Impact

- Minimal: D/A data adds ~200 bytes to YAML file
- No runtime performance impact
- File I/O is same as before (RapidYAML library)

## Future Improvements

1. **Short-term (Required):**
   - Add D/A edit controls to CalibrationFactor dialog
   - Remove "D/A Channels" menu item
   - Add unit tests for D/A save/load

2. **Long-term (Optional):**
   - Add validation ranges for D/A coefficients
   - Add tooltips explaining coefficient meaning
   - Add "Test Output" button to verify D/A calibration

## References

- Issue: "DAのcalibration factorsも`calibration.yml`で設定する"
- Schema: `schemas/calibration_factor.schema.json`
- Code: `src/CalibrationFactor.cpp`, `src/CalibrationFactor.h`
- Documentation: `UI_INTEGRATION_GUIDE.md`, `schemas/README.md`
- Test data: `test_calibration_with_da.yaml`
- Global state: `src/Variables.hpp` (lines 67-70)
