# REST API Enhancements

## Overview

This document describes the REST API enhancements for DigitShowDST, including raw voltage streaming, control state information, calibration data access, and automatic change notifications.

## Recent Additions (2026-01-22)

### 1. Raw Voltage Data Streaming

The API now streams raw A/D and D/A voltage values alongside physical measurements.

#### Data Structure

```json
{
  "raw_voltages": {
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
}
```

#### Usage

```bash
# Get snapshot with raw voltages
curl http://localhost:8080/api/sensor-data | jq '.raw_voltages'

# Stream raw voltages in real-time
curl http://localhost:8080/api/sensor-data/stream
```

#### Use Cases
- Sensor diagnostics and validation
- Calibration work
- Debugging signal processing
- Raw data archival

### 2. Control State Information

Real-time control loop state is now included in the API responses.

#### Data Structure

```json
{
  "control_state": {
    "current_step": 3,
    "is_running": true,
    "elapsed_ms": 15234
  }
}
```

#### Fields
- `current_step`: Current control step index (0-based)
- `is_running`: Whether control is actively running
- `elapsed_ms`: Time elapsed in current step (milliseconds)

#### Usage Example

```javascript
const eventSource = new EventSource('http://localhost:8080/api/sensor-data/stream');

eventSource.addEventListener('data', (event) => {
    const data = JSON.parse(event.data);
    const ctrl = data.control_state;
    
    console.log(`Step ${ctrl.current_step}: ${ctrl.is_running ? 'RUNNING' : 'STOPPED'}`);
    console.log(`Elapsed: ${(ctrl.elapsed_ms / 1000).toFixed(1)}s`);
});
```

#### Use Cases
- Monitor test progress
- Detect step transitions
- Timeline visualization
- Automated test completion detection

### 3. Calibration Data API

New endpoint provides access to A/D and D/A calibration coefficients.

#### Endpoint

```
GET /api/calibration
```

#### Response Example

```json
{
  "ad_channels": [
    {
      "channel": 0,
      "cal_a": 0.0,
      "cal_b": 100.5,
      "cal_c": -0.5
    },
    {
      "channel": 1,
      "cal_a": 0.001,
      "cal_b": 50.2,
      "cal_c": 0.0
    }
  ],
  "da_channels": [
    {
      "channel": 3,
      "cal_a": 0.017854906,
      "cal_b": -0.286962967
    }
  ]
}
```

#### Calibration Formulas

**A/D Channels (Quadratic)**:
```
Physical Value = cal_a × V² + cal_b × V + cal_c
```

**D/A Channels (Linear)**:
```
Physical Value = cal_a × V + cal_b
```

#### Usage Example

```python
import requests

# Get calibration data
response = requests.get('http://localhost:8080/api/calibration')
calibration = response.json()

# Display A/D channel 0 coefficients
for ch in calibration['ad_channels']:
    if ch['channel'] == 0:
        print(f"Ch0: a={ch['cal_a']}, b={ch['cal_b']}, c={ch['cal_c']}")
```

#### Use Cases
- Verify calibration settings
- Compute physical values externally
- Backup calibration data
- Compare calibrations across instruments

### 4. Calibration Change Notifications

The SSE stream emits `calibration` events when calibration data is modified.

#### Event Format

```
event: calibration
data: {"ad_channels": [...], "da_channels": [...]}
```

#### Usage Example

```javascript
const eventSource = new EventSource('http://localhost:8080/api/sensor-data/stream');

// Data events
eventSource.addEventListener('data', (event) => {
    const data = JSON.parse(event.data);
    updateChart(data);
});

// Calibration change events
eventSource.addEventListener('calibration', (event) => {
    const calibration = JSON.parse(event.data);
    console.log('Calibration updated!');
    
    // Show notification
    showNotification('Calibration data changed. Please reload.');
    
    // Update calibration display
    updateCalibrationInfo(calibration);
});
```

#### Trigger Conditions

Calibration events are emitted when:
1. Zero adjustment buttons are clicked in Calibration Factor dialog
2. Coefficients are manually edited and applied
3. Two-point calibration is performed in Amplifier Calibration dialog

#### Use Cases
- Real-time calibration change alerts
- Data integrity assurance
- Event logging
- External system synchronization

## Complete Integration Example

### Python Real-Time Monitoring

