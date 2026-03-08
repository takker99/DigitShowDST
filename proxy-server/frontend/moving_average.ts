export const movingAverage = (
  rawValues: number[],
  timestamps: number[],
  windowMs: number,
) => {
  if (!rawValues || rawValues.length === 0) return [];
  if (windowMs <= 0) return [...rawValues];

  const windowSec = windowMs / 1000;
  const result = new Array(rawValues.length);

  for (let i = 0; i < rawValues.length; i++) {
    const centerTime = timestamps[i];
    const windowStart = centerTime - windowSec / 2;
    const windowEnd = centerTime + windowSec / 2;

    let sum = 0;
    let count = 0;
    let startIdx = 0;
    let endIdx = -1;

    for (let j = 0; j < timestamps.length; j++) {
      if (timestamps[j] >= windowStart) startIdx = j;
      if (timestamps[j] <= windowEnd) endIdx = j;
      else if (timestamps[j] > windowEnd) break;
    }

    if (endIdx >= startIdx && startIdx >= 0) {
      for (let j = startIdx; j <= endIdx; j++) {
        sum += rawValues[j];
        count++;
      }
      result[i] = count > 0 ? sum / count : rawValues[i];
    } else {
      result[i] = rawValues[i];
    }
  }
  return result;
};
