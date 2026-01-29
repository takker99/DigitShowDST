# Testing Guide for New REST API Features

This document provides a step-by-step guide to test the newly implemented REST API features.

## Prerequisites

1. Build the application in Visual Studio (Release or Debug)
2. Have `curl` or a browser available for testing
3. Optional: Python 3 for advanced tests

## Setup

1. **Enable the API Server**
   
   Edit `api_config.json`:
   ```json
   {
     "enabled": true,
     "host": "127.0.0.1",
     "port": 8080,
     "update_interval_ms": 100,
     "cors_enabled": true,
     "max_connections": 10
   }
   ```

2. **Start the Application**
   
   Run `DigitShowDST.exe` and wait for the API server to start. You should see:
   ```
   [info] Starting API server on 127.0.0.1:8080
   [info] API server started successfully at http://127.0.0.1:8080
   ```

## Test 1: Health Check

Verify the API server is running:

```bash
curl http://localhost:8080/api/health
```

**Expected Output:**
```json
{
  "status": "ok",
  "timestamp": 1704153600000
}
```

✅ **Pass Criteria:** Returns `status: "ok"` and a valid timestamp

## Test 2: Raw Voltage Data

Get current sensor data with raw voltages:

```bash
curl http://localhost:8080/api/sensor-data | jq '.raw_voltages'
```

**Expected Output:**
```json
{
  "ad_channels": [
    {"channel": 0, "voltage": 2.543},
    {"channel": 1, "voltage": 1.234},
    ...
  ],
  "da_channels": [
    {"channel": 0, "voltage": 5.012},
    {"channel": 1, "voltage": 0.0},
    ...
  ]
}
```

✅ **Pass Criteria:**
- `ad_channels` contains 64 channel objects
- `da_channels` contains 8 channel objects
- Voltage values are reasonable (typically -10V to +10V range)
- Values match the internal `Vout[]` and `DAVout[]` arrays (verify in debugger if needed)

## Test 3: Control State Information

Get current control state:

```bash
curl http://localhost:8080/api/sensor-data | jq '.control_state'
```

**Expected Output:**
```json
{
  "current_step": 0,
  "is_running": false,
  "elapsed_ms": 0
}
```

**Test with Control Running:**

1. Load a control file and start control
2. Query again while control is running

```bash
curl http://localhost:8080/api/sensor-data | jq '.control_state'
```

**Expected Output (during control):**
```json
{
  "current_step": 3,
  "is_running": true,
  "elapsed_ms": 15234
}
```

✅ **Pass Criteria:**
- `current_step` increments as control progresses
- `is_running` is `true` during control, `false` when stopped
- `elapsed_ms` increases over time within each step
- When control advances to next step, `elapsed_ms` resets to 0

## Test 4: Calibration Data API

Get calibration coefficients:

```bash
curl http://localhost:8080/api/calibration
```

**Expected Output:**
```json
{
  "ad_channels": [
    {
      "channel": 0,
      "cal_a": 0.0,
      "cal_b": 100.5,
      "cal_c": -0.5
    },
    ...
  ],
  "da_channels": [
    {
      "channel": 3,
      "cal_a": 0.017854906,
      "cal_b": -0.286962967
    },
    ...
  ]
}
```

✅ **Pass Criteria:**
- Returns calibration data for channels with non-default values
- Coefficients match those set in Calibration Factor dialog
- Default channels (a=0, b=1, c=0 for A/D; a=0, b=0 for D/A) are omitted

## Test 5: SSE Stream with Data Events

Stream real-time sensor data:

```bash
curl http://localhost:8080/api/sensor-data/stream
```

**Expected Output:**
```
event: data
data: {"timestamp":1704153600000,"physical_input":{...},"physical_output":{...},"raw_voltages":{...},"control_state":{...}}

event: data
data: {"timestamp":1704153600100,"physical_input":{...},"physical_output":{...},"raw_voltages":{...},"control_state":{...}}

...
```

✅ **Pass Criteria:**
- Events arrive at configured interval (default 100ms)
- Each event contains all five top-level fields: `timestamp`, `physical_input`, `physical_output`, `raw_voltages`, `control_state`
- Stream continues until client disconnects (Ctrl+C)

## Test 6: Calibration Change Notifications

This is the most important test for the new functionality.

**Setup:**

1. Open PowerShell/CMD
2. Start streaming:
   ```bash
   curl http://localhost:8080/api/sensor-data/stream
   ```
3. Keep this window visible

**Test Steps:**

1. **Open Calibration Factor Dialog** (メニュー → File → Calibration Factor)
2. **Click Zero Adjustment** for Channel 0
3. **Observe the stream** - you should see:
   ```
   event: calibration
   data: {"ad_channels":[...],"da_channels":[...]}
   ```

4. **Try Amplifier Calibration:**
   - Click "Amp-0" button in Calibration Factor dialog
   - Set base point, offset point
   - Click "Update" button
   - Observe another `calibration` event

5. **Manual Coefficient Edit:**
   - Change a coefficient value in the dialog
   - Click Apply/OK
   - Observe `calibration` event

✅ **Pass Criteria:**
- `calibration` event is emitted immediately after calibration changes
- Event data contains updated calibration coefficients
- Multiple changes trigger multiple events
- No events are missed
- Data events continue normally between calibration events

## Test 7: Verify Calibration Data Accuracy

**Manual Verification:**

1. Open Calibration Factor dialog
2. Note the values for Channel 0: `cal_a`, `cal_b`, `cal_c`
3. Query the API:
   ```bash
   curl http://localhost:8080/api/calibration | jq '.ad_channels[] | select(.channel == 0)'
   ```
