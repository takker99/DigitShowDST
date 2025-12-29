/**
 * @file CalibrationFactor.cpp
 * @brief Implementation of calibration factor dialog
 *
 * インプリメンテーション ファイル
 */
#include "StdAfx.h"
#pragma warning(push)
#pragma warning(disable : 4800 4866)

#include "Board.hpp"
#include "CalibrationFactor.h"
#include "Variables.hpp"
#include "resource.h"

#include "CalibrationAmp.h"
#include "ui_helpers.hpp"
#include <format>
#include <fstream>
#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>
#include <spdlog/spdlog.h>
#include <sstream>

using namespace variables;

/////////////////////////////////////////////////////////////////////////////
// CCalibrationFactor ダイアログ
// CDigitShowBasicView

CCalibrationFactor::CCalibrationFactor(CWnd *pParent /*=NULL*/)
    : CDialog(CCalibrationFactor::IDD, pParent), m_CFP{}, m_C{}
{
    //{{AFX_DATA_INIT(CCalibrationFactor)

    //}}AFX_DATA_INIT
}

void CCalibrationFactor::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCalibrationFactor)

    // Resource ID arrays for DDX_Text loops (ch0-ch7 only)
    static constexpr int IDS_CFP[CHANNELS_CAL] = {IDC_EDIT_CFP00, IDC_EDIT_CFP01, IDC_EDIT_CFP02, IDC_EDIT_CFP03,
                                                  IDC_EDIT_CFP04, IDC_EDIT_CFP05, IDC_EDIT_CFP06, IDC_EDIT_CFP07};

    static constexpr int IDS_CFA[CHANNELS_CAL] = {IDC_EDIT_CFA00, IDC_EDIT_CFA01, IDC_EDIT_CFA02, IDC_EDIT_CFA03,
                                                  IDC_EDIT_CFA04, IDC_EDIT_CFA05, IDC_EDIT_CFA06, IDC_EDIT_CFA07};

    static constexpr int IDS_CFB[CHANNELS_CAL] = {IDC_EDIT_CFB00, IDC_EDIT_CFB01, IDC_EDIT_CFB02, IDC_EDIT_CFB03,
                                                  IDC_EDIT_CFB04, IDC_EDIT_CFB05, IDC_EDIT_CFB06, IDC_EDIT_CFB07};

    static constexpr int IDS_CFC[CHANNELS_CAL] = {IDC_EDIT_CFC00, IDC_EDIT_CFC01, IDC_EDIT_CFC02, IDC_EDIT_CFC03,
                                                  IDC_EDIT_CFC04, IDC_EDIT_CFC05, IDC_EDIT_CFC06, IDC_EDIT_CFC07};

    static constexpr int IDS_C[CHANNELS_CAL] = {IDC_STATIC_C00, IDC_STATIC_C01, IDC_STATIC_C02, IDC_STATIC_C03,
                                                IDC_STATIC_C04, IDC_STATIC_C05, IDC_STATIC_C06, IDC_STATIC_C07};

    // Use loops for array-based DDX
    for (size_t i = 0; i < CHANNELS_CAL; ++i)
    {
        DDX_Text(pDX, IDS_CFP[i], m_CFP[i]);
        DDX_Text(pDX, IDS_CFA[i], m_CFA[i]);
        DDX_Text(pDX, IDS_CFB[i], m_CFB[i]);
        DDX_Text(pDX, IDS_CFC[i], m_CFC[i]);
        DDX_Text(pDX, IDS_C[i], m_C[i]);
    }

    // Initial specimen data
    DDX_Text(pDX, IDC_EDIT_InitSpecHeight, m_InitSpecHeight);
    DDX_Text(pDX, IDC_EDIT_InitSpecArea, m_InitSpecArea);
    DDX_Text(pDX, IDC_EDIT_InitSpecWeight, m_InitSpecWeight);
    DDX_Text(pDX, IDC_EDIT_InitSpecBoxWeight, m_InitSpecBoxWeight);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF(CCalibrationFactor, CDialog)
