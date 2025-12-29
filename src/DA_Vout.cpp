/**
 * @file DA_Vout.cpp
 * @brief Implementation of D/A voltage output dialog
 *
 * インプリメンテーション ファイル
 */

#include "StdAfx.h"
#include "resource.h"

#include "DA_Vout.h"
#include "Variables.hpp"

using namespace variables;

/////////////////////////////////////////////////////////////////////////////
// CDA_Vout ダイアログ

CDA_Vout::CDA_Vout(CWnd *pParent /*=NULL*/) : CDialog(CDA_Vout::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDA_Vout)

    //}}AFX_DATA_INIT

    // Initialize array with current DAVout values (channels 1-8 are used)
    for (size_t i = 0; i < 8; ++i)
    {
        m_DAVout[i] = DAVout[i];
    }
}

void CDA_Vout::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDA_Vout)

    // Resource ID array for DDX_Text loop (only channels 1-8 are used)
    static constexpr std::array<int, 8> IDS_DAVOUT = {{IDC_EDIT_DAVout01, IDC_EDIT_DAVout02, IDC_EDIT_DAVout03,
                                                       IDC_EDIT_DAVout04, IDC_EDIT_DAVout05, IDC_EDIT_DAVout06,
                                                       IDC_EDIT_DAVout07, IDC_EDIT_DAVout08}};

    // Use loop for array-based DDX
    for (size_t i = 0; i < 8; ++i)
    {
        DDX_Text(pDX, IDS_DAVOUT[i], m_DAVout[i]);
    }
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF(CDA_Vout, CDialog)
//{{AFX_MSG_MAP(CDA_Vout)
ON_BN_CLICKED(IDC_BUTTON_DA_Vout, &CDA_Vout::OnBUTTONDAVout)
ON_BN_CLICKED(IDC_BUTTON_Reflesh, &CDA_Vout::OnBUTTONReflesh)
//}}AFX_MSG_MAP
END_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF()

/////////////////////////////////////////////////////////////////////////////
// CDA_Vout メッセージ ハンドラ

void CDA_Vout::OnBUTTONDAVout()
{
    // TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
    UpdateData(TRUE);
    for (size_t i = 0; i < 8; ++i)
    {
        DAVout[i] = m_DAVout[i];
    }
    pDoc->DA_OUTPUT();
}

void CDA_Vout::OnBUTTONReflesh()
{
    // TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
    for (size_t i = 0; i < 8; ++i)
    {
        m_DAVout[i] = DAVout[i];
    }
    UpdateData(FALSE);
}
