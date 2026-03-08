export interface LowPassFilterSeed {
  timestamp: number;
  filteredValue: number;
}

export const lowPassFilter = (
  rawValues: number[],
  timestamps: number[],
  cutoffFrequencyHz: number,
  seed?: LowPassFilterSeed | null,
) => {
  if (!rawValues || rawValues.length === 0) return [];
  if (cutoffFrequencyHz <= 0) return [...rawValues];

  const result: number[] = [];

  if (seed) {
    const dt = timestamps[0] - seed.timestamp;
    if (dt > 0) {
      const RC = 1.0 / (2.0 * Math.PI * cutoffFrequencyHz);
      const alpha = dt / (RC + dt);
      result.push(alpha * rawValues[0] + (1 - alpha) * seed.filteredValue);
    } else {
      result.push(rawValues[0]);
    }
  } else {
    result.push(rawValues[0]);
  }

  for (let i = 1; i < rawValues.length; i++) {
    const dt = timestamps[i] - timestamps[i - 1];
    if (dt <= 0) {
      result.push(rawValues[i]);
      continue;
    }

    const RC = 1.0 / (2.0 * Math.PI * cutoffFrequencyHz);
    const alpha = dt / (RC + dt);
    const filtered = alpha * rawValues[i] + (1 - alpha) * result[i - 1];
    result.push(filtered);
  }

  return result;
};
