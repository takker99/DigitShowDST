/**
 * @file DigitShowBasicDoc.h
 * @brief Declaration and interface definition for CDigitShowBasicDoc class
 *
 * CDigitShowBasicDoc クラスの宣言およびインターフェイスの定義をします。
 */
/////////////////////////////////////////////////////////////////////////////

#pragma once
#if !defined(AFX_DIGITSHOWBASICDOC_H__0F5B25DB_9338_44C6_9841_265C5A221957__INCLUDED_)
#define AFX_DIGITSHOWBASICDOC_H__0F5B25DB_9338_44C6_9841_265C5A221957__INCLUDED_

#include "TsvWriter.hpp"
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>

/**
 * @class CDigitShowBasicDoc
 * @brief Main document class for DigitShowBasic application
 *
 * Manages test data, hardware control, data logging, and test execution.
 * シリアライズ機能のみから作成します。
 */
class CDigitShowBasicDoc : public CDocument
{
  protected:
    /** @brief Default constructor (created only from serialization) */
    CDigitShowBasicDoc() noexcept = default;
    DECLARE_DYNCREATE(CDigitShowBasicDoc)

    // アトリビュート
  public:
    // オペレーション
  public:
    // オーバーライド
    //  ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CDigitShowBasicDoc)
  public:
    /** @brief Create new document */
    BOOL OnNewDocument() override;

    /** @brief Serialize document data */
    void Serialize(CArchive &ar) override;
    //}}AFX_VIRTUAL

    // インプリメンテーション
  public:
    /** @brief Stop all control operations */
    void Stop_Control();

    /** @brief Start control operations */
    void Start_Control();

    /** @brief Close hardware board connection */
    void CloseBoard();

    /** @brief Open hardware board connection */
    void OpenBoard();

    /** @brief Save data to file (periodic logging) */
    void SaveToFile();

    /** @brief Execute D/A control logic */
    void Control_DA();

    /** @brief Calculate derived parameters (p', q, strain, etc.) */
    void Cal_Param();

    /** @brief Output D/A values to hardware */
    void DA_OUTPUT();

    /** @brief Read A/D input values from hardware */
    void AD_INPUT();

    /**
     * @brief Open TSV writers for data logging
     * @param basePath Base file path (writers will append _vlt.tsv, .tsv, _out.tsv)
     * @return true if all writers opened successfully, false otherwise
     */
    bool OpenSaveWriters(const std::filesystem::path &basePath);

    /**
     * @brief Close all TSV writers
     */
    void CloseSaveWriters() noexcept;

    /**
     * @brief Flush TSV writers if needed (on control step change or after 1 minute)
     *
     * Should be called periodically during control recording. Flushes when:
     * - Control step number (CURNUM) changes
     * - More than 1 minute has elapsed since last flush
     */
    void FlushWritersIfNeeded();

    /**
     * @brief Force flush all TSV writers to disk
     */
    void FlushWriters();

    /**
     * @brief Reset flush tracking state (should be called when starting recording)
     */
    void ResetFlushState() noexcept;

    /** @brief Destructor */
    ~CDigitShowBasicDoc() override = default;
#ifdef _DEBUG
    virtual void AssertValid() const override;
    virtual void Dump(CDumpContext &dc) const override;
#endif

  protected:
    // 生成されたメッセージ マップ関数
  private:
    /** @brief TSV writer for voltage data (_vlt.tsv) */
    TsvWriter m_vltWriter;

    /** @brief TSV writer for physical data (.tsv) */
    TsvWriter m_phyWriter;

    /** @brief TSV writer for parameter data (_out.tsv) */
    TsvWriter m_paramWriter;

    /** @brief Reusable scratch buffer for formatting to avoid repeated allocations */
    std::string m_writeScratch;

    /** @brief Last control step number when flush was performed */
    std::size_t m_lastFlushedCurnum = 0;

    /** @brief Time point of last flush operation */
    std::chrono::steady_clock::time_point m_lastFlushTime{};

  protected:
    //{{AFX_MSG(CDigitShowBasicDoc)
    // メモ - ClassWizard はこの位置にメンバ関数を追加または削除します。
    //        この位置に生成されるコードを編集しないでください。
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DIGITSHOWBASICDOC_H__0F5B25DB_9338_44C6_9841_265C5A221957__INCLUDED_)
