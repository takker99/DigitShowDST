#pragma once

namespace sensitivity
{

inline double ERR_StressCom = 0.5; // kPa  これをERR_StressMを変えるためのパラメータとした。
inline double ERR_StressA = 0.5;   // (kPa) これをERR_StressPを変えるためのパラメータとした。
inline double ERR_StressExt =
    0.004; // (kPa)　「三軸プログラムにおけるsensitivityであるERR_StressCom」をモーター載荷におけるERR_StressMを変えるためのパラメータとした。

} // namespace sensitivity
