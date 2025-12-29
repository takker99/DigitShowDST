/**
 * @file Control_File.cpp
 * @brief Implementation of control file dialog and script execution
 *
 * インプリメンテーション ファイル
 */
#include "StdAfx.h"
#pragma warning(push)
#pragma warning(disable : 4800 4866)

#include "Control_File.h"
#include "control/control.hpp"
#include "control/json.hpp"

#include "resource.h"
#include <format>
#include <math.h>

#include "chrono_alias.hpp"
#include "ui_helpers.hpp"
#include <fstream>
#include <optional>
#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>
#include <span>
#include <spdlog/spdlog.h>
#include <sstream>

static constexpr auto toCFPARA(const control::ControlParams &params) noexcept
{
    return std::array<double, CControl_File::CFPARA_COUNT>{{params.loading_dir ? 0.0 : 1.0,
                                                            params.motor_rpm,
                                                            params.target_tau_kpa,
                                                            params.target_sigma_kpa,
                                                            params.duration.count(),
                                                            static_cast<double>(params.cycles),
                                                            params.tau_lower_kpa,
                                                            params.tau_upper_kpa,
                                                            params.target_displacement_mm,
                                                            params.displacement_lower_mm,
                                                            params.displacement_upper_mm,
                                                            params.acceleration_rate_rpm_per_min,
                                                            params.consolidation_rate_kpa_per_min,
                                                            params.tau_start_kpa,
                                                            params.tau_end_kpa,
                                                            params.sigma_start_kpa,
                                                            params.k_value,
                                                            params.err_stress_kpa,
                                                            params.err_disp_mm,
                                                            params.amp_v_per_kpa_m2,
                                                            params.amp2_f_v_per_mm,
                                                            params.amp2_r_v_per_mm,
                                                            params.dmax_v,
                                                            params.err_disp_cv_mm,
                                                            params.amp_cv_v_per_mm}};
}

static constexpr auto fromCFPARA(const std::array<double, CControl_File::CFPARA_COUNT> &cfpara,
                                 const ControlPattern pattern, std::string name) noexcept
{
    return control::ControlParams{.pattern = pattern,
                                  .loading_dir = cfpara[0] > 0 ? false : true,
                                  .motor_rpm = cfpara[1],
                                  .target_tau_kpa = cfpara[2],
                                  .target_sigma_kpa = cfpara[3],
                                  .duration = std::chrono::minutes_d{cfpara[4]},
                                  .cycles = static_cast<size_t>(cfpara[5]),
                                  .tau_lower_kpa = cfpara[6],
                                  .tau_upper_kpa = cfpara[7],
                                  .target_displacement_mm = cfpara[8],
                                  .displacement_lower_mm = cfpara[9],
                                  .displacement_upper_mm = cfpara[10],
                                  .acceleration_rate_rpm_per_min = cfpara[11],
                                  .consolidation_rate_kpa_per_min = cfpara[12],
                                  .tau_start_kpa = cfpara[13],
                                  .tau_end_kpa = cfpara[14],
                                  .sigma_start_kpa = cfpara[15],
                                  .k_value = cfpara[16],
                                  .err_stress_kpa = cfpara[17],
                                  .err_disp_mm = cfpara[18],
                                  .amp_v_per_kpa_m2 = cfpara[19],
                                  .amp2_f_v_per_mm = cfpara[20],
                                  .amp2_r_v_per_mm = cfpara[21],
                                  .dmax_v = cfpara[22],
                                  .err_disp_cv_mm = cfpara[23],
                                  .amp_cv_v_per_mm = cfpara[24],
                                  .name = name};
}

/////////////////////////////////////////////////////////////////////////////
// CControl_File ダイアログ

