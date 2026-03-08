import type uPlot from "uplot";

export const formatAxisValue: uPlot.Axis.DynamicValues = (_, splits) => {
  // 目盛り値の最小差を検出して、それが表現できる桁数を決定
  const validSplits = splits.filter((v) => v !== null);
  if (validSplits.length === 0) return splits.map(() => null);

  // 隣接する値の差の最小値を計算
  let minDiff = Infinity;
  for (let i = 1; i < validSplits.length; i++) {
    const diff = Math.abs(validSplits[i] - validSplits[i - 1]);
    if (diff > 0) minDiff = Math.min(minDiff, diff);
  }

  // 最小差が表現できる小数点以下桁数を計算
  let decimalPlaces = 0;
  if (minDiff !== Infinity && minDiff > 0) {
    decimalPlaces = Math.max(0, Math.ceil(-Math.log10(minDiff)));
  }

  // 全体の値の範囲を確認して指数表記が必要か判断
  const maxAbsValue = Math.max(...validSplits.map(Math.abs));
  const useExponential = maxAbsValue >= 1e5 ||
    (maxAbsValue > 0 && maxAbsValue < 1e-4);

  return splits.map((value) => {
    if (value === null) return null;
    if (value === 0) return "0";

    if (useExponential) {
      return value.toExponential(Math.max(2, Math.min(decimalPlaces, 4)));
    }

    // 小数点以下の桁数を最大10桁に制限（可読性のため）
    return value.toFixed(Math.min(decimalPlaces, 10));
  });
};
