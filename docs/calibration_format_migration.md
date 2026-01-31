# Calibration format migration

This project now supports a new compact calibration format where A/D channel calibration coefficients are stored as an array under `factors: [a, b, c]` representing the quadratic coefficients used as `Physical = a * V^2 + b * V + c`. D/A (output) calibration coefficients likewise support a `factors: [a, b]` array (Output = a * Physical + b), while legacy fields remain supported for backward compatibility.

Compatibility:
- The loader accepts both the new `factors` array and the legacy `cal_a`, `cal_b`, `cal_c` fields. Files written by the application use the new `factors` array format.

Examples:

New format (preferred):

```yaml
calibration_data:
  - channel: 0
    factors: [0.0, 1.0, 0.0]
```

Old format (still accepted):

```yaml
calibration_data:
  - channel: 0
    cal_a: 0.0
    cal_b: 1.0
    cal_c: 0.0
```

If you maintain automation that inspects calibration files, prefer the `factors` array moving forward. Legacy files will continue to be loaded successfully.
