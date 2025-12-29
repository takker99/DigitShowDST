/**
 * @file Control_Sensitivity.h
 * @brief Header file for control sensitivity settings dialog
 *
 * ヘッダー ファイル
 */

#pragma once
#include "resource.h"

#if !defined(AFX_CONTROL_SENSITIVITY_H__C6C8A25D_F321_4CD4_AB7F_76D1A0367063__INCLUDED_)
#define AFX_CONTROL_SENSITIVITY_H__C6C8A25D_F321_4CD4_AB7F_76D1A0367063__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
/**
 * @class CControl_Sensitivity
 * @brief Dialog class for control sensitivity parameters
 *
 * Manages error thresholds for stress control feedback loops.
 * CControl_Sensitivity ダイアログ
 */
class CControl_Sensitivity : public CDialog
{
    // コンストラクション
  public:
    /**
     * @brief Standard constructor
     * @param pParent Pointer to parent window (default NULL)
     */
    CControl_Sensitivity(CWnd *pParent = NULL);

    // ダイアログ データ
    //{{AFX_DATA(CControl_Sensitivity)
    enum
    {
        IDD = IDD_Control_Sensitivity /**< Dialog resource ID */
    };
    double m_ERR_StressA;   /**< Stress error threshold for axial control [kPa] */
    double m_ERR_StressCom; /**< Stress error threshold for compression [kPa] */
    double m_ERR_StressExt; /**< Stress error threshold for extension [kPa] */
                            //}}AFX_DATA

    // オーバーライド
    // ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CControl_Sensitivity)
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
    //{{AFX_MSG(CControl_Sensitivity)
    /**
     * @brief Handle OK button click
     */
    void OnOK() override;
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_CONTROL_SENSITIVITY_H__679A7459_BCF0_43C8_A9B5_9D7663AB1B5B__INCLUDED_)
