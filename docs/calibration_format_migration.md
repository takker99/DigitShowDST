# Calibration Factor Format Migration Guide

## Overview

As of this update, calibration coefficients are stored in an array format instead of individual fields. This change simplifies the data structure and makes it easier to work with calibration data programmatically.

## Format Changes

### Old Format (Deprecated but still supported)
```json
{
  "calibration_data": [
    {
      "channel": 0,
      "cal_a": 0.0,
      "cal_b": 1.0,
      "cal_c": 0.0
    }
  ]
}
```

```yaml
calibration_data:
  - channel: 0
    cal_a: 0.0
    cal_b: 1.0
    cal_c: 0.0
```

### New Format (Recommended)
```json
{
  "calibration_data": [
    {
      "channel": 0,
      "factors": [0.0, 1.0, 0.0]
    }
  ]
}
```

```yaml
calibration_data:
  - channel: 0
    factors: [0.0, 1.0, 0.0]
```

## Backward Compatibility

The application **fully supports both formats** for loading calibration files. This means:

1. ✅ Old calibration files with `cal_a`, `cal_b`, `cal_c` fields will continue to work
2. ✅ New calibration files with `factors` array will work
3. ✅ You can mix old and new format files - the application handles both

## Saving Behavior

When you save calibration data through the application, it will **always use the new array format**. This means:

- New files saved will use `factors: [a, b, c]`
- If you load an old format file and save it, it will be converted to the new format
- This conversion is automatic and transparent

## Migration

### Do I need to migrate my old files?

**No**, you don't need to do anything. Your old files will continue to work perfectly.

### How to migrate to the new format (optional)

If you want to update your old files to the new format for consistency:

1. **Method 1: Use the application**
   - Load your old calibration file
   - Save it again
   - The file will automatically be saved in the new format

2. **Method 2: Manual conversion (Python script)**
   ```python
   import json
   
   def convert_calibration_format(data):
       if "calibration_data" in data:
           for channel in data["calibration_data"]:
               if "cal_a" in channel and "cal_b" in channel and "cal_c" in channel:
                   channel["factors"] = [
                       channel.pop("cal_a"),
                       channel.pop("cal_b"),
                       channel.pop("cal_c")
                   ]
       return data
   
   # Load, convert, and save
   with open("old_file.json", 'r') as f:
       data = json.load(f)
   
   converted = convert_calibration_format(data)
   
   with open("new_file.json", 'w') as f:
       json.dump(converted, f, indent=2)
   ```

## Benefits of the New Format

1. **Simpler structure**: Array notation `[a, b, c]` is more compact than three separate fields
2. **Better for programming**: Easier to iterate over coefficients programmatically
3. **Consistency**: Matches common mathematical notation for coefficients
4. **Future-proof**: Easier to extend to higher-order polynomials if needed

## Technical Details

### Calibration Formula

Both formats represent the same quadratic calibration equation:

```
Physical Value = a*V² + b*V + c
```

Where:
- `a` (factors[0]): Quadratic coefficient
- `b` (factors[1]): Linear coefficient  
- `c` (factors[2]): Constant coefficient
- `V`: Input voltage from A/D converter

### Schema Validation

The JSON schema validates both formats using `oneOf`:
- New format requires `factors` array with exactly 3 elements
- Old format requires `cal_a`, `cal_b`, `cal_c` fields
- Both formats are valid according to the schema

## Questions?

If you encounter any issues with the format migration or have questions:

1. Check that your JSON/YAML syntax is valid
2. Verify the schema validation using: `ajv validate -s schemas/calibration_factor.schema.json -d your_file.json`
3. Open an issue on the GitHub repository with details about the problem