4. Compare values

✅ **Pass Criteria:** API values exactly match dialog values

## Test 8: OpenAPI Specification

Verify the updated OpenAPI spec:

```bash
# Get as JSON
curl http://localhost:8080/api/openapi.json > openapi.json

# Get as YAML
curl http://localhost:8080/api/openapi.yml > openapi.yaml
```

Check that the specification includes:
- `RawVoltages` schema
- `ControlState` schema
- `CalibrationData` schema
- `/api/calibration` endpoint
- Updated `SensorData` schema with new fields

✅ **Pass Criteria:**
- JSON and YAML formats both work
- Spec validates against OpenAPI 3.0 standard
- All new schemas and endpoints are documented

## Test 9: Integration Test (Python)

Save this as `test_api.py`:

```python
#!/usr/bin/env python3
import requests
import json
import time

def test_all_features():
    base_url = "http://localhost:8080"
    
    # Test 1: Health check
    print("Test 1: Health Check")
    r = requests.get(f"{base_url}/api/health")
    assert r.status_code == 200
    assert r.json()["status"] == "ok"
    print("✓ Passed\n")
    
    # Test 2: Sensor data with new fields
    print("Test 2: Sensor Data Fields")
    r = requests.get(f"{base_url}/api/sensor-data")
    data = r.json()
    assert "raw_voltages" in data
    assert "control_state" in data
    assert "ad_channels" in data["raw_voltages"]
    assert "da_channels" in data["raw_voltages"]
    assert "current_step" in data["control_state"]
    assert "is_running" in data["control_state"]
    assert "elapsed_ms" in data["control_state"]
    print(f"✓ Passed: Found {len(data['raw_voltages']['ad_channels'])} A/D channels")
    print(f"✓ Passed: Found {len(data['raw_voltages']['da_channels'])} D/A channels")
    print(f"✓ Passed: Control state = {data['control_state']}\n")
    
    # Test 3: Calibration endpoint
    print("Test 3: Calibration Endpoint")
    r = requests.get(f"{base_url}/api/calibration")
    assert r.status_code == 200
    cal = r.json()
    assert "ad_channels" in cal
    assert "da_channels" in cal
    print(f"✓ Passed: {len(cal['ad_channels'])} A/D calibrations")
    print(f"✓ Passed: {len(cal['da_channels'])} D/A calibrations\n")
    
    # Test 4: SSE stream
    print("Test 4: SSE Stream (10 seconds)")
    r = requests.get(f"{base_url}/api/sensor-data/stream", stream=True)
    
    data_events = 0
    cal_events = 0
    start_time = time.time()
    
    for line in r.iter_lines():
        if time.time() - start_time > 10:
            break
            
        if line.startswith(b'event: data'):
            data_events += 1
        elif line.startswith(b'event: calibration'):
            cal_events += 1
    
    print(f"✓ Passed: Received {data_events} data events")
    if cal_events > 0:
        print(f"✓ Passed: Received {cal_events} calibration events")
    else:
        print("  Note: No calibration changes during test (expected)")
    
    print("\n" + "="*50)
    print("ALL TESTS PASSED!")
    print("="*50)

if __name__ == "__main__":
    test_all_features()
```

Run:
```bash
python test_api.py
```

✅ **Pass Criteria:** All tests pass without errors

## Performance Test

Monitor CPU and memory usage:

1. Start Windows Task Manager
2. Start DigitShowDST
3. Enable API server
4. Connect 5-10 SSE clients simultaneously
5. Run control sequence

✅ **Pass Criteria:**
- CPU usage remains reasonable (<30% with 10 clients)
- Memory usage is stable (no leaks)
- No lag in UI
- All clients receive updates smoothly

## Error Conditions

Test error handling:

1. **Invalid endpoint:** `curl http://localhost:8080/api/invalid`
   - Should return 404

2. **Server stopped:** Stop application, try to connect
   - Should get connection refused

3. **Rapid calibration changes:** Click zero adjust multiple times quickly
   - Should emit multiple events, no crashes

✅ **Pass Criteria:** No crashes, appropriate error messages

## Regression Tests

Verify existing functionality still works:

1. Load control file
2. Run control sequence
3. Save data files
4. Load calibration from file
5. All basic UI operations

✅ **Pass Criteria:** No regressions introduced

## Final Checklist

- [ ] All 9 tests pass
- [ ] Python integration test passes
- [ ] Performance is acceptable
- [ ] No error conditions cause crashes
- [ ] No regressions in existing features
- [ ] Documentation matches actual behavior
- [ ] OpenAPI spec is complete and accurate

## Troubleshooting

### API Server Won't Start

Check:
- `api_config.json` has `"enabled": true`
- Port 8080 is not already in use
- Check log file for error messages

### Raw Voltages Are All Zero

Check:
- Board is initialized (offline mode detection)
- `Flag_SetBoard` is true
- A/D channels are being read (Timer 1 running)

### No Calibration Events

Check:
- `pDoc` pointer is set in calibration dialogs
- `NotifyCalibrationChanged()` is being called (add breakpoint)
- API server is running when calibration changes

### Control State Not Updating

Check:
- Control is actually running
- Timer 2 is active (500ms control loop)
- `UpdateApiServerData()` is called from Timer 1

## Reporting Issues

If tests fail, please report:
1. Which test failed
2. Expected vs actual behavior
3. Log file contents
4. Screenshots if applicable
5. Steps to reproduce

## Success Criteria

The implementation is successful when:
- All API tests pass
- Calibration notifications work reliably
- Raw voltages match internal arrays
- Control state accurately reflects system state
- No performance degradation
- Documentation is accurate
