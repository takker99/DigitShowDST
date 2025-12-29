/**
 * @file DigitShowBasic.h
 * @brief Main header file for DigitShowBasic application
 *
 * DIGITSHOWBASIC アプリケーションのメイン ヘッダー ファイル
 */

#pragma once
#if !defined(AFX_DIGITSHOWBASIC_H__022B46E0_9575_4547_950E_44664C9C23DA__INCLUDED_)
#define AFX_DIGITSHOWBASIC_H__022B46E0_9575_4547_950E_44664C9C23DA__INCLUDED_

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h" // メイン シンボル

/////////////////////////////////////////////////////////////////////////////
/**
 * @class CDigitShowBasicApp
 * @brief Main application class for DigitShowBasic
 *
 * CDigitShowBasicApp:
 * このクラスの動作の定義に関しては DigitShowBasic.cpp ファイルを参照してください。
 */
class CDigitShowBasicApp : public CWinApp
{
  public:
    /**
     * @brief Default constructor
     */
    CDigitShowBasicApp() noexcept = default;

    // オーバーライド
    // ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CDigitShowBasicApp)
  public:
    /**
     * @brief Initialize application instance
     * @return TRUE if initialization succeeds
     */
    BOOL InitInstance() override;
    //}}AFX_VIRTUAL

    // インプリメンテーション
    //{{AFX_MSG(CDigitShowBasicApp)
    // メモ - ClassWizard はこの位置にメンバ関数を追加または削除します。
    //        この位置に生成されるコードを編集しないでください。
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DIGITSHOWBASIC_H__022B46E0_9575_4547_950E_44664C9C23DA__INCLUDED_)
