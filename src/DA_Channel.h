/**
 * @file DA_Channel.h
 * @brief Header file for D/A channel calibration dialog
 *
 * ヘッダー ファイル
 */

#pragma once
#if !defined(AFX_DA_CHANNEL_H__DE7B2C20_A5DC_11D5_B813_0020E0636BD5__INCLUDED_)
#define AFX_DA_CHANNEL_H__DE7B2C20_A5DC_11D5_B813_0020E0636BD5__INCLUDED_

#include "resource.h"
#include <array>

/////////////////////////////////////////////////////////////////////////////
/**
 * @class CDA_Channel
 * @brief Dialog class for D/A channel calibration
 *
 * Manages linear calibration coefficients for D/A output channels.
 * CDA_Channel ダイアログ
 */
class CDA_Channel : public CDialog
{
    // コンストラクション
  public:
    /**
     * @brief Standard constructor
     * @param pParent Pointer to parent window (default NULL)
     */
    CDA_Channel(CWnd *pParent = NULL);

    // ダイアログ データ
    //{{AFX_DATA(CDA_Channel)
    enum
    {
        IDD = IDD_DA_Channel /**< Dialog resource ID */
    };

    // Array size for D/A channels
    static constexpr int CHANNELS_DA = 8; /**< Number of D/A channels */

    // D/A calibration coefficients consolidated into arrays
    std::array<double, CHANNELS_DA> m_DA_Cala{}; /**< D/A linear coefficient a for channels 0-7 */
    std::array<double, CHANNELS_DA> m_DA_Calb{}; /**< D/A linear coefficient b for channels 0-7 */
                                                 //}}AFX_DATA

    // オーバーライド
    // ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CDA_Channel)
  protected:
    /**
     * @brief Data exchange function for DDX/DDV support
     * @param pDX Pointer to CDataExchange object
     */
    void DoDataExchange(CDataExchange *pDX) override;
    //}}AFX_VIRTUAL

    // インプリメンテーション
  protected:
    // 生成されたメッセージ マップ関数
    //{{AFX_MSG(CDA_Channel)
    /**
     * @brief Handle OK button click
     */
    void OnOK() override;
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DA_CHANNEL_H__DE7B2C20_A5DC_11D5_B813_0020E0636BD5__INCLUDED_)
