# Windows Testing Guide for Calibration Array Format

## Overview
This guide provides step-by-step instructions for testing the calibration coefficient array format changes on Windows.

## Prerequisites
- Windows 11 with Visual Studio 2026
- CONTEC API-AIO(WDM) Ver.8.90 or compatible
- Repository cloned with all submodules
- `.env` file configured with VS_INSTALL_PATH

## Build Instructions

### 1. Open PowerShell in Repository Root
```powershell
cd path\to\DigitShowDST
```

### 2. Load Development Environment
```powershell
. .\scripts\Load-DevEnv.ps1
```

### 3. Build the Project
```powershell
# Build Release configuration (recommended for testing)
msbuild .\DigitShowDST.vcxproj -t:Build -p:Configuration=Release -p:Platform=x64
```

Expected output: `Build succeeded` with no errors.

### 4. Build and Run Tests
```powershell
# Build test executable
msbuild .\DigitShowDST.vcxproj -t:Build -p:Configuration=Release -p:Platform=x64

# Run the test suite
.\x64\Release\test_yaml_support.exe
```

## Test Checklist

### ✅ Unit Tests (Automated)

Run the test executable and verify all tests pass:

```powershell
.\x64\Release\test_yaml_support.exe
```

**Expected Results:**
- `TEST_SUITE("Calibration Factor Array Format")` - All 4 tests PASS:
  - ✅ `LoadConfigFile parses calibration with new factors array format (JSON)`
  - ✅ `LoadConfigFile parses calibration with new factors array format (YAML)`
  - ✅ `LoadConfigFile maintains backward compatibility with old cal_a/b/c format`
  - ✅ `New format round-trip from JSON to YAML preserves array structure`

### ✅ Manual Testing in Application UI

#### Test 1: Load Old Format File
1. Launch `DigitShowDST.exe`
2. Open Calibration Factor dialog (Menu: Settings → Calibration Factor)
3. Click "Load Config" button
4. Select `test_data/old_format_calibration.json` (create this using old format)
5. **Expected**: Coefficients load successfully, no errors

**Verification:**
- Check that cal_a, cal_b, cal_c values display correctly in UI
- Check application log for message: `"Loaded calibration for channel X (legacy format)"`

#### Test 2: Save New Format File
1. With calibration data loaded, click "Save Config" button
2. Choose a filename (e.g., `my_calibration.json`)
3. Save as JSON format
4. **Expected**: File saved successfully

**Verification:**
- Open saved file in text editor
- Verify format is:
  ```json
  {
    "calibration_data": [
      {
        "channel": 0,
        "factors": [a, b, c]
      }
    ]
  }
  ```
- Should NOT contain `cal_a`, `cal_b`, `cal_c` fields

#### Test 3: Round-Trip Conversion
1. Load old format file (with cal_a, cal_b, cal_c)
2. Save as new file
3. Close dialog
4. Re-open Calibration Factor dialog
5. Load the newly saved file
6. **Expected**: Values match original, no data loss

**Verification:**
- Compare coefficient values before and after
- All channels should maintain their calibration values
- Check log shows: `"Loaded calibration for channel X (new format)"`

#### Test 4: Load New Format File
1. Click "Load Config" button
2. Select `test_data/sample_calibration.json` (already in new format)
3. **Expected**: Loads successfully

**Verification:**
- Coefficients display correctly
- Log shows: `"Loaded calibration for channel X (new format)"`

#### Test 5: Mixed Format Handling
1. Create a test file manually with mixed format:
   - Channel 0: new format `factors: [0.0, 1.0, 0.0]`
   - Channel 1: old format `cal_a: 0.0, cal_b: 1.0, cal_c: 0.0`
2. Load this file
3. **Expected**: Both channels load successfully
4. Save the file
5. **Expected**: Both channels now in new format

### ✅ Schema Validation Tests

Already verified on Linux, but you can re-verify on Windows:

```powershell
# Install ajv-cli if needed
npm install -g ajv-cli

# Validate schema
ajv compile -s schemas\calibration_factor.schema.json

# Validate test files
ajv validate -s schemas\calibration_factor.schema.json -d test_data\sample_calibration.json
ajv validate -s schemas\calibration_factor.schema.json -d test_data\sample_calibration_with_version.json
```

## Creating Test Files

### Old Format Test File
Create `test_data/old_format_calibration.json`:
```json
{
  "calibration_data": [
    {
      "channel": 0,
      "cal_a": 0.5,
      "cal_b": 1.5,
      "cal_c": 2.5
    },
    {
      "channel": 1,
      "cal_a": 0.0,
      "cal_b": 1.0,
      "cal_c": 0.0
    }
  ],
  "initial_specimen": {
    "height_mm": 120.0,
    "area_mm2": 14400.0,
    "weight_g": 0.0,
    "box_weight_g": 10000.0
  }
}
```

### Mixed Format Test File
Create `test_data/mixed_format_calibration.json`:
```json
{
  "calibration_data": [
    {
      "channel": 0,
      "factors": [0.5, 1.5, 2.5]
    },
    {
      "channel": 1,
      "cal_a": 0.0,
      "cal_b": 1.0,
      "cal_c": 0.0
    }
  ],
  "initial_specimen": {
    "height_mm": 120.0,
    "area_mm2": 14400.0,
    "weight_g": 0.0,
    "box_weight_g": 10000.0
  }
}
```

## Logging and Debugging

### Enable Trace Logging
To see detailed format detection messages:

1. Set log level to TRACE in application
2. Look for these messages in logs:
   - `"Loaded calibration for channel X (new format): a=..., b=..., c=..."`
   - `"Loaded calibration for channel X (legacy format): a=..., b=..., c=..."`
   - `"Channel X has 'factors' array with Y elements (expected 3), skipping"`
   - `"Channel X missing calibration data (neither 'factors' array nor individual 'cal_a/b/c' fields), skipping"`

### Common Issues

**Issue**: File won't load
- **Check**: Is JSON/YAML syntax valid?
- **Check**: Does file match schema?
- **Solution**: Validate with `ajv validate`

**Issue**: Old format file loads but saves in wrong format
- **Check**: Save function should always write new format
- **Verify**: Open saved file and check structure

**Issue**: Coefficients display as 0.0 in UI
- **Check**: Channel number matches expected index
- **Check**: Load function successfully parsed values
- **Solution**: Check application logs for warnings

## Success Criteria

All of the following must be true:

- ✅ All unit tests pass (4 tests in "Calibration Factor Array Format" suite)
- ✅ Old format files load successfully
- ✅ New format files load successfully
- ✅ Saving always produces new format
- ✅ Round-trip conversion preserves data
- ✅ UI displays coefficients correctly
- ✅ No errors or warnings in application log (except for intentionally malformed test files)
- ✅ Schema validates both formats

## Reporting Results

After completing all tests, please report:

1. **Build Status**: Did project build successfully?
2. **Unit Test Results**: Did all tests pass? Copy test output.
3. **Manual Test Results**: Check ✅ or ❌ for each test case above
4. **Issues Found**: Any problems or unexpected behavior
5. **Log Excerpts**: Relevant log messages showing format detection

## Screenshots Needed

Please capture screenshots showing:
1. Calibration Factor dialog with loaded old format coefficients
2. Saved file opened in text editor showing new format
3. Test execution output showing all tests passed
4. Application log showing format detection messages

## Next Steps

Once testing is complete:
1. Update PR description with test results
2. Remove `[WIP]` tag from PR title if all tests pass
3. Request code review
4. Merge when approved