//{{AFX_MSG_MAP(CCalibrationFactor)
ON_BN_CLICKED(IDC_BUTTON_CFUpdate, &CCalibrationFactor::OnBUTTONCFUpdate)
ON_BN_CLICKED(IDC_BUTTON_Zero00, &CCalibrationFactor::OnBUTTONZero00)
ON_BN_CLICKED(IDC_BUTTON_Zero01, &CCalibrationFactor::OnBUTTONZero01)
ON_BN_CLICKED(IDC_BUTTON_Zero02, &CCalibrationFactor::OnBUTTONZero02)
ON_BN_CLICKED(IDC_BUTTON_Zero03, &CCalibrationFactor::OnBUTTONZero03)
ON_BN_CLICKED(IDC_BUTTON_Zero04, &CCalibrationFactor::OnBUTTONZero04)
ON_BN_CLICKED(IDC_BUTTON_Zero05, &CCalibrationFactor::OnBUTTONZero05)
ON_BN_CLICKED(IDC_BUTTON_Zero06, &CCalibrationFactor::OnBUTTONZero06)
ON_BN_CLICKED(IDC_BUTTON_Zero07, &CCalibrationFactor::OnBUTTONZero07)
ON_BN_CLICKED(IDC_BUTTON_Amp00, &CCalibrationFactor::OnBUTTONAmp00)
ON_BN_CLICKED(IDC_BUTTON_Amp01, &CCalibrationFactor::OnBUTTONAmp01)
ON_BN_CLICKED(IDC_BUTTON_Amp02, &CCalibrationFactor::OnBUTTONAmp02)
ON_BN_CLICKED(IDC_BUTTON_Amp03, &CCalibrationFactor::OnBUTTONAmp03)
ON_BN_CLICKED(IDC_BUTTON_Amp04, &CCalibrationFactor::OnBUTTONAmp04)
ON_BN_CLICKED(IDC_BUTTON_Amp05, &CCalibrationFactor::OnBUTTONAmp05)
ON_BN_CLICKED(IDC_BUTTON_Amp06, &CCalibrationFactor::OnBUTTONAmp06)
ON_BN_CLICKED(IDC_BUTTON_Amp07, &CCalibrationFactor::OnBUTTONAmp07)
ON_BN_CLICKED(IDC_BUTTON_CFLoadJSON, &CCalibrationFactor::OnBUTTONCFLoadJSON)
ON_BN_CLICKED(IDC_BUTTON_CFSaveJSON, &CCalibrationFactor::OnBUTTONCFSaveJSON)
//}}AFX_MSG_MAP
END_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF()

/////////////////////////////////////////////////////////////////////////////
// CCalibrationFactor メッセージ ハンドラ

