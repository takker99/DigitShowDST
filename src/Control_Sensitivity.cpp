/**
 * @file Control_Sensitivity.cpp
 * @brief Implementation of control sensitivity settings dialog
 *
 * インプリメンテーション ファイル
 */
#include "StdAfx.h"

#include "Control_Sensitivity.h"
#include "Sensitivity.hpp"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CControl_Sensitivity ダイアログ

CControl_Sensitivity::CControl_Sensitivity(CWnd *pParent /*=NULL*/)
    : CDialog(CControl_Sensitivity::IDD, pParent), m_ERR_StressA(sensitivity::ERR_StressA),
      m_ERR_StressCom(sensitivity::ERR_StressCom), m_ERR_StressExt(sensitivity::ERR_StressExt)
{
}

void CControl_Sensitivity::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CControl_Sensitivity)
    DDX_Text(pDX, IDC_EDIT_ERR_StressA, m_ERR_StressA);
    DDX_Text(pDX, IDC_EDIT_ERR_StressCom, m_ERR_StressCom);
    DDV_MinMaxDouble(pDX, m_ERR_StressCom, 0., 50.);
    DDX_Text(pDX, IDC_EDIT_ERR_StressExt, m_ERR_StressExt);
    DDV_MinMaxDouble(pDX, m_ERR_StressExt, -50., 0.);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF(CControl_Sensitivity, CDialog)
//{{AFX_MSG_MAP(CControl_Sensitivity)
//}}AFX_MSG_MAP
END_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF()

/////////////////////////////////////////////////////////////////////////////
// CControl_Sensitivity メッセージ ハンドラ

void CControl_Sensitivity::OnOK()
{
    using namespace sensitivity;

    // TODO: この位置にその他の検証用のコードを追加してください
    UpdateData(TRUE);
    ERR_StressA = m_ERR_StressA;
    ERR_StressCom = m_ERR_StressCom;
    ERR_StressExt = m_ERR_StressExt;
    CDialog::OnOK();
}