CControl_File::CControl_File(CWnd *pParent /*=NULL*/)
    : CDialog(CControl_File::IDD, pParent), m_StepNum(control::CURNUM),
      m_SCFNum(control::CURNUM < control::control_steps.size()
                   ? static_cast<size_t>(control::control_steps[control::CURNUM].parameters.pattern)
                   : 0),
      m_CurNum(control::CURNUM),
      m_CFNum(control::CURNUM < control::control_steps.size()
                  ? static_cast<size_t>(control::control_steps[control::CURNUM].parameters.pattern)
                  : 0),
      m_CFLabel(
          control::CURNUM < control::control_steps.size()
              ? CA2W(std::string(get_use_label(control::control_steps[control::CURNUM].parameters.pattern)).c_str(),
                     CP_UTF8)
              : CA2W(std::string(get_use_label(ControlPattern::NoControl)).c_str(), CP_UTF8)),
      m_CurLabel(
          control::CURNUM < control::control_steps.size()
              ? CA2W(std::string(get_use_label(control::control_steps[control::CURNUM].parameters.pattern)).c_str(),
                     CP_UTF8)
              : CA2W(std::string(get_use_label(ControlPattern::NoControl)).c_str(), CP_UTF8))
{
    //{{AFX_DATA_INIT(CControl_File)

    //}}AFX_DATA_INIT

    // Initialize array with current control step parameters
    if (control::CURNUM < control::control_steps.size())
    {
        m_CFPARA = toCFPARA(control::control_steps[control::CURNUM].parameters);
    }
}

void CControl_File::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CControl_File)

    // Resource ID array for DDX_Text loop - now includes all ControlParams fields (except description)
    static constexpr int IDS_CFPARA[CFPARA_COUNT] = {
        IDC_EDIT_CFPARA0,  IDC_EDIT_CFPARA1,  IDC_EDIT_CFPARA2,  IDC_EDIT_CFPARA3,  IDC_EDIT_CFPARA4,
        IDC_EDIT_CFPARA5,  IDC_EDIT_CFPARA6,  IDC_EDIT_CFPARA7,  IDC_EDIT_CFPARA8,  IDC_EDIT_CFPARA9,
        IDC_EDIT_CFPARA10, IDC_EDIT_CFPARA11, IDC_EDIT_CFPARA12, IDC_EDIT_CFPARA13, IDC_EDIT_CFPARA14,
        IDC_EDIT_CFPARA15, IDC_EDIT_CFPARA16, IDC_EDIT_CFPARA17, IDC_EDIT_CFPARA18, IDC_EDIT_CFPARA19,
        IDC_EDIT_CFPARA20, IDC_EDIT_CFPARA21, IDC_EDIT_CFPARA22, IDC_EDIT_CFPARA23, IDC_EDIT_CFPARA24};

    // Use loop for array-based DDX (numeric parameters)
    for (size_t i = 0; i < CFPARA_COUNT; ++i)
    {
        DDX_Text(pDX, IDS_CFPARA[i], m_CFPARA[i]);
    }

    // Description field (string)
    DDX_Text(pDX, IDC_EDIT_CFPARA25, m_Description);

    DDX_Text(pDX, IDC_EDIT_StepNum, m_StepNum);
    if (!control::control_steps.empty())
    {
        DDV_MinMaxULongLong(pDX, m_StepNum, 0, control::control_steps.size() - 1);
    }
    DDX_Control(pDX, IDC_COMBO_ControlPattern, m_ComboPattern);
    DDX_Text(pDX, IDC_EDIT_CurNum, m_CurNum);
    DDX_Text(pDX, IDC_EDIT_CFNum, m_CFNum);
    DDX_Text(pDX, IDC_STATIC_CurLabel, m_CurLabel);
    DDX_Text(pDX, IDC_STATIC_PatternDesc, m_PatternDesc);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF(CControl_File, CDialog)