BOOL CCalibrationFactor::OnInitDialog()
{
    CDialog::OnInitDialog();
    CF_Load();

    // Initialize status control
    SetDlgItemText(IDC_STATIC_STATUS, _T(""));

    // TODO: この位置に初期化の補足処理を追加してください

    return TRUE; // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
                 // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CCalibrationFactor::CF_Load()
{
    calc_physical();

    // NOLINTBEGIN(*-pro-type-vararg)
    // Load calibration factors for channels 0-7
    for (size_t i = 0; i < CHANNELS_CAL; ++i)
    {
        m_CFA[i] = Cal_a[i];
        m_CFB[i] = Cal_b[i];
        m_CFC[i] = Cal_c[i];
        m_CFP[i].Format(_T("%11.5f"), Phyout[i]);
    }

    // Channel labels for CH0-CH7
    m_C[0] = _T("CH00, Shear LC");
    m_C[1] = _T("CH01, Vertical LC");
    m_C[2] = _T("CH02, H-LVDT");
    m_C[3] = _T("CH03, V-front-LVDT");
    m_C[4] = _T("CH04, V-rear-LVDT");
    m_C[5] = _T("CH05, Front friction");
    m_C[6] = _T("CH06, Rear friction");
    m_C[7] = _T("CH07");

    // Load initial specimen data
    m_InitSpecHeight = variables::SpecimenData.height_mm();
    m_InitSpecArea = variables::SpecimenData.area_mm2();
    m_InitSpecWeight = variables::SpecimenData.weight_g();
    m_InitSpecBoxWeight = variables::SpecimenData.box_weight_g();

    UpdateData(FALSE);
    // NOLINTEND(*-pro-type-vararg)
}

void CCalibrationFactor::OnBUTTONCFUpdate()
{
    // TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
    UpdateData(TRUE);

    // Save calibration factors for channels 0-7
    for (size_t i = 0; i < CHANNELS_CAL; ++i)
    {
        Cal_a[i] = m_CFA[i];
        Cal_b[i] = m_CFB[i];
        Cal_c[i] = m_CFC[i];
    }

    // Save initial specimen data
    variables::SpecimenData = {m_InitSpecHeight, m_InitSpecArea, m_InitSpecWeight, m_InitSpecBoxWeight};
}

void CCalibrationFactor::OnBUTTONZero00()
{
    OnBUTTONCFUpdate();
    Cal_c[0] = Cal_c[0] - Phyout[0];
    CF_Load();
}

void CCalibrationFactor::OnBUTTONZero01()
{
    OnBUTTONCFUpdate();
    Cal_c[1] = Cal_c[1] - Phyout[1];
    CF_Load();
}

void CCalibrationFactor::OnBUTTONZero02()
{
    OnBUTTONCFUpdate();
    Cal_c[2] = Cal_c[2] - Phyout[2];
    CF_Load();
}

void CCalibrationFactor::OnBUTTONZero03()
{
    OnBUTTONCFUpdate();
    Cal_c[3] = Cal_c[3] - Phyout[3];
    CF_Load();
}

void CCalibrationFactor::OnBUTTONZero04()
{
    OnBUTTONCFUpdate();
    Cal_c[4] = Cal_c[4] - Phyout[4];
    CF_Load();
}

void CCalibrationFactor::OnBUTTONZero05()
{
    OnBUTTONCFUpdate();
    Cal_c[5] = Cal_c[5] - Phyout[5];
    CF_Load();
}

void CCalibrationFactor::OnBUTTONZero06()
{
    OnBUTTONCFUpdate();
    Cal_c[6] = Cal_c[6] - Phyout[6];
    CF_Load();
}

void CCalibrationFactor::OnBUTTONZero07()
{
    OnBUTTONCFUpdate();
    Cal_c[7] = Cal_c[7] - Phyout[7];
    CF_Load();
}

void CCalibrationFactor::OnBUTTONAmp00()
{
    AmpID = 0;
    if (AmpID <= board::AdMaxCH)
    {
        CCalibrationAmp CalibrationAmp;
        if (CalibrationAmp.DoModal() == IDOK)
        {
            CF_Load();
        }
    }
}

void CCalibrationFactor::OnBUTTONAmp01()
{
    AmpID = 1;
    if (AmpID <= board::AdMaxCH)
    {
        CCalibrationAmp CalibrationAmp;
        if (CalibrationAmp.DoModal() == IDOK)
        {
            CF_Load();
        }
    }
}

void CCalibrationFactor::OnBUTTONAmp02()
{
    AmpID = 2;
    if (AmpID <= board::AdMaxCH)
    {
        CCalibrationAmp CalibrationAmp;
        if (CalibrationAmp.DoModal() == IDOK)
        {
            CF_Load();
        }
    }
}

void CCalibrationFactor::OnBUTTONAmp03()
{
    AmpID = 3;
    if (AmpID <= board::AdMaxCH)
    {
        CCalibrationAmp CalibrationAmp;
        if (CalibrationAmp.DoModal() == IDOK)
        {
            CF_Load();
        }
    }
}

void CCalibrationFactor::OnBUTTONAmp04()
{
    AmpID = 4;
    if (AmpID <= board::AdMaxCH)
    {
        CCalibrationAmp CalibrationAmp;
        if (CalibrationAmp.DoModal() == IDOK)
        {
            CF_Load();
        }
    }
}

void CCalibrationFactor::OnBUTTONAmp05()
{
    AmpID = 5;
    if (AmpID <= board::AdMaxCH)
    {
        CCalibrationAmp CalibrationAmp;
        if (CalibrationAmp.DoModal() == IDOK)
        {
            CF_Load();
        }
    }
}

void CCalibrationFactor::OnBUTTONAmp06()
{
    AmpID = 6;
    if (AmpID <= board::AdMaxCH)
    {
        CCalibrationAmp CalibrationAmp;
        if (CalibrationAmp.DoModal() == IDOK)
        {
            CF_Load();
        }
    }
}

void CCalibrationFactor::OnBUTTONAmp07()
{
    AmpID = 7;
    if (AmpID <= board::AdMaxCH)
    {
        CCalibrationAmp CalibrationAmp;
        if (CalibrationAmp.DoModal() == IDOK)
        {
            CF_Load();
        }
    }
}

void CCalibrationFactor::OnBUTTONCFSaveJSON()
{
    spdlog::info("Save calibration to JSON button clicked");
    // Save calibration data to JSON file
    CFileDialog CalSaveFile_dlg(FALSE, _T("json"), _T("*.json"), OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT,
                                _T("JSON Files(*.json)|*.json| All Files(*.*)|*.*| |"), NULL);

    if (CalSaveFile_dlg.DoModal() == IDOK)
    {
        const CStringW cspathW = CalSaveFile_dlg.GetPathName();
        const std::wstring wpath{cspathW};
        const std::string path_u8{CW2A(cspathW, CP_UTF8)};
        spdlog::info("Saving calibration data to: {}", path_u8);

        try
        {
            ryml::Tree tree;
            ryml::NodeRef root = tree.rootref();
            root |= ryml::MAP;

            ryml::NodeRef channels = root["calibration_data"];
            channels |= ryml::SEQ;

            // Only save non-zero calibration data
            for (size_t i = 0; i < CHANNELS_CAL; i++)
            {
                if (Cal_a[i] != 0.0 || Cal_b[i] != 0.0 || Cal_c[i] != 0.0)
                {
                    ryml::NodeRef ch = channels.append_child();
                    ch |= ryml::MAP;
                    ch["channel"] << i;
                    ch["cal_a"] << Cal_a[i];
                    ch["cal_b"] << Cal_b[i];
                    ch["cal_c"] << Cal_c[i];
                    if (AmpPB[i] != 0.0 || AmpPO[i] != 0.0)
                    {
                        ch["amp_pb"] << AmpPB[i];
                        ch["amp_po"] << AmpPO[i];
                    }
                }
            }
            spdlog::debug("Saving {} channel calibration factors", channels.num_children());

            // Save initial specimen data
            ryml::NodeRef specimen = root["initial_specimen"];
            specimen |= ryml::MAP;
            specimen["height_mm"] << variables::SpecimenData.height_mm();
            specimen["area_mm2"] << variables::SpecimenData.area_mm2();
            specimen["weight_g"] << variables::SpecimenData.weight_g();
            specimen["box_weight_g"] << variables::SpecimenData.box_weight_g();
            spdlog::debug("Saving initial specimen data: height={}, area={}, weight={}, box_weight={}",
                          variables::SpecimenData.height_mm(), variables::SpecimenData.area_mm2(),
                          variables::SpecimenData.weight_g(), variables::SpecimenData.box_weight_g());

            std::ofstream ofs(wpath);
            if (!ofs.is_open())
            {
                spdlog::error("Failed to open calibration JSON file for write: {}", path_u8);
                AfxMessageBox(_T("Failed to open calibration JSON file for write."), MB_ICONEXCLAMATION | MB_OK);
                return;
            }

            ofs << ryml::as_json(tree);
            ofs.close();

            spdlog::info("Calibration data saved successfully");
            set_status_text_and_fit_dialog(*this, IDC_STATIC_STATUS, "Calibration data saved to JSON successfully.");
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error saving calibration JSON: {}", e.what());
            AfxMessageBox(CA2W(std::format("Error saving calibration JSON: {}", e.what()).c_str(), CP_UTF8),
                          MB_ICONEXCLAMATION | MB_OK);
        }
    }
    else
    {
        spdlog::debug("Calibration save cancelled by user");
    }
}

void CCalibrationFactor::OnBUTTONCFLoadJSON()
{
    spdlog::info("Load calibration from JSON button clicked");
    // Load calibration data from JSON file
    CFileDialog CalLoadFile_dlg(TRUE, _T("json"), _T("*.json"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                                _T("JSON Files(*.json)|*.json| All Files(*.*)|*.*| |"), NULL);

    if (CalLoadFile_dlg.DoModal() == IDOK)
    {
        const CStringW cspathW = CalLoadFile_dlg.GetPathName();
        const std::wstring wpath{cspathW};
        const std::string path_u8{CW2A(cspathW, CP_UTF8)};
        spdlog::info("Loading calibration data from: {}", path_u8);

        try
        {
            std::ifstream ifs(wpath);
            if (!ifs.is_open())
            {
                spdlog::error("Failed to open calibration JSON file: {}", path_u8);
                AfxMessageBox(_T("Failed to open calibration JSON file."), MB_ICONEXCLAMATION | MB_OK);
                return;
            }

            std::string json_content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();

            ryml::Tree tree = ryml::parse_json_in_arena(ryml::to_csubstr(json_content));
            ryml::ConstNodeRef root = tree.rootref();

            if (!root.has_child("calibration_data") || !root["calibration_data"].is_seq())
            {
                spdlog::error("Invalid calibration JSON format: missing or invalid calibration_data");
                AfxMessageBox(_T("Invalid calibration JSON format.\n"
                                 "Expected a 'calibration_data' array.\n"
                                 "This may not be a calibration data file."),
                              MB_ICONEXCLAMATION | MB_OK);
                return;
            }

            spdlog::debug("Initializing calibration factors to zero");
            // Initialize all channels to 0 (default for omitted channels)
            for (size_t i = 0; i < 64; i++)
            {
                Cal_a[i] = 0.0;
                Cal_b[i] = 0.0;
                Cal_c[i] = 0.0;
                AmpPB[i] = 0.0f;
                AmpPO[i] = 0.0f;
            }

            // Load only the channels present in the file
            int loaded_channels = 0;
            std::ostringstream channel_list;
            ryml::ConstNodeRef channels = root["calibration_data"];
            for (const auto &ch : channels)
            {
                size_t idx = 0;
                ch["channel"] >> idx;
                if (idx < 64)
                {
                    ch["cal_a"] >> Cal_a[idx];
                    ch["cal_b"] >> Cal_b[idx];
                    ch["cal_c"] >> Cal_c[idx];
                    if (ch.has_child("amp_pb"))
                    {
                        ch["amp_pb"] >> AmpPB[idx];
                    }
                    if (ch.has_child("amp_po"))
                    {
                        ch["amp_po"] >> AmpPO[idx];
                    }
                    loaded_channels++;
                    if (loaded_channels <= 10)
                    {
                        channel_list << "CH" << idx << " ";
                    }
                    spdlog::trace("Loaded calibration for channel {}: a={}, b={}, c={}", idx, Cal_a[idx], Cal_b[idx],
                                  Cal_c[idx]);
                }
            }

            spdlog::info("Calibration data loaded successfully: {} channels", loaded_channels);

            // Load initial specimen data if present
            if (root.has_child("initial_specimen") && root["initial_specimen"].is_map())
            {
                ryml::ConstNodeRef specimen = root["initial_specimen"];
                double height_mm = 0.0;
                double area_mm2 = 0.0;
                double weight_g = 0.0;
                double box_weight_g = 0.0;
                if (specimen.has_child("height_mm"))
                    specimen["height_mm"] >> height_mm;
                if (specimen.has_child("area_mm2"))
                    specimen["area_mm2"] >> area_mm2;
                if (specimen.has_child("weight_g"))
                    specimen["weight_g"] >> weight_g;
                if (specimen.has_child("box_weight_g"))
                    specimen["box_weight_g"] >> box_weight_g;
                spdlog::info("Initial specimen data loaded: height={}, area={}, weight={}, box_weight={}", height_mm,
                             area_mm2, weight_g, box_weight_g);
                variables::SpecimenData = {height_mm, area_mm2, weight_g, box_weight_g};
            }
            else
            {
                spdlog::debug("No initial_specimen section in calibration file");
            }

            CF_Load();

            // Show appropriate message based on what was loaded
            if (loaded_channels == 0)
            {
                AfxMessageBox(_T("Warning: No calibration data was loaded from the JSON file.\n"
                                 "The 'calibration_data' array is empty or contains invalid entries.\n"
                                 "This may not be a calibration data file."),
                              MB_ICONWARNING | MB_OK);
            }
            else
            {
                set_status_text_and_fit_dialog(
                    *this, IDC_STATIC_STATUS,

                    loaded_channels <= 10
                        ? std::format("Calibration data loaded successfully.\nLoaded {} channel(s): {}",
                                      loaded_channels, channel_list.str())
                        : std::format(
                              "Calibration data loaded successfully.\nLoaded {} channel(s): {}... (and {} more)",
                              loaded_channels, channel_list.str(), loaded_channels - 10));
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error loading calibration JSON: {}", e.what());
            AfxMessageBox(CA2W(std::format("Error loading calibration JSON: {}", e.what()).c_str(), CP_UTF8),
                          MB_ICONEXCLAMATION | MB_OK);
        }
    }
}

#pragma warning(pop)