```python
#!/usr/bin/env python3
"""Complete monitoring with all new features"""

import requests
import json
from datetime import datetime
import csv

def monitor_stream():
    csv_file = open('sensor_log.csv', 'w', newline='')
    writer = csv.writer(csv_file)
    writer.writerow([
        'Timestamp', 
        'Step', 
        'Running', 
        'Elapsed(s)',
        'Shear_Stress(kPa)', 
        'AD0_Voltage(V)',
        'DA3_Voltage(V)'
    ])
    
    response = requests.get(
        'http://localhost:8080/api/sensor-data/stream',
        stream=True
    )
    
    for line in response.iter_lines():
        if line.startswith(b'event: data'):
            data_line = next(response.iter_lines())
            if data_line.startswith(b'data: '):
                data = json.loads(data_line[6:])
                
                ctrl = data['control_state']
                phys = data['physical_input']
                
                # Extract specific voltages
                ad0 = next((ch['voltage'] for ch in data['raw_voltages']['ad_channels'] 
                           if ch['channel'] == 0), 0.0)
                da3 = next((ch['voltage'] for ch in data['raw_voltages']['da_channels'] 
                           if ch['channel'] == 3), 0.0)
                
                timestamp = datetime.fromtimestamp(data['timestamp']/1000)
                print(f"[{timestamp:%H:%M:%S}] Step {ctrl['current_step']} "
                      f"({'RUN' if ctrl['is_running'] else 'STOP'}): "
                      f"τ={phys['shear_stress_kpa']:.2f} kPa")
                
                writer.writerow([
                    timestamp.isoformat(),
                    ctrl['current_step'],
                    ctrl['is_running'],
                    ctrl['elapsed_ms'] / 1000,
                    phys['shear_stress_kpa'],
                    ad0,
                    da3
                ])
                csv_file.flush()
        
        elif line.startswith(b'event: calibration'):
            print("\n⚠️  CALIBRATION CHANGED ⚠️\n")

if __name__ == '__main__':
    print("Starting monitoring (Ctrl+C to stop)")
    try:
        monitor_stream()
    except KeyboardInterrupt:
        print("\nMonitoring stopped")
```

## Previous Features (Still Available)

### OpenAPI Specification Endpoint

The REST API now serves its OpenAPI 3.0 specification directly via HTTP.

#### Endpoint

```
GET /api/openapi
```

#### Response

- **Content-Type**: `application/x-yaml; charset=utf-8`
- **Body**: OpenAPI 3.0 specification in YAML format

#### Example Usage

```bash
# Fetch the OpenAPI specification
curl http://localhost:8080/api/openapi

# Save to a file
curl http://localhost:8080/api/openapi -o openapi.yaml
```

#### Implementation Details

- The specification is embedded at build time as a C++ `constexpr std::string_view` in `src/openapi_spec.hpp`
- No disk I/O required to serve the specification
- Content is synchronized with `docs/api/openapi.yaml`
- The specification documents all available API endpoints including itself

### 2. Automatic Configuration File Generation

The API server now automatically creates `api_config.json` with default values if it doesn't exist.

#### Behavior

When `ApiServer::load_config("api_config.json")` is called:

1. **File exists**: Loads configuration from the file
2. **File missing**:
   - Creates `api_config.json` in the same directory as the executable
   - Populates with default values from the `ApiConfig` struct
   - Logs the creation
   - Returns default configuration

#### Default Configuration

```json
{
  "$schema": "schemas/api_config.schema.json",
  "enabled": false,
  "host": "127.0.0.1",
  "port": 8080,
  "update_interval_ms": 100,
  "cors_enabled": true,
  "max_connections": 10
}
```

#### Benefits

- **First-run experience**: No manual configuration file creation needed
- **Self-documenting**: Generated file includes JSON schema reference
- **Fail-safe**: Application continues with defaults even if file creation fails
- **Standard location**: Configuration file is created next to the executable

#### Implementation Details

- Uses `std::filesystem::exists()` to check for file existence
- Uses `std::ofstream` for file writing
- Pretty-prints JSON with 2-space indentation
- All file operations use standard C++ library (no MFC dependencies)
- Comprehensive logging via spdlog

## API Endpoints Summary

After these enhancements, the REST API provides:

