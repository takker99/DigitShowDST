/**
 * @file DigitShowBasicDoc.cpp
 * @brief Implementation of CDigitShowBasicDoc class
 *
 * Implements document functionality including hardware control, data acquisition,
 * control algorithms, and data logging.
 * CDigitShowBasicDoc クラスの動作の定義を行います。
 */

#include "StdAfx.h"

#include "Board.hpp"
#include "Constants.h"
#include "DataConvert.h"
#include "DigitShowBasicDoc.h"
#include "File.hpp"
#include "Logging.hpp"
#include "Sensitivity.hpp"
#include "Variables.hpp"
#include "aio_error_logger.hpp"
#include "aio_wrapper.hpp"
#include "board_control.hpp"
#include "chrono_alias.hpp"
#include "control/control.hpp"
#include "control/measurement.hpp"
#include "control/patterns.hpp"
#include "control/timer.hpp"
#include "control/utils.hpp"
#include "physical_variables.hpp"
#include "resource.h"
#include "timer.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <format>
#include <spdlog/spdlog.h>
#include <utility>

using namespace board;
using namespace control;
using namespace sensitivity;
using namespace file;
using namespace variables;

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CDigitShowBasicDoc, CDocument)
BEGIN_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF(CDigitShowBasicDoc, CDocument)
//{{AFX_MSG_MAP(CDigitShowBasicDoc)
// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
//        この位置に生成されるコードを編集しないでください。
//}}AFX_MSG_MAP
END_MESSAGE_MAP_IGNORE_UNUSED_LOCAL_TYPEDEF()

/////////////////////////////////////////////////////////////////////////////

BOOL CDigitShowBasicDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // Initialize latest_physical_input with initial specimen (present = initial at startup)
    variables::physical::latest_physical_input.store({
        SpecimenData,
        0.0, // shear_force_N
        0.0, // vertical_force_N
        0.0, // shear_displacement_mm
        0.0, // front_vertical_disp_mm
        0.0, // rear_vertical_disp_mm
        0.0, // front_friction_force_N
        0.0, // rear_friction_force_N
    });

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

// CDigitShowBasicDoc シリアライゼーション
void CDigitShowBasicDoc::Serialize(CArchive &ar)
{
    if (ar.IsStoring())
    {
        // TODO: この位置に保存用のコードを追加してください。
    }
    else
    {
        // TODO: この位置に読み込み用のコードを追加してください。
    }
}

/////////////////////////////////////////////////////////////////////////////

// CDigitShowBasicDoc file writers management

bool CDigitShowBasicDoc::OpenSaveWriters(const std::filesystem::path &basePath)
{
    using namespace file;

    // Build file paths with appropriate suffixes
    const auto vltPath = std::filesystem::path(basePath).replace_extension("") += "_vlt.tsv";
    const auto phyPath = std::filesystem::path(basePath).replace_extension("tsv");
    const auto paramPath = std::filesystem::path(basePath).replace_extension("") += "_out.tsv";

    // Open voltage writer
    if (!m_vltWriter.open(vltPath, true))
    {
        spdlog::error("Failed to open voltage file: {}",
                      reinterpret_cast<const char *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                          vltPath.u8string().c_str()));
        return false;
    }

    // Open physical writer
    if (!m_phyWriter.open(phyPath, true))
    {
        spdlog::error("Failed to open physical file: {}",
                      reinterpret_cast<const char *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                          phyPath.u8string().c_str()));
        m_vltWriter.close();
        return false;
    }

    // Open parameter writer
    if (!m_paramWriter.open(paramPath, true))
    {
        spdlog::error("Failed to open parameter file: {}",
                      reinterpret_cast<const char *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                          paramPath.u8string().c_str()));
        m_vltWriter.close();
        m_phyWriter.close();
        return false;
    }

    // Reserve scratch buffer (typical line might be ~500-1000 chars)
    m_writeScratch.reserve(1024);

    // Write headers to each file
    // Voltage file header
    m_vltWriter.writeLine(
        "UnixTime(ms)\tCH00_(V)\tCH01_(V)\tCH02_(V)\tCH03_(V)\tCH04_(V)\tCH05_(V)\tCH06_(V)\tCH07_(V)");

    // Physical file header
    m_phyWriter.writeLine(
        "UnixTime(ms)\tLoad_(N)\tCell_P.(kPa)\tDisp.(mm)\tP.W.P(kPa)\tSP.Vol.(mm3)\tCH05_(V)\tV-LDT1_(mm)\tCH07_(V)");

    // Parameter file header
    m_paramWriter.writeLine(
        "UnixTime(ms)\ts(a)_(kPa)\ts(r)_(kPa)\ts'(a)(kPa)\ts'(r)(kPa)\tPore_(kPa)\tp____(kPa)\tq____(kPa)\tp'___(kPa)"
        "\te(a)_(%)_\te(r)_(%)_\te(vol)_(%)_\tAvLDT(%)_\tDA(%)inCOMP\tDA(%)inEXT\tCont_No__\tStep_time(s)");

    spdlog::info("Opened TSV writers: vlt={}, phy={}, param={}",
                 reinterpret_cast<const char *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                     vltPath.u8string().c_str()),
                 reinterpret_cast<const char *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                     phyPath.u8string().c_str()),
                 reinterpret_cast<const char *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                     paramPath.u8string().c_str()));

    return true;
}