//{{AFX_MSG_MAP(CControl_File)
ON_BN_CLICKED(IDC_BUTTON_Update, &CControl_File::OnBUTTONUpdate)
ON_BN_CLICKED(IDC_BUTTON_LoadJSON, &CControl_File::OnBUTTONReadJSON)
ON_BN_CLICKED(IDC_BUTTON_SaveJSON, &CControl_File::OnBUTTONSaveJSON)
ON_BN_CLICKED(IDC_BUTTON_Load, &CControl_File::OnBUTTONLoad)
ON_BN_CLICKED(IDC_CHECK_ChangeNo, &CControl_File::OnCHECKChangeNo)
ON_BN_CLICKED(IDC_BUTTON_StepDec, &CControl_File::OnBUTTONStepDec)
ON_BN_CLICKED(IDC_BUTTON_StepInc, &CControl_File::OnBUTTONStepInc)
ON_CBN_SELCHANGE(IDC_COMBO_ControlPattern, &CControl_File::OnSelChangeControlPattern)
//}}AFX_MSG_MAP
END_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF()

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CControl_File メッセージ ハンドラ
BOOL CControl_File::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Initialize status control
    SetDlgItemText(IDC_STATIC_STATUS, _T(""));

    // Initialize combo box with control patterns
    m_ComboPattern.ResetContent();

    // Add all control patterns to combo box
    const std::vector<std::pair<ControlPattern, const char *>> patterns = {
        {ControlPattern::NoControl, "No Control"},
        {ControlPattern::MonotonicLoadingConstantPressure, "Monotonic Loading - Constant Pressure"},
        {ControlPattern::MonotonicLoadingConstantVolume, "Monotonic Loading - Constant Volume"},
        {ControlPattern::CyclicLoadingConstantPressure, "Cyclic Loading - Constant Pressure"},
        {ControlPattern::CyclicLoadingConstantVolume, "Cyclic Loading - Constant Volume"},
        {ControlPattern::CreepConstantPressure, "Creep - Constant Pressure"},
        {ControlPattern::CreepConstantVolume, "Creep - Constant Volume"},
        {ControlPattern::RelaxationConstantPressure, "Relaxation - Constant Pressure"},
        {ControlPattern::RelaxationConstantVolume, "Relaxation - Constant Volume"},
        {ControlPattern::MonotonicLoadingDisplacementConstantPressure, "Monotonic Loading (Disp) - Constant Pressure"},
        {ControlPattern::MonotonicLoadingDisplacementConstantVolume, "Monotonic Loading (Disp) - Constant Volume"},
        {ControlPattern::CyclicLoadingDisplacementConstantPressure, "Cyclic Loading (Disp) - Constant Pressure"},
        {ControlPattern::CyclicLoadingDisplacementConstantVolume, "Cyclic Loading (Disp) - Constant Volume"},
        {ControlPattern::AccelerationConstantPressure, "Acceleration - Constant Pressure"},
        {ControlPattern::AccelerationConstantVolume, "Acceleration - Constant Volume"},
        {ControlPattern::ConstantTauConsolidation, "Constant-Tau Consolidation"},
        {ControlPattern::KConsolidation, "K-Consolidation"},
        {ControlPattern::CreepConstantPressureFast, "Creep after Fast ML - Constant Pressure"},
        {ControlPattern::CreepConstantPressureFastRef, "Creep after Fast ML (Ref) - Constant Pressure"},
        {ControlPattern::PreConsolidation, "Pre-consolidation"},
        {ControlPattern::BeforeConsolidation, "Rebase Reference"},
        {ControlPattern::AfterConsolidation, "Rebase Reference"}};

    for (const auto &[pattern, label] : patterns)
    {
        const int index = m_ComboPattern.AddString(CA2W(label, CP_UTF8));
        m_ComboPattern.SetItemData(index, static_cast<DWORD_PTR>(pattern));
    }

    // Select first item by default
    m_ComboPattern.SetCurSel(0);

    // Update pattern description for initial selection
    UpdatePatternDescription();

    // TODO: この位置に初期化の補足処理を追加してください
    GetDlgItem(IDC_BUTTON_StepDec)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_StepInc)->EnableWindow(FALSE);

