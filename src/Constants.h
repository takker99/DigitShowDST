/**
 * @file Constants.h
 * @brief Small, scoped constants to reduce magic numbers
 *
 * This header introduces inline constexpr values to replace timer IDs
 * and common unit factors in a non-invasive way.
 */

#pragma once

/**
 * @namespace dsb
 * @brief DigitShowBasic namespace containing constants and utilities
 */
namespace dsb
{
/** @brief The Number of A/D Board ( NUMAD=1-2 ) */
inline constexpr int NUMAD = 1;
/** @brief The Number of D/A Board ( NUMDA=1 ) */
inline constexpr int NUMDA = 1;

} // namespace dsb