void CDigitShowBasicDoc::CloseSaveWriters() noexcept
{
    m_vltWriter.close();
    m_phyWriter.close();
    m_paramWriter.close();
    spdlog::debug("Closed all TSV writers");
}

void CDigitShowBasicDoc::FlushWriters()
{
    m_vltWriter.flush();
    m_phyWriter.flush();
    m_paramWriter.flush();
    m_lastFlushTime = std::chrono::steady_clock::now();
    m_lastFlushedCurnum = control::CURNUM;
    spdlog::debug("Flushed all TSV writers (CURNUM={})", m_lastFlushedCurnum);
}

void CDigitShowBasicDoc::FlushWritersIfNeeded()
{
    using namespace std::chrono;

    // Skip flush if not initialized (ResetFlushState not called yet)
    if (m_lastFlushTime.time_since_epoch().count() == 0)
    {
        return;
    }

    // Flush if control step number changed
    if (control::CURNUM != m_lastFlushedCurnum)
    {
        spdlog::info("Control step changed ({} -> {}), flushing writers", m_lastFlushedCurnum, control::CURNUM);
        FlushWriters();
        return;
    }

    // Flush if more than 1 minute has elapsed since last flush
    constexpr auto kFlushInterval = 1min;
    const auto now = steady_clock::now();
    if (now - m_lastFlushTime >= kFlushInterval)
    {
        spdlog::debug("Flush interval elapsed, flushing writers");
        FlushWriters();
    }
}

void CDigitShowBasicDoc::ResetFlushState() noexcept
{
    m_lastFlushedCurnum = control::CURNUM;
    m_lastFlushTime = std::chrono::steady_clock::now();
}

/////////////////////////////////////////////////////////////////////////////