#pragma warning(push)
#pragma warning(disable : 4946)
    reinterpret_cast<CButton *>(GetDlgItem(IDC_CHECK_ChangeNo)) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        ->SetCheck(0);
#pragma warning(pop)
    return TRUE; // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
                 // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CControl_File::OnBUTTONLoad()
{
    // TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
    UpdateData(TRUE);
    if (m_StepNum < control::control_steps.size())
    {
        const auto &params = control::control_steps[m_StepNum].parameters;

        // Set combo box selection
        const int count = m_ComboPattern.GetCount();
        for (int i = 0; i < count; ++i)
        {
            if (static_cast<ControlPattern>(m_ComboPattern.GetItemData(i)) == params.pattern)
            {
                m_ComboPattern.SetCurSel(i);
                break;
            }
        }

        // Load all parameters from ControlParams
        m_CFPARA = toCFPARA(params);
        m_Description = CA2W(params.name.c_str(), CP_UTF8);

        // Update pattern description
        UpdatePatternDescription();
    }
    UpdateData(FALSE);
}

void CControl_File::OnBUTTONUpdate()
{
    // TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
    UpdateData(TRUE);
    if (m_StepNum < control::control_steps.size())
    {
        // Get selected pattern from combo box
        const int sel = m_ComboPattern.GetCurSel();
        if (sel == CB_ERR)
        {
            AfxMessageBox(_T("Please select a control pattern."), MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        const auto pattern = static_cast<ControlPattern>(m_ComboPattern.GetItemData(sel));

        // Create ControlParams from UI values
        control::control_steps[m_StepNum].parameters =
            fromCFPARA(m_CFPARA, pattern, CW2A(m_Description, CP_UTF8).m_psz);
    }
    UpdateData(FALSE);
}

void CControl_File::OnBUTTONReadJSON()
{
    spdlog::info("Load control script button clicked");
    // Load control script from JSON file (new format with "use" field)
    CFileDialog dlg(TRUE, _T("json"), _T("*.json"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                    _T("JSON Files(*.json)|*.json| All Files(*.*)|*.*| |"), NULL);

    if (dlg.DoModal() != IDOK)
    {
        spdlog::debug("Control script load cancelled by user");
        return;
    }

    const CStringW cspathW = dlg.GetPathName();
    const std::wstring wpath{cspathW};
    const std::string path_u8{CW2A(cspathW, CP_UTF8)};
    spdlog::info("Loading control script from: {}", path_u8);
    try
    {
        std::ifstream ifs(wpath);
        if (!ifs.is_open())
        {
            spdlog::error("Failed to open control JSON file: {}", path_u8);
            AfxMessageBox(_T("Failed to open control JSON file."), MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        std::string json_content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();

        ryml::Tree tree = ryml::parse_json_in_arena(ryml::to_csubstr(json_content));
        ryml::ConstNodeRef root = tree.rootref();

        if (!root.has_child("steps") || !root["steps"].is_seq())
        {
            spdlog::error("Invalid control JSON format: missing 'steps' array");
            AfxMessageBox(_T("Invalid control JSON format: missing 'steps' array."), MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        spdlog::debug("Initializing control arrays");

        // reset control steps
        control::CURNUM = 0;
        control::control_steps.clear();

        // Convert each JSON step to ControlStep using helper function
        int loaded_steps = 0;
        int failed_steps = 0;
        ryml::ConstNodeRef steps = root["steps"];
        for (const auto &step : steps)
        {
            auto params_opt = JsonStepToControlParams(step);
            if (params_opt.has_value())
            {
                auto params = params_opt.value();
                control::control_steps.push_back({.parameters = params});
                loaded_steps++;
                spdlog::debug("Loaded step {}: {}", control::control_steps.size() - 1, get_use_label(params.pattern));
            }
            else
            {
                // Failed to parse this step, skip or log error
                spdlog::warn("Failed to parse step {}", control::control_steps.size());
                failed_steps++;
                AfxMessageBox(CA2W(std::format("Warning: Failed to parse step {} (missing or invalid 'use' field)",
                                               control::control_steps.size())
                                       .c_str(),
                                   CP_UTF8),
                              MB_ICONWARNING | MB_OK);
            }
        }

        spdlog::info("Control script loaded successfully: {} steps", control::control_steps.size());
        m_StepNum = control::CURNUM;
        m_CurNum = control::CURNUM;
        m_SCFNum = control::CURNUM < control::control_steps.size()
                       ? static_cast<size_t>(control::control_steps[control::CURNUM].parameters.pattern)
                       : 0;
        m_CFNum = m_SCFNum;
        m_CFLabel =
            control::CURNUM < control::control_steps.size()
                ? CA2W(std::string(get_use_label(control::control_steps[control::CURNUM].parameters.pattern)).c_str(),
                       CP_UTF8)
                : CA2W(std::string(get_use_label(ControlPattern::NoControl)).c_str(), CP_UTF8);
        m_CurLabel = m_CFLabel;

        // Load current step parameters into UI
        if (m_StepNum < control::control_steps.size())
        {
            const auto &params = control::control_steps[m_StepNum].parameters;
            m_CFPARA = toCFPARA(params);
            m_Description = CA2W(params.name.c_str(), CP_UTF8);

            // Set combo box selection
            const int count = m_ComboPattern.GetCount();
            for (int i = 0; i < count; ++i)
            {
                if (static_cast<ControlPattern>(m_ComboPattern.GetItemData(i)) == params.pattern)
                {
                    m_ComboPattern.SetCurSel(i);
                    break;
                }
            }

            // Update pattern description
            UpdatePatternDescription();
        }

        UpdateData(FALSE);

        // Show appropriate message based on what was loaded
        if (loaded_steps == 0)
        {
            AfxMessageBox(_T("Warning: No control steps were loaded from the JSON file.\n"
                             "All steps failed to parse or the 'steps' array is empty.\n"
                             "This may not be a control script file."),
                          MB_ICONWARNING | MB_OK);
        }
        else if (failed_steps > 0)
        {
            AfxMessageBox(CA2W(std::format("Control script loaded with warnings.\n"
                                           "Successfully loaded: {} step(s)\n"
                                           "Failed to parse: {} step(s)",
                                           loaded_steps, failed_steps)
                                   .c_str(),
                               CP_UTF8),
                          MB_ICONWARNING | MB_OK);
        }
        else
        {
            set_status_text_and_fit_dialog(*this, IDC_STATIC_STATUS,
                                           std::format("Control script loaded successfully.\n"
                                                       "Loaded {} control step(s).",
                                                       loaded_steps));
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error loading control JSON: {}", e.what());
        AfxMessageBox(CA2W(std::format("Error loading control JSON: {}", e.what()).c_str(), CP_UTF8),
                      MB_ICONEXCLAMATION | MB_OK);
    }
}

void CControl_File::OnBUTTONSaveJSON()
{
    // Save control script to JSON file (new format with "use" field)
    CFileDialog dlg(FALSE, _T("json"), _T("*.json"), OFN_OVERWRITEPROMPT,
                    _T("JSON Files(*.json)|*.json| All Files(*.*)|*.*| |"), NULL);

    if (dlg.DoModal() != IDOK)
        return;

    const std::wstring wpath = std::wstring(dlg.GetPathName().GetString());
    try
    {
        ryml::Tree tree;
        ryml::NodeRef root = tree.rootref();
        root |= ryml::MAP;

        root["$schema"] << "../schemas/control_script.schema.json";

        ryml::NodeRef steps = root["steps"];
        steps |= ryml::SEQ;

        // Save all control steps from vector
        for (const auto &control_step : control::control_steps)
        {
            if (control_step.parameters.pattern == ControlPattern::NoControl)
                continue; // Skip no_control steps

            // Convert ControlStep to ryml tree using helper function
            ryml::Tree step_tree = ControlParamsToJsonStep(control_step.parameters);

            // Copy the step tree into the steps array
            ryml::NodeRef new_step = steps.append_child();
            new_step |= ryml::MAP;
            for (const auto &child : step_tree.rootref())
            {
                auto key_str = child.key();
                // Map node
                if (child.is_map())
                {
                    ryml::NodeRef dst_child = new_step[ryml::to_csubstr(key_str)];
                    dst_child |= ryml::MAP;
                    for (const auto &grandchild : child)
                    {
                        auto gkey_str = grandchild.key();
                        dst_child[ryml::to_csubstr(gkey_str)] << grandchild.val();
                    }
                }
                else if (child.is_seq())
                {
                    // Sequence node
                    ryml::NodeRef dst_child = new_step[ryml::to_csubstr(key_str)];
                    dst_child |= ryml::SEQ;
                    for (const auto &item : child)
                    {
                        dst_child.append_child() << item.val();
                    }
                }
                else if (child.is_keyval())
                {
                    // Simple key-value
                    new_step[ryml::to_csubstr(key_str)] << child.val();
                }
            }
        }

        std::ofstream ofs(wpath);
        if (!ofs.is_open())
        {
            AfxMessageBox(_T("Failed to open control JSON file for write."), MB_ICONEXCLAMATION | MB_OK);
            return;
        }
        ofs << ryml::as_json(tree);
        ofs.close();
        set_status_text_and_fit_dialog(*this, IDC_STATIC_STATUS, "Control script saved to JSON successfully.");
    }
    catch (const std::exception &e)
    {
        AfxMessageBox(CA2W(std::format("Error saving control JSON: {}", e.what()).c_str(), CP_UTF8),
                      MB_ICONEXCLAMATION | MB_OK);
    }
}

void CControl_File::OnCHECKChangeNo()
{
#pragma warning(push)
#pragma warning(disable : 4946)
    auto chkbox1 = reinterpret_cast<CButton *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        GetDlgItem(IDC_CHECK_ChangeNo));
#pragma warning(pop)
    GetDlgItem(IDC_BUTTON_StepDec)->EnableWindow(chkbox1->GetCheck() && control::CURNUM > 0 ? TRUE : FALSE);
    GetDlgItem(IDC_BUTTON_StepInc)
        ->EnableWindow(chkbox1->GetCheck() && control::CURNUM < control::control_steps.size() - 1 ? TRUE : FALSE);
}

void CControl_File::OnBUTTONStepDec()
{
    // TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
    using namespace std::chrono_literals;
    control::CURNUM--;
    control::NUM_Cyclic = 0;
    control::TotalStepTime = 0s;
    m_CurNum = control::CURNUM;
    m_CFNum = (control::CURNUM < control::control_steps.size())
                  ? static_cast<size_t>(control::control_steps[control::CURNUM].parameters.pattern)
                  : 0;
    m_CFLabel =
        (control::CURNUM < control::control_steps.size())
            ? CA2W(std::string(get_use_label(control::control_steps[control::CURNUM].parameters.pattern)).c_str(),
                   CP_UTF8)
            : L"";
    m_CurLabel = m_CFLabel;

    // Load current step parameters into UI
    if (control::CURNUM < control::control_steps.size())
    {
        const auto &params = control::control_steps[control::CURNUM].parameters;
        m_CFPARA = toCFPARA(params);
        m_Description = CA2W(params.name.c_str(), CP_UTF8);

        // Set combo box selection
        const int count = m_ComboPattern.GetCount();
        for (int i = 0; i < count; ++i)
        {
            if (static_cast<ControlPattern>(m_ComboPattern.GetItemData(i)) == params.pattern)
            {
                m_ComboPattern.SetCurSel(i);
                break;
            }
        }

        // Update pattern description
        UpdatePatternDescription();
    }

    GetDlgItem(IDC_BUTTON_StepDec)->EnableWindow(control::CURNUM > 0 ? TRUE : FALSE);
    GetDlgItem(IDC_BUTTON_StepInc)->EnableWindow(control::CURNUM < control::control_steps.size() - 1 ? TRUE : FALSE);
    UpdateData(FALSE);
}

void CControl_File::OnBUTTONStepInc()
{
    using namespace std::chrono_literals;
    // TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
    control::CURNUM++;
    control::NUM_Cyclic = 0;
    control::TotalStepTime = 0s;
    m_CurNum = control::CURNUM;
    m_CFNum = (control::CURNUM < control::control_steps.size())
                  ? static_cast<size_t>(control::control_steps[control::CURNUM].parameters.pattern)
                  : 0;
    m_CFLabel =
        (control::CURNUM < control::control_steps.size())
            ? CA2W(std::string(get_use_label(control::control_steps[control::CURNUM].parameters.pattern)).c_str(),
                   CP_UTF8)
            : L"";
    m_CurLabel = m_CFLabel;

    // Load current step parameters into UI
    if (control::CURNUM < control::control_steps.size())
    {
        const auto &params = control::control_steps[control::CURNUM].parameters;
        m_CFPARA = toCFPARA(params);
        m_Description = CA2W(params.name.c_str(), CP_UTF8);

        // Set combo box selection
        const int count = m_ComboPattern.GetCount();
        for (int i = 0; i < count; ++i)
        {
            if (static_cast<ControlPattern>(m_ComboPattern.GetItemData(i)) == params.pattern)
            {
                m_ComboPattern.SetCurSel(i);
                break;
            }
        }

        // Update pattern description
        UpdatePatternDescription();
    }

    GetDlgItem(IDC_BUTTON_StepDec)->EnableWindow(control::CURNUM > 0 ? TRUE : FALSE);
    GetDlgItem(IDC_BUTTON_StepInc)->EnableWindow(control::CURNUM < control::control_steps.size() - 1 ? TRUE : FALSE);
    UpdateData(FALSE);
}

void CControl_File::UpdatePatternDescription()
{
    // Update pattern description based on current combo box selection
    const int sel = m_ComboPattern.GetCurSel();
    if (sel == CB_ERR)
        return;

    const auto pattern = static_cast<ControlPattern>(m_ComboPattern.GetItemData(sel)); // Map patterns to descriptions
    std::string desc;
    switch (pattern)
    {
    case ControlPattern::NoControl:
        desc = "No control active. End of control sequence.";
        break;
    case ControlPattern::MonotonicLoadingConstantPressure:
        desc = "Monotonic loading with constant confining pressure. Params: loading_dir (0=load/1=unload), motor_rpm, "
               "target_tau_kpa, target_sigma_kpa";
        break;
    case ControlPattern::MonotonicLoadingConstantVolume:
        desc = "Monotonic loading with constant volume. Params: loading_dir, motor_rpm, target_tau_kpa";
        break;
    case ControlPattern::CyclicLoadingConstantPressure:
        desc = "Cyclic loading with constant pressure. Params: loading_dir, motor_rpm, tau_lower_kpa, tau_upper_kpa, "
               "cycles, target_sigma_kpa";
        break;
    case ControlPattern::CyclicLoadingConstantVolume:
        desc =
            "Cyclic loading with constant volume. Params: loading_dir, motor_rpm, tau_lower_kpa, tau_upper_kpa, cycles";
        break;
    case ControlPattern::CreepConstantPressure:
        desc = "Creep test with constant pressure. Params: motor_rpm, target_tau_kpa, duration_min, target_sigma_kpa";
        break;
    case ControlPattern::CreepConstantVolume:
        desc = "Creep test with constant volume. Params: motor_rpm, target_tau_kpa, duration_min";
        break;
    case ControlPattern::RelaxationConstantPressure:
        desc = "Relaxation test with constant pressure. Params: duration_min, target_sigma_kpa";
        break;
    case ControlPattern::RelaxationConstantVolume:
        desc = "Relaxation test with constant volume. Params: duration_min";
        break;
    case ControlPattern::MonotonicLoadingDisplacementConstantPressure:
        desc = "Displacement-controlled monotonic loading with constant pressure. Params: loading_dir, motor_rpm, "
               "target_disp_mm, target_sigma_kpa";
        break;
    case ControlPattern::MonotonicLoadingDisplacementConstantVolume:
        desc = "Displacement-controlled monotonic loading with constant volume. Params: loading_dir, motor_rpm, "
               "target_disp_mm";
        break;
    case ControlPattern::CyclicLoadingDisplacementConstantPressure:
        desc = "Displacement-controlled cyclic loading with constant pressure. Params: loading_dir, motor_rpm, "
               "disp_lower_mm, disp_upper_mm, cycles, target_sigma_kpa";
        break;
    case ControlPattern::CyclicLoadingDisplacementConstantVolume:
        desc = "Displacement-controlled cyclic loading with constant volume. Params: loading_dir, motor_rpm, "
               "disp_lower_mm, disp_upper_mm, cycles";
        break;
    case ControlPattern::AccelerationConstantPressure:
        desc = "Acceleration test with constant pressure. Params: loading_dir, tau_start_kpa, tau_end_kpa, "
               "accel_rate_rpm, target_sigma_kpa";
        break;
    case ControlPattern::AccelerationConstantVolume:
        desc =
            "Acceleration test with constant volume. Params: loading_dir, tau_start_kpa, tau_end_kpa, accel_rate_rpm";
        break;
    case ControlPattern::ConstantTauConsolidation:
        desc = "Constant-tau consolidation. Params: loading_dir (0=compression/1=dilation), motor_rpm, target_tau_kpa, "
               "consol_rate_kpa, target_sigma_kpa";
        break;
    case ControlPattern::KConsolidation:
        desc = "K-consolidation (linear stress path). Params: loading_dir, motor_rpm, tau_start_kpa, tau_end_kpa, "
               "sigma_start_kpa, k_value";
        break;
    case ControlPattern::CreepConstantPressureFast:
        desc = "Creep after fast monotonic loading with constant pressure. Params: motor_rpm, target_tau_kpa, "
               "duration_min, target_sigma_kpa, loading_dir";
        break;
    case ControlPattern::CreepConstantPressureFastRef:
        desc = "Creep after fast ML with reference tau and constant pressure. Params: motor_rpm, target_tau_kpa, "
               "duration_min, target_sigma_kpa, loading_dir, tau_start_kpa (ref)";
        break;
    case ControlPattern::PreConsolidation:
        desc = "Pre-consolidation phase. Params: target_tau_kpa, motor_rpm";
        break;
    case ControlPattern::BeforeConsolidation:
        desc = "Rebase measurement reference (zero-reference for physical inputs). Alias: before_consolidation, "
               "after_consolidation. No parameters required.";
        break;
    case ControlPattern::AfterConsolidation:
        desc = "Rebase measurement reference (zero-reference for physical inputs). Alias: before_consolidation, "
               "after_consolidation. No parameters required.";
        break;
    default:
        desc = "Unknown pattern.";
        break;
    }

    m_PatternDesc = CA2W(desc.c_str(), CP_UTF8);
    UpdateData(FALSE);
}

void CControl_File::OnSelChangeControlPattern()
{
    // Handle combo box selection change by user
    UpdatePatternDescription();
}

#pragma warning(pop)