| Endpoint | Method | Description | Content-Type |
|----------|--------|-------------|--------------|
| `/api/health` | GET | Health check | `application/json` |
| `/api/openapi` | GET | OpenAPI specification (JSON) | `application/json` |
| `/api/openapi.json` | GET | OpenAPI specification (JSON) | `application/json` |
| `/api/openapi.yml` | GET | OpenAPI specification (YAML) | `application/x-yaml` |
| `/api/sensor-data` | GET | Current sensor data snapshot (with raw voltages & control state) | `application/json` |
| `/api/sensor-data/stream` | GET | Real-time SSE data stream (data + calibration events) | `text/event-stream` |
| `/api/calibration` | GET | Calibration coefficients | `application/json` |

## Configuration Schema

The `api_config.json` file follows the schema defined in `schemas/api_config.schema.json`:

```json
{
  "enabled": boolean,           // Enable/disable the API server
  "host": string,               // Bind address (e.g., "127.0.0.1")
  "port": integer,              // Port number (1024-65535)
  "update_interval_ms": integer,// SSE update interval (10-10000 ms)
  "cors_enabled": boolean,      // Enable CORS headers
  "max_connections": integer    // Max concurrent connections (1-100)
}
```

## Usage Examples

### Enabling the API Server

1. Set `"enabled": true` in `api_config.json`
2. Optionally adjust host, port, and other settings
3. Restart the application

### Accessing the OpenAPI Specification

```bash
# View in terminal
curl http://localhost:8080/api/openapi

# Import into Swagger Editor
curl http://localhost:8080/api/openapi | swagger-editor

# Generate client code
curl http://localhost:8080/api/openapi -o api.yaml
openapi-generator generate -i api.yaml -g python -o ./client
```

### Integrating with API Tools

The OpenAPI specification can be used with various tools:

- **Swagger UI**: Interactive API documentation
- **Postman**: Import and test endpoints
- **OpenAPI Generator**: Generate client libraries
- **VS Code REST Client**: Use with the REST Client extension

## Technical Notes

### Build-Time Embedding

The OpenAPI specification is embedded at compile time:

```cpp
namespace api {
constexpr std::string_view kOpenApiYaml = R"(
openapi: 3.0.3
info:
  title: DigitShowDST Sensor Data API
  ...
)";
}
```

**Advantages**:
- Zero runtime overhead
- No file system dependencies
- Cannot be accidentally deleted or corrupted
- Always synchronized with build

**Updating the Spec**:
1. Edit `docs/api/openapi.yaml`
2. Update `src/openapi_spec.hpp` with the same content
3. Rebuild the application

### File System Operations

All file operations use standard C++ libraries:

```cpp
#include <filesystem>
#include <fstream>

// Check existence
std::filesystem::exists(path)

// Create file
std::ofstream out_file(path)
```

No MFC or Windows-specific APIs are used, maintaining cross-platform compatibility and modern C++ practices.

## Error Handling

### Configuration Loading

- **File not found**: Creates default, logs warning
- **Parse error**: Uses defaults, logs error
- **Write failure**: Uses defaults, logs error

All error conditions are handled gracefully without crashing the application.

### OpenAPI Endpoint

- **Exception during serving**: Returns HTTP 500 with error JSON
- **Invalid request**: Standard HTTP error handling via cpp-httplib

## Logging

The implementation provides detailed logging:

```
[info] API config file not found: api_config.json. Creating with default values.
[info] Created default API config file: api_config.json
[info] Loaded API config from: api_config.json
[debug] Served OpenAPI specification
```

Log levels:
- `info`: Normal operations (config load, file creation)
- `warn`: Non-critical issues (file not found)
- `error`: Failures (parse errors, write failures)
- `debug`: Detailed operation traces (spec served)

## Future Enhancements

Potential improvements for future versions:

1. **Content Negotiation**: Serve JSON format via Accept header
2. **Versioned Specs**: Support multiple API versions
3. **Dynamic Generation**: Generate spec from C++ code annotations
4. **Hot Reload**: Reload configuration without restart
5. **Web UI**: Embedded Swagger UI for interactive documentation

## References

- OpenAPI 3.0 Specification: https://spec.openapis.org/oas/v3.0.3
- cpp-httplib Documentation: https://github.com/yhirose/cpp-httplib
- API Schema: `schemas/api_config.schema.json`
- Original Spec: `docs/api/openapi.yaml`
- Implementation: `src/ApiServer.cpp`, `src/openapi_spec.hpp`
