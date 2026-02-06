/*
 * DigitShowDST - Direct Shear Test Machine Control Software
 * Copyright (C) 2025 Makoto KUNO, Takuto ISHII
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file CalibrationFactor.h
 * @brief Header file for calibration factor dialog
 *
 * ヘッダー ファイル
 */

#pragma once
#if !defined(AFX_CALIBRATIONFACTOR_H__180C3E2C_AEA3_469C_B493_87EF9EE3B75D__INCLUDED_)
#define AFX_CALIBRATIONFACTOR_H__180C3E2C_AEA3_469C_B493_87EF9EE3B75D__INCLUDED_

#include "DigitShowDSTDoc.h"
#include "resource.h"
#include <array>

/////////////////////////////////////////////////////////////////////////////
/**
 * @class CCalibrationFactor
 * @brief Dialog class for A/D channel calibration factors
 *
 * Manages quadratic calibration coefficients (a, b, c) for all A/D channels.
 * CCalibrationFactor ダイアログ
 */
class CCalibrationFactor : public CDialog
{
    // コンストラクション
  public:
    /**
     * @brief Load calibration factors from document
     */
    void CF_Load();

    /**
     * @brief Standard constructor
     * @param pParent Pointer to parent window (default NULL)
     */
    CCalibrationFactor(CWnd *pParent = NULL);

    CDigitShowDSTDoc *pDoc{}; /**< Pointer to document */
    // ダイアログ データ
    //{{AFX_DATA(CCalibrationFactor)
    enum
    {
        IDD = IDD_Calibration_Factor /**< Dialog resource ID */
    };

    // Array sizes for calibration channels
    static constexpr size_t CHANNELS_CAL = 8; /**< Number of calibration channels (ch0-ch7) */
    static constexpr size_t CHANNELS_DA = 8;  /**< Number of D/A channels (ch0-ch7) */

    // Calibration data consolidated into arrays
    std::array<CStringW, CHANNELS_CAL> m_CFP; /**< Physical quantity names for channels 0-7 */

    // Consolidated A/D calibration members: polynomial-ordered
    // m_Cal[i] = {f0, f1, f2} where f0 = constant (c), f1 = linear (b), f2 = quadratic (a)
    std::array<std::array<double, 3>, CHANNELS_CAL> m_Cal{}; /**< Per-channel calibration factors f[0..2] */

    std::array<CStringW, CHANNELS_CAL> m_C; /**< Current voltage values [V] */

    // D/A calibration data consolidated: m_DACal[i] = {f0, f1} where f0 = constant (b), f1 = linear (a)
    std::array<std::array<double, 2>, CHANNELS_DA> m_DACal{}; /**< Per-channel D/A factors f[0..1] */

    // Initial specimen data
    double m_InitSpecHeight{};    /**< Initial specimen height [mm] */
    double m_InitSpecArea{};      /**< Initial specimen area [mm^2] */
    double m_InitSpecWeight{};    /**< Initial specimen weight [g] */
    double m_InitSpecBoxWeight{}; /**< Box weight [g] */
                                  //}}AFX_DATA

    // オーバーライド
    // ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CCalibrationFactor)
  protected:
    /**
     * @brief Data exchange function for DDX/DDV support
     * @param pDX Pointer to CDataExchange object
     */
    void DoDataExchange(CDataExchange *pDX) override;
    //}}AFX_VIRTUAL

    // インプリメンテーション
  private:
  protected:
    // 生成されたメッセージ マップ関数
    //{{AFX_MSG(CCalibrationFactor)
    /** @brief Update document with calibration factors */
    afx_msg void Update();

    /** @brief Zero calibration for channel 0 */
    afx_msg void OnBUTTONZero00();
    afx_msg void OnBUTTONZero01();
    afx_msg void OnBUTTONZero02();
    afx_msg void OnBUTTONZero03();
    afx_msg void OnBUTTONZero04();
    afx_msg void OnBUTTONZero05();
    afx_msg void OnBUTTONZero06();
    /** @brief Zero calibration for channel 7 */
    afx_msg void OnBUTTONZero07();
    /** @brief Amplifier calibration for channel 0 */
    afx_msg void OnBUTTONAmp00();
    afx_msg void OnBUTTONAmp01();
    afx_msg void OnBUTTONAmp02();
    afx_msg void OnBUTTONAmp03();
    afx_msg void OnBUTTONAmp04();
    afx_msg void OnBUTTONAmp05();
    afx_msg void OnBUTTONAmp06();
    /** @brief Amplifier calibration for channel 7 */
    afx_msg void OnBUTTONAmp07();

    /** @brief Save calibration factors to config file */
    afx_msg void OnBUTTONCFSaveConfig();
    /** @brief Load calibration factors from config file */
    afx_msg void OnBUTTONCFLoadConfig();

    /**
     * @brief Initialize dialog
     * @return TRUE if initialization succeeds
     */
    BOOL OnInitDialog() override;

    /** @brief Handle text change events to enable Update button */
    afx_msg void OnEditChange();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

  private:
    /** @brief Enable or disable the Update button */
    void EnableUpdateButton(const bool enable) noexcept;

    /** @brief Apply member variables to global state (calibration arrays, specimen snapshot) */
    void SaveMembersToGlobals() noexcept;

    /** @brief Apply zero adjustment to the specified channel */
    void ApplyZeroAdjustment(const size_t channel) noexcept;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_CALIBRATIONFACTOR_H__180C3E2C_AEA3_469C_B493_87EF9EE3B75D__INCLUDED_)
