# Config Persistence Test Plan

This document describes how to test the new application state persistence feature.

## Test Scenarios

### 1. Sampling Time Persistence

**Setup:**
1. Start the application for the first time
2. The default `api_config.json` will be created with `sampling_time_ms: 1000` (1 second)

**Test Steps:**
1. Open the application
2. Locate the "Sampling Time" combo box in the UI
3. Select a different sampling time (e.g., "0.5 s")
4. Click the "Set" button
5. Close the application
6. Open `api_config.json` in a text editor
7. Verify that `sampling_time_ms` is now `500`
8. Restart the application
9. Verify that the sampling time display shows "500" ms

**Expected Result:**
- The sampling time persists across application restarts
- The config file is updated when the setting changes
- The saved value is loaded on next startup

### 2. Calibration File Path Persistence

**Setup:**
1. Prepare a calibration file (`.json`, `.yml`, or `.yaml` format)
2. Start the application

**Test Steps:**
1. Open the Calibration Factor dialog (from menu)
2. Click "Load Config" button
3. Select your calibration file
4. Verify the calibration loads successfully
5. Close the dialog
6. Close the application
7. Open `api_config.json` in a text editor
8. Verify that `last_calibration_file` contains the path to your calibration file

**Expected Result:**
- The last loaded calibration file path is saved to config
- The path is in UTF-8 format
- The path can be used as a quick reference for which calibration was last loaded

### 3. Config File Structure

**Test Steps:**
1. After running the above tests, open `api_config.json`
2. Verify it contains:
   ```json
   {
     "$schema": "schemas/api_config.schema.json",
     "version": "<git-commit-hash>",
     "enabled": false,
     "host": "127.0.0.1",
     "port": 8080,
     "update_interval_ms": 100,
     "cors_enabled": true,
     "max_connections": 10,
     "sampling_time_ms": <your-value>,
     "last_calibration_file": "<path-to-last-loaded-file>"
   }
   ```

**Expected Result:**
- Config file is well-formed JSON
- All fields are present (except `last_calibration_file` may be omitted if empty)
- Version field shows the git commit hash
- Values match the application state

### 4. Error Handling

**Test Steps:**
1. Manually corrupt `api_config.json` (e.g., remove a closing brace)
2. Start the application
3. Check the log file for error messages

**Expected Result:**
- Application starts successfully with default values
- Error is logged: "Error loading API config from api_config.json: <error details>. Using defaults."
- Application continues to function normally

## Manual Testing Notes

Since this is a Windows MFC application, testing should be performed on:
- Windows 10/11
- With Visual Studio Build Tools
- In both Debug and Release configurations

## Automated Testing

The config persistence logic uses standard C++ file I/O and JSON serialization.
Unit tests could be added to verify:
- `ApiServer::load_config()` correctly parses valid JSON
- `ApiServer::save_config()` writes correct JSON format
- Invalid config files fall back to defaults
- Schema validation works correctly

However, full integration testing requires the MFC application environment.
