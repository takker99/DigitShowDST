# Calibration format migration

This project now supports a new compact calibration format where channel calibration coefficients are stored as a polynomial-ordered array under `factors`. For A/D (quadratic) the array is `factors: [f0, f1, f2]` and maps to the polynomial

    y = f0 + f1*x + f2*x^2

(i.e., `f0` = constant term, `f1` = linear coefficient, `f2` = quadratic coefficient). For compatibility with previous named fields, this corresponds to `f0 = cal_c`, `f1 = cal_b`, `f2 = cal_a`.

D/A (output) calibration coefficients use the smaller polynomial `factors: [f0, f1]` (i.e., `y = f0 + f1*x`), where `f0` was previously `da_cal_b` and `f1` was previously `da_cal_a`. Legacy fields continue to be accepted during loading for backward compatibility.

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
