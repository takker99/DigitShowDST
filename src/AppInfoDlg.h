/**
 * @file AppInfoDlg.h
 * @brief Application information dialog header
 *
 * Displays git version information and library versions used in the application.
 */

#pragma once

#include "resource.h"
#include <afxwin.h>

/**
 * @class CAppInfoDlg
 * @brief About dialog showing application and library version information
 */
class CAppInfoDlg : public CDialogEx
{
  public:
    /**
     * @brief Constructor
     * @param pParent Parent window pointer (default nullptr)
     */
    explicit CAppInfoDlg(CWnd *pParent = nullptr) noexcept;

    /**
     * @brief Dialog IDD
     */
    enum
    {
        IDD = IDD_AppInfo
    };

  protected:
    /**
     * @brief Initialize dialog
     * @return TRUE if initialization succeeds
     */
    BOOL OnInitDialog() override;

    /**
     * @brief Data exchange for dialog controls
     * @param pDX Data exchange object
     */
    void DoDataExchange(CDataExchange *pDX) override;

    /**
     * @brief Handle Copy Diff button click
     */
    afx_msg void OnCopyDiffToClipboard();

    DECLARE_MESSAGE_MAP()

  private:
    /**
     * @brief Build the complete about text with version information
     * @return Formatted about text
     */
    CString BuildAboutText() const;

    /**
     * @brief Copy text to Windows clipboard
     * @param text Text to copy
     * @return true if successful
     */
    bool CopyToClipboard(const CString &text);
};