// CDigitShowBasicDoc クラスの診断
#ifdef _DEBUG
void CDigitShowBasicDoc::AssertValid() const
{
    CDocument::AssertValid();
}
void CDigitShowBasicDoc::Dump(CDumpContext &dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////

// CDigitShowBasicDoc コマンド
void CDigitShowBasicDoc::OpenBoard()
{
    using namespace board;

    spdlog::info("OpenBoard called");

    if (Flag_SetBoard)
    {
        spdlog::warn("Board already initialized, skipping OpenBoard");
        AfxMessageBox(_T("Initialization has been already accomplished"), MB_ICONSTOP | MB_OK);
        return;
    }

    // Device IDs: AIO000 (first A/D), AIO001 (D/A), AIO002 (second A/D)
    constexpr std::array<std::string_view, 2> kAdDeviceNames = {{"AIO000", "AIO002"}};
    constexpr std::array<std::string_view, 1> kDaDeviceNames = {{"AIO001"}};

    // Use board_control layer to initialize all boards
    const auto result = board_control::InitializeBoards(std::span{kAdDeviceNames}, std::span{kDaDeviceNames});

    if (!result)
    {
        AfxMessageBox(CA2W(std::format("Board initialization failed: {}", result.error()).c_str(), CP_UTF8),
                      MB_ICONSTOP | MB_OK);
        return;
    }

    spdlog::info("Board initialization completed successfully via board_control layer");
}

void CDigitShowBasicDoc::CloseBoard()
{
    using namespace board;

    spdlog::info("CloseBoard called");

    if (!Flag_SetBoard)
    {
        spdlog::debug("No boards to close (Flag_SetBoard is false)");
        return;
    }

    // Use board_control layer to close all boards
    if (const auto result = board_control::CloseBoards(); !result)
    {
        spdlog::warn("Error closing boards: {}", result.error());
    }
}
//--- Input from A/D Board ---
void CDigitShowBasicDoc::AD_INPUT()
{
    // Use board_control layer
    if (const auto result = board_control::ReadAnalogInputs(); !result)
    {
        aio::log_error("AD_INPUT via board_control", result.error());
    }
}

//--- Output to D/A Board ---
void CDigitShowBasicDoc::DA_OUTPUT()
{
    // Use board_control layer
    if (const auto result = board_control::WriteAnalogOutputs(); !result)
    {
        aio::log_error("DA_OUTPUT via board_control", result.error());
    }
}
//--- Calcuration of the Other Parameters ---
void CDigitShowBasicDoc::Cal_Param()
{
    // Build PhysicalInput with initial specimen and Phyout values (initial-based displacements)
    const PhysicalInput initial_based_input{
        SpecimenData,
        Phyout[0], // shear load component
        Phyout[1], // cell load component
        Phyout[2], // shear displacement
        Phyout[3], // front vertical displacement (initial-based)
        Phyout[4], // rear vertical displacement (initial-based)
        Phyout[5], // front friction force
        Phyout[6], // rear friction force
    };

    // Get current present specimen from latest_physical_input
    const auto current_input = variables::physical::latest_physical_input.load();
    const auto &present_specimen = current_input.specimen;

    // Rebase to present specimen (convert initial-based displacements to present-based)
    const auto present_based_input = rebase(initial_based_input, present_specimen);

    variables::physical::latest_physical_input.store(present_based_input);

    variables::physical::latest_physical_output.store(
        {fromVoltage(static_cast<double>(DAVout[CH_EP_Cell_f]), DA_Cal_a[CH_EP_Cell_f], DA_Cal_b[CH_EP_Cell_f]),
         fromVoltage(static_cast<double>(DAVout[CH_EP_Cell_r]), DA_Cal_a[CH_EP_Cell_r], DA_Cal_b[CH_EP_Cell_r]),
         fromIISMotorVoltage(DAVout[CH_Motor], DAVout[CH_MotorCruch], DAVout[CH_MotorSpeed], DA_Cal_a[CH_MotorSpeed],
                             DA_Cal_b[CH_MotorSpeed])

        });
}

void CDigitShowBasicDoc::SaveToFile()
{
    using namespace file;

    // Save Voltage and Physical Data
    // Synthetic stable wall-clock time for UnixTime(ms)
    const auto syntheticNow = board::SyntheticNow();
    const auto unixTimeMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(syntheticNow.time_since_epoch()).count();

    // Build voltage line
    m_writeScratch.clear();
    std::format_to(std::back_inserter(m_writeScratch), "{}\t", unixTimeMs);
    size_t k = 0;
    for (size_t i = 0; i < dsb::NUMAD; i++)
    {
        for (size_t j = 0; std::cmp_less(j, AdChannels[i] / 2); j++)
        { // 2023.2    1/2を戻してCH１つを実施
            std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", Vout[k]);
            k++;
        }
    }
    m_writeScratch.back() = '\n'; // Replace last tab with newline
    m_vltWriter.stream().write(m_writeScratch.data(), static_cast<std::streamsize>(m_writeScratch.size()));

    // Build physical line
    m_writeScratch.clear();
    std::format_to(std::back_inserter(m_writeScratch), "{}\t", unixTimeMs);
    k = 0;
    for (size_t i = 0; i < dsb::NUMAD; i++)
    {
        for (size_t j = 0; std::cmp_less(j, AdChannels[i] / 2); j++)
        {
            std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", Phyout[k]);
            k++;
        }
    }
    m_writeScratch.back() = '\n'; // Replace last tab with newline
    m_phyWriter.stream().write(m_writeScratch.data(), static_cast<std::streamsize>(m_writeScratch.size()));

    // Build parameter line using snapshot values (no reliance on CalParam)
    m_writeScratch.clear();
    std::format_to(std::back_inserter(m_writeScratch), "{}\t", unixTimeMs);
    const auto physical_input = variables::physical::latest_physical_input.load();
    const auto physical_output = variables::physical::latest_physical_output.load();
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", physical_input.shear_stress_kpa());    // param[0]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", physical_input.shear_displacement_mm); // param[1]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", physical_input.vertical_stress_kpa()); // param[2]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t",
                   physical_input.normal_displacement_mm());                                  // param[3]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", physical_input.tilt_mm()); // param[4]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t",
                   physical_input.front_friction_force_N);                                                // param[5]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", physical_input.rear_friction_force_N); // param[6]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", physical_output.motor_rpm);            // param[7]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", physical_output.front_ep_kpa);         // param[8]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", physical_output.rear_ep_kpa);          // param[9]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", DAVout[CH_MotorSpeed]);                // param[10]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", DAVout[CH_EP_Cell_f]);                 // param[11]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", DAVout[CH_EP_Cell_r]);                 // param[12]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", static_cast<double>(NUM_Cyclic));      // param[13]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t", static_cast<double>(CURNUM));          // param[14]
    std::format_to(std::back_inserter(m_writeScratch), "{:.6f}\t",
                   std::chrono::seconds_d{control::TotalStepTime}.count()); // param[15]
    m_writeScratch.back() = '\n';                                           // Replace last tab with newline
    m_paramWriter.stream().write(m_writeScratch.data(), static_cast<std::streamsize>(m_writeScratch.size()));
}
//--- Control Statements ---
void CDigitShowBasicDoc::Start_Control()
{
    spdlog::info("Control started");
}
void CDigitShowBasicDoc::Stop_Control()
{
    spdlog::info("Control stopped");
    DAVout[CH_Motor] = 0.0f;      // Motor Stop
    DAVout[CH_MotorSpeed] = 0.0f; // Motor Speed->0
    DA_OUTPUT();
    spdlog::debug("Motor stopped, DAVout reset");
}

