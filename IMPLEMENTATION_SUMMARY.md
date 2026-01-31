# D/A Calibration Factor Implementation Summary

## Overview
This implementation adds support for persisting D/A (Digital-to-Analog) calibration factors in YAML configuration files, solving the problem where users had to manually reconfigure D/A settings on every application startup.

## Problem Solved
**Before this change:**
- Users had to open "D/A Channels" dialog and manually set calibration factors on every app startup
- OR modify Variables.hpp and recompile the application
- Settings were lost when the application closed

**After this change:**
- D/A calibration factors are saved to YAML files along with A/D calibration data
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
    "required": ["channel", "da_cal_a", "da_cal_b"],
    "properties": {
      "channel": { "type": "integer", "minimum": 0, "maximum": 7 },
      "da_cal_a": { "type": "number" },
      "da_cal_b": { "type": "number" }
    }
  }
}
```

**Formula:** `Output_Voltage = da_cal_a * Physical_Value + da_cal_b`

### 2. Code Changes

#### CalibrationFactor.h
- Added `CHANNELS_DA` constant (8)
- Added member variables:
  - `std::array<double, CHANNELS_DA> m_DA_Cala{}`
  - `std::array<double, CHANNELS_DA> m_DA_Calb{}`

#### CalibrationFactor.cpp

**CF_Load() function:**
- Loads D/A calibration factors from global arrays (`DA_Cal_a`, `DA_Cal_b`)
- Populates dialog member variables

**Update() function:**
- Saves D/A calibration factors back to global arrays
- Called when user clicks "Update" button

**OnBUTTONCFSaveConfig() function:**
- Creates `da_calibration_data` YAML node
- Saves D/A factors for channels where `DA_Cal_a[i] != 0.0 || DA_Cal_b[i] != 0.0`
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
    cal_a: 0.0
    cal_b: 1.0
    cal_c: 0.0

da_calibration_data:
  - channel: 2
    da_cal_a: 0.0033333
    da_cal_b: 0.0
  - channel: 3
    da_cal_a: 0.017854906
    da_cal_b: -0.286962967
  - channel: 4
    da_cal_a: 0.018384256
    da_cal_b: -0.335375138

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
