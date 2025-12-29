/**
 * @file DA_Vout.h
 * @brief Header file for D/A voltage output dialog
 *
 * ヘッダー ファイル
 */

#pragma once
#if !defined(AFX_DA_VOUT_H__ECF95710_176B_4995_B453_27585B2276F0__INCLUDED_)
#define AFX_DA_VOUT_H__ECF95710_176B_4995_B453_27585B2276F0__INCLUDED_

#include "DigitShowBasicDoc.h"
#include <array>

/////////////////////////////////////////////////////////////////////////////
/**
 * @class CDA_Vout
 * @brief Dialog class for D/A voltage output control
 *
 * Allows manual control of D/A output channels with voltage values.
 * CDA_Vout ダイアログ
 */
class CDA_Vout : public CDialog
{
    // コンストラクション
  public:
    /**
     * @brief Standard constructor
     * @param pParent Pointer to parent window (default NULL)
     */
    CDA_Vout(CWnd *pParent = NULL);

    CDigitShowBasicDoc *pDoc{}; /**< Pointer to document */
    // ダイアログ データ
    //{{AFX_DATA(CDA_Vout)
    enum
    {
        IDD = IDD_DA_Vout /**< Dialog resource ID */
    };

    // Array size for D/A voltage channels
    static constexpr int CHANNELS_DAVOUT = 16; /**< Number of D/A voltage output channels */

    // D/A voltage outputs consolidated into array
    std::array<float, CHANNELS_DAVOUT> m_DAVout{}; /**< D/A voltage output [V] for channels 1-16 */
                                                   //}}AFX_DATA

    // オーバーライド
    // ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CDA_Vout)
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
    //{{AFX_MSG(CDA_Vout)
    /** @brief Execute D/A voltage output with current values */
    afx_msg void OnBUTTONDAVout();

    /** @brief Refresh dialog with current document values */
    afx_msg void OnBUTTONReflesh();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DA_VOUT_H__ECF95710_176B_4995_B453_27585B2276F0__INCLUDED_)