static const double &ERR_StressM = ERR_StressCom; // Sensitivity of Stress Control by Motor
static constexpr double ERR_RPM = 1.0;            // Sensitivity of Motor to avoid over/under shooting
static const double &ERR_StressP =
    ERR_StressA; // (kPa)　「三軸プログラムにおけるsensitivityであるERR_StressA」を鉛直ベロフラムのためのERR_StressPを変えるためのパラメータとした。
static const double &ERR_Disp =
    ERR_StressExt; // (mm)　「三軸プログラムにおけるsensitivityであるERR_StressExt」を左右の鉛直LVDTの差に関するERR_Dispを変えるためのパラメータとした。
static constexpr double amp = 0.5, amp2F = 0.5,
                        amp2R = 0.5; // Sensitivity of stress and displacement during shearing control
static constexpr double Dmax = 1.2;  // Maximum output change per step (kPa)
static constexpr double ERR_DispCV = 0.002, ampCV = 17.0; // Sensitivity for Constant Volume control

void CDigitShowBasicDoc::Control_DA()
{
    // Bounds check for control_steps
    if (control::CURNUM >= control::control_steps.size())
    {
        spdlog::error("Control_DA: CURNUM={} out of bounds (size={}), stopping control", control::CURNUM,
                      control::control_steps.size());
        return;
    }

    // Get current control step for easier access
    const auto &current_step = control::control_steps[control::CURNUM];

    // Maintain control state across calls (for cyclic patterns)
    static ControlOutput control_output;

    // Accumulate elapsed time using chrono types directly
    using namespace std::chrono;
    control::TotalStepTime += control::CtrlStepTime;

    spdlog::debug("Control_DA tick: CtrlStepTime={}, TotalStepTime={}, CURNUM={}, pattern={}",
                  duration_cast<milliseconds>(control::CtrlStepTime), seconds_d{control::TotalStepTime},
                  control::CURNUM, get_use_label(current_step.parameters.pattern));

    if (current_step.parameters.pattern == ControlPattern::NoControl)
    {
        spdlog::trace("pattern == NoControl, stopping motor");
        stop_motor(DAVout[CH_Motor], DAVout[CH_MotorSpeed]);
    }

    // Initialize control_output with current DAVout values to allow incremental updates
    control_output.front_ep_kpa = fromVoltage(DAVout[CH_EP_Cell_f], DA_Cal_a[CH_EP_Cell_f], DA_Cal_b[CH_EP_Cell_f]);
    control_output.rear_ep_kpa = fromVoltage(DAVout[CH_EP_Cell_r], DA_Cal_a[CH_EP_Cell_r], DA_Cal_b[CH_EP_Cell_r]);
    control_output.motor_rpm = fromIISMotorVoltage(DAVout[CH_Motor], DAVout[CH_MotorCruch], DAVout[CH_MotorSpeed],
                                                   DA_Cal_a[CH_MotorSpeed], DA_Cal_b[CH_MotorSpeed]);
    control_output.step_completed = true;

    const auto &p = current_step.parameters;
    const EPConstantPressureParams ep_cp_params{.target_sigma = p.target_sigma_kpa,
                                                .err_stress = fallback(p.err_stress_kpa, ERR_StressP),
                                                .err_disp = fallback(p.err_disp_mm, ERR_Disp),
                                                .amp = fallback(p.amp_v_per_kpa_m2, amp),
                                                .amp2_f = fallback(p.amp2_f_v_per_mm, amp2F),
                                                .amp2_r = fallback(p.amp2_r_v_per_mm, amp2R),
                                                .dmax = fallback(p.dmax_v, Dmax)};
    const EPConstantVolumeParams ep_cv_params{.err_disp_cv = fallback(p.err_disp_cv_mm, ERR_DispCV),
                                              .amp_cv = fallback(p.amp_cv_v_per_mm, ampCV),
                                              .dmax = fallback(p.dmax_v, Dmax)};
    MonotonicMotorControlParams motor_params{
        .target_tau = p.target_tau_kpa, .motor_rpm = p.motor_rpm, .loading_dir = p.loading_dir};
    CyclicMotorControlParams cyclic_motor_params{.loading_dir = p.loading_dir,
                                                 .motor_rpm = p.motor_rpm,
                                                 .tau_lower = p.target_tau_kpa,
                                                 .tau_upper = p.target_sigma_kpa,
                                                 .num_cycles = p.cycles};
    MonotonicMotorControlDispParams motor_disp_params{
        .target_es = p.target_displacement_mm,
        .motor_rpm = p.motor_rpm,
        .loading_dir = p.loading_dir,

    };
    CyclicMotorControlDispParams cyclic_motor_disp_params{.loading_dir = p.loading_dir,
                                                          .motor_rpm = p.motor_rpm,
                                                          .displacement_lower = p.displacement_lower_mm,
                                                          .displacement_upper = p.displacement_upper_mm,
                                                          .num_cycles = p.cycles};
    MotorAccelerationParams motor_accel_params{.loading_dir = p.loading_dir,
                                               .start_rpm = p.motor_rpm,
                                               .target_rpm =
                                                   p.target_sigma_kpa, // TODO: 必要なpropsがControlParamsにない、要修正
                                               .acceleration = p.target_tau_kpa, //
                                               .err_rpm = ERR_RPM,
                                               .time_interval_min = minutes_d{timer::TimeInterval_2}};
    MotorTauControlParams motor_tau_params{.target_tau = p.target_tau_kpa,
                                           .err_stress = ERR_StressM,
                                           .max_rpm = p.motor_rpm,
                                           .loading_dir = p.loading_dir};
    TimerControlParams timer_params{
        .target_duration = duration_cast<steady_clock::duration>(p.duration),
        .elapsed_time = TotalStepTime,
    };

    using variables::physical::latest_physical_input;
    using variables::physical::latest_physical_output;

    switch (current_step.parameters.pattern)
    {
        using enum ControlPattern;
    case ConstantTauConsolidation: {
        static double inital_sigma =
            latest_physical_input.load().vertical_stress_kpa(); // Initial sigma for consolidation step
        ConstantTauConsolidationParams params{.direction = p.loading_dir,
                                              .consolidation_rate = p.consolidation_rate_kpa_per_min,
                                              .target_sigma = p.target_sigma_kpa,
                                              .initial_sigma = inital_sigma,
                                              .elapsed_time = minutes_d{TotalStepTime},
                                              .err_stress_p = fallback(p.err_stress_kpa, ERR_StressP),
                                              .err_disp = fallback(p.err_disp_mm, ERR_Disp),
                                              .amp = fallback(p.amp_v_per_kpa_m2, amp),
                                              .amp2_f = fallback(p.amp2_f_v_per_mm, amp2F),
                                              .amp2_r = fallback(p.amp2_r_v_per_mm, amp2R),
                                              .dmax = fallback(p.dmax_v, Dmax)};
        control_output = Constant_Tau_Consolidation(params, motor_tau_params, latest_physical_input, control_output);
        if (control_output.step_completed)
            inital_sigma = 0.0; // Reset initial sigma for next consolidation
        break;
    }
    case KConsolidation: {
        PathMLoadingConstantPressureParams params{.loading_dir = p.loading_dir,
                                                  .target_tau = p.target_tau_kpa,
                                                  .sigma_start = p.target_sigma_kpa,
                                                  .sigma_end = static_cast<double>(p.cycles),
                                                  .k_value = p.target_sigma_kpa,
                                                  .err_stress_p = fallback(p.err_stress_kpa, ERR_StressP),
                                                  .err_disp = fallback(p.err_disp_mm, ERR_Disp),
                                                  .amp = fallback(p.amp_v_per_kpa_m2, amp),
                                                  .amp2_f = fallback(p.amp2_f_v_per_mm, amp2F),
                                                  .amp2_r = fallback(p.amp2_r_v_per_mm, amp2R),
                                                  .dmax = fallback(p.dmax_v, Dmax)};
        control_output = Path_MLoading_Constant_Pressure(params, motor_params, latest_physical_input, control_output);
        break;
    }
    case PreConsolidation: {
        static double inital_sigma =
            latest_physical_input.load().vertical_stress_kpa(); // Initial sigma for consolidation step
        const auto &physical_input = latest_physical_input.load();
        const double area = physical_input.specimen.area_mm2();

        control_output.front_ep_kpa += 0.2 * -inital_sigma * area / 1000.0;
        control_output.rear_ep_kpa += 0.2 * -inital_sigma * area / 1000.0;

        control_output = apply_motor_tau_control(motor_tau_params, latest_physical_input, control_output);
        if (control_output.step_completed)
            inital_sigma = 0.0; // Reset initial sigma for next consolidation
        break;
    }
    case BeforeConsolidation:
    case AfterConsolidation: {
        // Get current physical input
        auto expected = latest_physical_input.load();

        while (!latest_physical_input.compare_exchange_weak(
            expected,
            // Rebase to new present (this zero-sets displacements relative to new present)
            control::rebase(expected, control::present_specimen(expected))))
        {
        }

        control_output.step_completed = true;
        break;
    }
    case NoControl:
        // No control action needed
        break;
    default: {
#pragma warning(push)
#pragma warning(disable : 4061 5246)
        // EP control
        if (std::ranges::contains(std::array{MonotonicLoadingConstantPressure, CyclicLoadingConstantPressure,
                                             CreepConstantPressure, RelaxationConstantPressure,
                                             MonotonicLoadingDisplacementConstantPressure,
                                             CyclicLoadingDisplacementConstantPressure, AccelerationConstantPressure,
                                             CreepConstantPressureFast, CreepConstantPressureFastRef},
                                  current_step.parameters.pattern))
        {
            control_output = apply_ep_constant_pressure_control(ep_cp_params, latest_physical_input, control_output);
        }
        else if (std::ranges::contains(std::array{MonotonicLoadingConstantVolume, CyclicLoadingConstantVolume,
                                                  CreepConstantVolume, RelaxationConstantVolume,
                                                  MonotonicLoadingDisplacementConstantVolume,
                                                  CyclicLoadingDisplacementConstantVolume, AccelerationConstantVolume},
                                       current_step.parameters.pattern))
        {
            control_output = apply_ep_constant_volume_control(ep_cv_params, latest_physical_input, control_output);
        }

        // Motor control
        if (std::ranges::contains(std::array{MonotonicLoadingConstantPressure, MonotonicLoadingConstantVolume},
                                  current_step.parameters.pattern))
        {
            control_output = apply_monotonic_motor_control(motor_params, latest_physical_input, control_output);
            break;
        }
        if (std::ranges::contains(
                std::array{MonotonicLoadingDisplacementConstantPressure, MonotonicLoadingDisplacementConstantVolume},
                current_step.parameters.pattern))
        {
            control_output =
                apply_monotonic_motor_control_disp(motor_disp_params, latest_physical_input, control_output);
            break;
        }
        if (std::ranges::contains(std::array{CyclicLoadingConstantPressure, CyclicLoadingConstantVolume},
                                  current_step.parameters.pattern))
        {
            control_output = apply_cyclic_motor_control(cyclic_motor_params, latest_physical_input, control_output);
            break;
        }
        if (std::ranges::contains(
                std::array{CyclicLoadingDisplacementConstantPressure, CyclicLoadingDisplacementConstantVolume},
                current_step.parameters.pattern))
        {
            control_output =
                apply_cyclic_motor_control_disp(cyclic_motor_disp_params, latest_physical_input, control_output);
            break;
        }
        if (std::ranges::contains(std::array{AccelerationConstantPressure, AccelerationConstantVolume},
                                  current_step.parameters.pattern))
        {
            control_output = apply_motor_acceleration(motor_accel_params, latest_physical_input, control_output);
            break;
        }

        if (current_step.parameters.pattern == CreepConstantPressureFast)
        {
            control_output = apply_monotonic_motor_control(motor_params, latest_physical_input, control_output);
            // tau controlもするのでbreakしない
        }
        if (current_step.parameters.pattern == CreepConstantPressureFastRef)
        {
            auto motor_ref_params = motor_params;
            motor_ref_params.target_tau =
                p.target_tau_kpa; // TODO: reference tauである必要があるがControlParamsから欠けている、要修正
            control_output = apply_monotonic_motor_control(motor_ref_params, latest_physical_input, control_output);
            // tau controlもするのでbreakしない
        }
        if (std::ranges::contains(std::array{CreepConstantPressure, CreepConstantVolume, CreepConstantPressureFast,
                                             CreepConstantPressureFastRef},
                                  current_step.parameters.pattern))
        {
            control_output = apply_motor_tau_control(motor_tau_params, latest_physical_input, control_output);
            // timer controlもするのでbreakしない
        }
        if (std::ranges::contains(std::array{RelaxationConstantPressure, RelaxationConstantVolume},
                                  current_step.parameters.pattern))
        {
            control_output.motor_rpm = 0;
            // timer controlもするのでbreakしない
        }
        if (std::ranges::contains(std::array{CreepConstantPressure, CreepConstantVolume, RelaxationConstantPressure,
                                             RelaxationConstantVolume, CreepConstantPressureFast,
                                             CreepConstantPressureFastRef},
                                  current_step.parameters.pattern))
        {
            control_output = apply_timer_control(timer_params, control_output);
            break;
        }
        spdlog::warn("Unknown pattern: {}", static_cast<size_t>(current_step.parameters.pattern));
        break;
    }
    }
#pragma warning(pop)

    if (control_output.step_completed)
    {
        spdlog::info("Control step {} ({}) completed, advancing to next step", CURNUM,
                     get_use_label(current_step.parameters.pattern));
        TotalStepTime = decltype(TotalStepTime){};
        CURNUM++;
        // Reset control_output for next step
        control_output = ControlOutput{};
    }

    // Apply control output to global state
    const auto [motor_on_voltage, motor_clutch_voltage, motor_speed_voltage] =
        toIISMotorVoltage(control_output.motor_rpm, DA_Cal_a[CH_MotorSpeed], DA_Cal_b[CH_MotorSpeed]);
    // クラッチの消耗を抑えるため、モーターが回転していないときはクラッチを操作しない
    DAVout[CH_Motor] = motor_speed_voltage > 0.f ? motor_on_voltage : DAVout[CH_Motor];
    DAVout[CH_MotorCruch] = motor_speed_voltage > 0.f ? motor_clutch_voltage : DAVout[CH_MotorCruch];
    DAVout[CH_MotorSpeed] = motor_speed_voltage;
    DAVout[CH_EP_Cell_f] =
        static_cast<float>(toVoltage(control_output.front_ep_kpa, DA_Cal_a[CH_EP_Cell_f], DA_Cal_b[CH_EP_Cell_f]));
    DAVout[CH_EP_Cell_r] =
        static_cast<float>(toVoltage(control_output.rear_ep_kpa, DA_Cal_a[CH_EP_Cell_r], DA_Cal_b[CH_EP_Cell_r]));

    // Update global cyclic state for backward compatibility
    NUM_Cyclic = control_output.num_cyclic;
    DA_OUTPUT();
}
