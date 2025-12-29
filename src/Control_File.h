/**
 * @file Control_File.h
 * @brief Header file for file-based control script dialog
 *
 * ヘッダー ファイル
 */

#pragma once
#if !defined(AFX_CONTROL_FILE_H__EF6A40AB_D157_4D17_BCC0_884EC690BC71__INCLUDED_)
#define AFX_CONTROL_FILE_H__EF6A40AB_D157_4D17_BCC0_884EC690BC71__INCLUDED_

#include "resource.h"
#include <array>

/////////////////////////////////////////////////////////////////////////////
/**
 * @class CControl_File
 * @brief Dialog class for control script file management
 *
 * Manages loading, editing, and executing .ctl control script files.
 * CControl_File ダイアログ
 */
class CControl_File : public CDialog
{
    // コンストラクション
  public:
    /**
     * @brief Standard constructor
     * @param pParent Pointer to parent window (default NULL)
     */
    CControl_File(CWnd *pParent = NULL);

    // ダイアログ データ
    //{{AFX_DATA(CControl_File)
    enum
    {
        IDD = IDD_Control_File /**< Dialog resource ID */
    };

    // Array size for control file parameters - now includes all ControlParams fields
    static constexpr size_t CFPARA_COUNT = 25; /**< Number of control file parameters (0-25) */

    // Control file parameters consolidated into array
    std::array<double, CFPARA_COUNT> m_CFPARA{}; /**< Control file parameters 0-25 */

    size_t m_StepNum{};       /**< Total number of steps in script */
    size_t m_SCFNum{};        /**< Selected control file number (CFNUM) */
    size_t m_CurNum{};        /**< Current step number */
    size_t m_CFNum{};         /**< Control function number (CFNUM) */
    CStringW m_CFLabel;       /**< Control pattern label (use field) */
    CStringW m_CurLabel;      /**< Current control pattern label */
    CStringW m_PatternDesc;   /**< Description of selected control pattern */
    CStringW m_Description;   /**< Description field from ControlParams */
    CComboBox m_ComboPattern; /**< Combo box for pattern selection */
                              //}}AFX_DATA

    // オーバーライド
    // ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CControl_File)
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
    //{{AFX_MSG(CControl_File)
    /** @brief Update document with current step parameters */
    afx_msg void OnBUTTONUpdate();
    /** @brief Load current step from arrays into UI */
    afx_msg void OnBUTTONLoad();
    /** @brief Read control script from JSON */
    afx_msg void OnBUTTONReadJSON();
    /** @brief Save control script to JSON */
    afx_msg void OnBUTTONSaveJSON();
    afx_msg void OnCHECKChangeNo();

    /** @brief Decrement current step number */
    afx_msg void OnBUTTONStepDec();

    /** @brief Increment current step number */
    afx_msg void OnBUTTONStepInc();

    /** @brief Handle control pattern selection change */
    afx_msg void OnSelChangeControlPattern();

    /**
     * @brief Initialize dialog
     * @return TRUE if initialization succeeds
     */
    BOOL OnInitDialog() override;
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

  private:
    /** @brief Update pattern description based on current combo box selection */
    void UpdatePatternDescription();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_CONTROL_FILE_H__EF6A40AB_D157_4D17_BCC0_884EC690BC71__INCLUDED_)
