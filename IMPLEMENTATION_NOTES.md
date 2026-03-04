# Implementation Summary: Application State Persistence

## Overview
This PR implements persistent storage of application settings in `api_config.json` to address issue #1.

## Implemented Features

### 1. Sampling Time Persistence (Timer 3 Interval)
- **What**: The data logging interval can now be saved and restored across sessions
- **Where**: 
  - Loaded: `CDigitShowDSTDoc::StartApiServer()` 
  - Saved: `CDigitShowDSTView::OnBUTTONSetTimeInterval()`
- **Range**: 50-600000 milliseconds (0.05 seconds to 10 minutes)
- **UI Impact**: When user changes sampling time via combo box, it persists to next session

### 2. Calibration File Path Persistence
- **What**: The path to the last loaded calibration file is saved
- **Where**: `CCalibrationFactor::OnBUTTONCFLoadConfig()`
- **Purpose**: Provides quick reference to which calibration was last used
- **UI Impact**: Path is saved automatically when calibration file is loaded successfully

## Technical Implementation

### Schema Changes
Extended `schemas/api_config.schema.json` with:
```json
{
  "sampling_time_ms": {
    "type": "integer",
    "description": "Timer 3 sampling interval in milliseconds for data logging",
    "default": 1000,
    "minimum": 50,
    "maximum": 600000
  },
  "last_calibration_file": {
    "type": "string",
    "description": "Path to the last loaded calibration file (for convenience)"
  }
}
```

### Code Changes
1. **ApiConfig struct** (`src/ApiServer.hpp`):
   - Added `int sampling_time_ms = 1000`
   - Added `std::string last_calibration_file = ""`

2. **ApiServer methods** (`src/ApiServer.cpp`):
   - Enhanced `load_config()` to read new fields
   - Implemented `save_config()` to persist entire config
   - Both methods handle JSON serialization/deserialization
   - Error handling with fallback to defaults

3. **Application integration**:
   - Sampling time loaded on app startup before API server start
   - Sampling time saved whenever user changes it
   - Calibration path saved on successful load

## Error Handling
- Invalid/corrupt config files fall back to defaults
- All config operations logged via spdlog
- File I/O wrapped in try-catch blocks
- Application continues normally even if config fails

## Backward Compatibility
- Existing `api_config.json` files without new fields work fine
- New fields use defaults if not present
- Version field remains informational (not loaded back)

## Testing
See `test_config_persistence.md` for detailed test plan.

Key test scenarios:
1. First run creates default config
2. Sampling time persists across restarts
3. Calibration path saved on load
4. Corrupt config handled gracefully

## Documentation
- Updated README.md with config file section
- Created `api_config.example.json` showing all fields
- Created `test_config_persistence.md` with test plan
- Added code comments explaining version field behavior

## Files Modified
- `schemas/api_config.schema.json`: Schema extension
- `src/ApiServer.hpp`: Struct and method declarations
- `src/ApiServer.cpp`: Load/save implementation
- `src/DigitShowDSTDoc.cpp`: Load config on startup
- `src/DigitShowDSTView.cpp`: Save sampling time on change
- `src/CalibrationFactor.cpp`: Save calibration path on load
- `api_config.json`: Added default sampling_time_ms
- `README.md`: Usage documentation
- `api_config.example.json`: Example configuration (new)
- `test_config_persistence.md`: Test plan (new)

## Security
No vulnerabilities detected by CodeQL scan.

## Future Considerations
The issue mentioned possibly renaming the file to `dsb_config.json`. This can be done in a future PR if desired by:
1. Adding a migration function to copy `api_config.json` → `dsb_config.json`
2. Updating all references in code
3. Updating schema path
4. Maintaining backward compatibility with old filename

## Impact
- **User Experience**: Settings persist across sessions
- **Code Maintenance**: Centralized config management
- **Testing**: Requires manual testing on Windows with Visual Studio
- **Breaking Changes**: None

## Notes
- Config file created automatically with defaults if missing
- All fields optional except "enabled" (per schema)
- Version field tracks which software version last saved the file
- UTF-8 encoding for file paths ensures international character support
