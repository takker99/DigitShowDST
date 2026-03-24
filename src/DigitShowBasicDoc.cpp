/*
 * DigitShowBasic - Triaxial Test Machine Control Software
 * Copyright (C) 2025 Makoto KUNO
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "StdAfx.h"

#include "CAIO.H"
#include "DataConvert.h"
#include "DigitShowBasic.h"
#include "DigitShowBasicDoc.h"

#include "math.h"
#include "time.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CDigitShowBasicDoc, CDocument)

BEGIN_MESSAGE_MAP(CDigitShowBasicDoc, CDocument)
//{{AFX_MSG_MAP(CDigitShowBasicDoc)
// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
//        この位置に生成されるコードを編集しないでください。
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDigitShowBasicDoc クラスの構築/消滅

CDigitShowBasicDoc::CDigitShowBasicDoc()
{
}

CDigitShowBasicDoc::~CDigitShowBasicDoc()
{
}

BOOL CDigitShowBasicDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // (SDI ドキュメントはこのドキュメントを再利用します。)

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDigitShowBasicDoc シリアライゼーション

void CDigitShowBasicDoc::Serialize(CArchive &ar)
{
    if (ar.IsStoring())
    {
    }
    else
    {
    }
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
    DigitShowContext *ctx = GetContext();
    int i = 0;

    if (ctx->FlagSetBoard)
    {
        AfxMessageBox(_T("Initialization has been already accomplished"), MB_ICONSTOP | MB_OK);
        return;
    }
    else
    {
        // OPEN A/D BOARDS.
        if (ctx->NumAD > 0)
        {
            char ad0[] = "AIO000";
            ctx->Ret = AioInit(ad0, &ctx->ad.Id[0]);
            if (ctx->Ret != 0)
            {
                ctx->Ret2 = AioGetErrorString(ctx->Ret, ctx->ErrorString);
                ctx->TextString.Format(_T("AioInit = %d : %S"), ctx->Ret, ctx->ErrorString);
                AfxMessageBox(ctx->TextString, MB_ICONSTOP | MB_OK);
                return;
            }
            else
            {
                ctx->Ret = AioResetDevice(ctx->ad.Id[0]);
                if (ctx->Ret != 0)
                {
                    ctx->Ret2 = AioGetErrorString(ctx->Ret, ctx->ErrorString);
                    ctx->TextString.Format(_T("AioResetDevice = %d : %S"), ctx->Ret, ctx->ErrorString);
                    AfxMessageBox(ctx->TextString, MB_ICONSTOP | MB_OK);
                    return;
                }
            }
        }
        if (ctx->NumAD > 1)
        {
            char ad1[] = "AIO001";
            ctx->Ret = AioInit(ad1, &ctx->ad.Id[1]);
            if (ctx->Ret != 0)
            {
                ctx->Ret2 = AioGetErrorString(ctx->Ret, ctx->ErrorString);
                ctx->TextString.Format(_T("AioInit = %d : %S"), ctx->Ret, ctx->ErrorString);
                AfxMessageBox(ctx->TextString, MB_ICONSTOP | MB_OK);
                return;
            }
            else
            {
                ctx->Ret = AioResetDevice(ctx->ad.Id[1]);
                if (ctx->Ret != 0)
                {
                    ctx->Ret2 = AioGetErrorString(ctx->Ret, ctx->ErrorString);
                    ctx->TextString.Format(_T("AioResetDevice = %d : %S"), ctx->Ret, ctx->ErrorString);
                    AfxMessageBox(ctx->TextString, MB_ICONSTOP | MB_OK);
                    return;
                }
            }
        }
        // OPEN D/A BOARDS.
        if (ctx->NumDA > 0)
        {
            char da0[] = "AIO001";
            ctx->Ret = AioInit(da0, &ctx->da.Id[0]);
            if (ctx->Ret != 0)
            {
                ctx->Ret2 = AioGetErrorString(ctx->Ret, ctx->ErrorString);
                ctx->TextString.Format(_T("AioInit = %d : %S"), ctx->Ret, ctx->ErrorString);
                AfxMessageBox(ctx->TextString, MB_ICONSTOP | MB_OK);
                return;
            }
            else
            {
                ctx->Ret = AioResetDevice(ctx->da.Id[0]);
                if (ctx->Ret != 0)
                {
                    ctx->Ret2 = AioGetErrorString(ctx->Ret, ctx->ErrorString);
                    ctx->TextString.Format(_T("AioResetDevice = %d : %S"), ctx->Ret, ctx->ErrorString);
                    AfxMessageBox(ctx->TextString, MB_ICONSTOP | MB_OK);
                    return;
                }
            }
        }
        // Set Sampling Condition
        for (i = 0; i < ctx->NumAD; i++)
        {
            ctx->Ret = AioGetAiInputMethod(ctx->ad.Id[i], &ctx->ad.InputMethod[i]);
            ctx->Ret = AioGetAiResolution(ctx->ad.Id[i], &ctx->ad.Resolution[i]);
            ctx->Ret = AioGetAiMaxChannels(ctx->ad.Id[i], &ctx->ad.Channels[i]);
            ctx->Ret = AioSetAiChannels(ctx->ad.Id[i], ctx->ad.Channels[i]);
            ctx->AdMaxChannels += ctx->ad.Channels[i] / 2;
            ctx->Ret = AioSetAiRangeAll(ctx->ad.Id[i], 0);
            // (-10V, 10V)
            ctx->Ret = AioGetAiRange(ctx->ad.Id[i], 0, &ctx->ad.Range[i]);
            ctx->Ret = GetRangeValue(ctx->ad.Range[i], &ctx->ad.RangeMax[i], &ctx->ad.RangeMin[i]);
            ctx->Ret = AioGetAiMemoryType(ctx->ad.Id[i], &ctx->ad.MemoryType[i]);
            ctx->Ret = AioGetAiSamplingClock(ctx->ad.Id[i], &ctx->ad.SamplingClock[i]);
            // 2020.02.13 M.Kuno ScanClock制御を追加
            {
                short maxChannels = 64;
                ctx->Ret = AioGetAiMaxChannels(ctx->ad.Id[i], &maxChannels);
                float scanClock = 1000.f / maxChannels;
                ctx->Ret = AioSetAiScanClock(ctx->ad.Id[i], scanClock);
                ctx->Ret = AioGetAiScanClock(ctx->ad.Id[i], &ctx->ad.ScanClock[i]);
            }
            ctx->Ret = AioGetAiEventSamplingTimes(ctx->ad.Id[i], &ctx->ad.SamplingTimes[i]);
        }
        ctx->sampling.SavingTime = 300;
        ctx->sampling.TotalSamplingTimes = long(ctx->sampling.SavingTime * 1000000 / ctx->ad.SamplingClock[0]);
        ctx->sampling.AllocatedMemory = 4 * ctx->AdMaxChannels * ctx->sampling.TotalSamplingTimes / 1024.0f / 1024.0f;
        ctx->sampling.AvSmplNum = 20;
        for (i = 0; i < ctx->NumDA; i++)
        {
            ctx->Ret = AioGetAoResolution(ctx->da.Id[i], &ctx->da.Resolution[i]);
            ctx->Ret = AioGetAoMaxChannels(ctx->da.Id[i], &ctx->da.Channels[i]);
            ctx->Ret = AioSetAoRangeAll(ctx->da.Id[i], 50);
            // 0 - 10V
            ctx->Ret = AioGetAoRange(ctx->da.Id[i], 0, &ctx->da.Range[i]);
            ctx->Ret = GetRangeValue(ctx->da.Range[i], &ctx->da.RangeMax[i], &ctx->da.RangeMin[i]);
        }
        ctx->FlagSetBoard = TRUE;
    }
    return;
}

void CDigitShowBasicDoc::CloseBoard()
{
    DigitShowContext *ctx = GetContext();
    // Close A/D and D/A board to end the application
    if (ctx->FlagSetBoard == TRUE)
    {
        // Ensure all D/A outputs are driven to 0V before releasing the D/A board.
        ZeroAllDaOutputsOnShutdown();

        if (ctx->NumAD > 0)
            ctx->Ret = AioExit(ctx->ad.Id[0]);
        if (ctx->NumAD > 1)
            ctx->Ret = AioExit(ctx->ad.Id[1]);
        if (ctx->NumDA > 0)
            ctx->Ret = AioExit(ctx->da.Id[0]);

        ctx->FlagSetBoard = FALSE;
        ctx->FlagCtrl = FALSE;
    }
}

void CDigitShowBasicDoc::ZeroAllDaOutputsOnShutdown()
{
    DigitShowContext *ctx = GetContext();

    if (ctx->FlagSetBoard == FALSE || ctx->NumDA <= 0)
        return;

    for (int i = 0; i < 8; i++)
    {
        ctx->DAVout[i] = 0.0f;
    }

    DA_OUTPUT();

    if (ctx->Ret != 0)
    {
        ctx->Ret2 = AioGetErrorString(ctx->Ret, ctx->ErrorString);
        ctx->TextString.Format(_T("AioMultiAo (shutdown) = %d : %S"), ctx->Ret, ctx->ErrorString);
    }
}

//--- Input from A/D Board ---
void CDigitShowBasicDoc::AD_INPUT()
{
    DigitShowContext *ctx = GetContext();
    int i = 0, j = 0, k = 0;
    k = 0;
    if (ctx->NumAD > 0)
    {
        for (i = 0; i < ctx->ad.Channels[0] / 2; i++)
        {
            ctx->Vout[k] = 0.0f;
            for (j = 0; j < ctx->sampling.AvSmplNum; j++)
            {
                ctx->Vout[k] += BinaryToVolt(ctx->ad.RangeMax[0], ctx->ad.RangeMin[0], ctx->ad.Resolution[0],
                                             ctx->ad.Data0[ctx->ad.Channels[0] * j + 2 * i]) /
                                float(ctx->sampling.AvSmplNum);
            }
            ++k;
        }
    }
    if (ctx->NumAD > 1)
    {
        for (i = 0; i < ctx->ad.Channels[1] / 2; i++)
        {
            ctx->Vout[k] = 0.0f;
            for (j = 0; j < ctx->sampling.AvSmplNum; j++)
            {
                ctx->Vout[k] += BinaryToVolt(ctx->ad.RangeMax[1], ctx->ad.RangeMin[1], ctx->ad.Resolution[1],
                                             ctx->ad.Data1[ctx->ad.Channels[1] * j + 2 * i]) /
                                float(ctx->sampling.AvSmplNum);
            }
            ++k;
        }
    }
}

//--- Output to D/A Board ---
void CDigitShowBasicDoc::DA_OUTPUT()
{
    DigitShowContext *ctx = GetContext();
    int i = 0, j = 0, k = 0;
    k = 0;
    for (i = 0; i < ctx->NumDA; i++)
    {
        for (j = 0; j < ctx->da.Channels[i]; j++)
        {
            if (ctx->DAVout[k] < 0.0f)
                ctx->DAVout[k] = 0.0f;
            if (ctx->DAVout[k] > 9.9999f)
                ctx->DAVout[k] = 9.9999f;
            ctx->da.Data[j] =
                VoltToBinary(ctx->da.RangeMax[i], ctx->da.RangeMin[i], ctx->da.Resolution[i], ctx->DAVout[k]);
            ++k;
        }
        ctx->Ret = AioMultiAo(ctx->da.Id[i], ctx->da.Channels[i], &ctx->da.Data[0]);
    }
}

//--- Calcuration of Physical Value ---
void CDigitShowBasicDoc::Cal_Physical()
{
    DigitShowContext *ctx = GetContext();
    int i = 0;
    for (i = 0; i < 64; i++)
    {
        ctx->Phyout[i] = ctx->cal.a[i] * ctx->Vout[i] * ctx->Vout[i] + ctx->cal.b[i] * ctx->Vout[i] + ctx->cal.c[i];
    }
}

//--- Calcuration of the Other Parameters ---
void CDigitShowBasicDoc::Cal_Param()
{
    DigitShowContext *ctx = GetContext();
    const int NUM_Cyclic = ctx->NumCyclic;
    int CURNUM = ctx->controlFile.CurrentNum;
    double TotalStepTime = ctx->TotalStepTime;

    //	Specimen Data in drain and undrain condition
    ctx->height = ctx->specimen.Height[0] - ctx->Phyout[1];          // Current height
    ctx->volume = ctx->specimen.Volume[0] - ctx->Phyout[4];          // Current volume in drain condition
    ctx->area = ctx->volume / ctx->height;                           // Current area
    ctx->phys.ea = ctx->Phyout[1] / ctx->specimen.Height[0] * 100.0; // Usual Axial Strain (%)
    ctx->phys.ev = ctx->Phyout[4] / ctx->specimen.Volume[0] * 100.0; // Usual Volumetric Strain in drain condition (%)
    ctx->phys.er = (ctx->Phyout[10] + ctx->Phyout[12] + ctx->Phyout[14]) / 3.0 / ctx->specimen.Diameter[0] *
                   100.0; // 2020.5   (ev-ea)/2.0より変更, CGの測定値より計算

    // Phyout5から6へ（LDT・CGのみ１つ飛ばしに）2020.5
    // 2021.3LDTの値が負の場合でもひずみを計算可能に
    ctx->phys.eLDT1 = ctx->Phyout[6] / ctx->specimen.VLDT1[0] * 100.0; // Usual LDT Strain (%)

    // Phyout6から8へ（LDT・CGのみ１つ飛ばしに）2020.5
    // 2021.3LDTの値が負の場合でもひずみを計算可能に
    ctx->phys.eLDT2 = ctx->Phyout[8] / ctx->specimen.VLDT2[0] * 100.0; // Usual LDT Strain (%)

    ctx->phys.eLDT = (ctx->phys.eLDT1 + ctx->phys.eLDT2) / 2.0;
    //
    ctx->phys.q = ctx->Phyout[0] / ctx->area * 1000.0;             // Deviator Stress (kPa)
    ctx->phys.sr = ctx->Phyout[2];                                 // Cell(Radial) Stress (kPa)
    ctx->phys.sa = ctx->phys.q + ctx->phys.sr;                     // Axial Stress (kPa)
    ctx->phys.p = (ctx->phys.sa + 2.0 * ctx->phys.sr) / 3.0;       // Mean Principal Stress (kPa)
    ctx->phys.e_sr = ctx->Phyout[2] - ctx->Phyout[3];              // Cell Effective Stress (kPa)  2019.8
    ctx->phys.e_sa = ctx->phys.q + ctx->phys.e_sr;                 // Axial Effective Stress (kPa)
    ctx->phys.u = ctx->Phyout[3];                                  // Pore Pressure  2019.8
    ctx->phys.e_p = (ctx->phys.e_sa + 2.0 * ctx->phys.e_sr) / 3.0; // Mean Effective Stress (kPa)
                                                                   //
    //---The Value to display---
    ctx->CalParam[0] = ctx->phys.sa;
    ctx->CalParam[1] = ctx->phys.sr;
    ctx->CalParam[2] = ctx->phys.e_sa;
    ctx->CalParam[3] = ctx->phys.e_sr;
    ctx->CalParam[4] = ctx->phys.u;
    ctx->CalParam[5] = ctx->cyclicState.DaFlag;
    ctx->CalParam[6] = ctx->phys.q;
    ctx->CalParam[7] = ctx->phys.e_p;
    ctx->CalParam[8] = ctx->phys.ea;
    ctx->CalParam[9] = NUM_Cyclic;
    ctx->CalParam[10] = ctx->cyclicState.RuIndicator;
    ctx->CalParam[11] = ctx->phys.eLDT;
    ctx->CalParam[12] =
        ctx->phys.ea - ctx->cyclicState.MinAxialStrain; // 2020.5    eLDT2から変更   圧縮載荷時のDA(%)の表示
    ctx->CalParam[13] =
        ctx->cyclicState.MaxAxialStrain - ctx->phys.ea; // 2020.5    eLDTから変更   伸張載荷時のDA(%)の表示
    ctx->CalParam[14] = CURNUM;                         // 2020.5   (e_sa+e_sr)/2.0から変更　　コントロールNo.の表示
    ctx->CalParam[15] = TotalStepTime * 60;
    //  2020.5 (e_sa+e_sr)/2.0から変更　経過時間の表示を可能にした。Phyout[15]= TotalStepTime * 60;などでも可能
}

void CDigitShowBasicDoc::SaveToFile()
{
    DigitShowContext *ctx = GetContext();
    if (ctx->FileSaveData0 == nullptr || ctx->FileSaveData1 == nullptr || ctx->FileSaveData2 == nullptr)
        return;
    // Save Voltage and Physical Data
    int i = 0, j = 0, k = 0;

    k = 0;
    fprintf(ctx->FileSaveData0, "%.3lf    ", ctx->SequentTime2);
    fprintf(ctx->FileSaveData1, "%.3lf    ", ctx->SequentTime2);
    for (i = 0; i < ctx->NumAD; i++)
    {
        for (j = 0; j < ctx->ad.Channels[i] / 2; j++)
        {
            fprintf(ctx->FileSaveData0, "%lf    ", ctx->Vout[k]);
            fprintf(ctx->FileSaveData1, "%lf    ", ctx->Phyout[k]);
            ++k;
        }
    }
    fprintf(ctx->FileSaveData0, "\n");
    fprintf(ctx->FileSaveData1, "\n");
    // Save Parameter Data
    fprintf(ctx->FileSaveData2, "%.3lf    ", ctx->SequentTime2);
    for (i = 0; i < 16; i++)
    {
        fprintf(ctx->FileSaveData2, "%lf    ", ctx->CalParam[i]);
    }
    fprintf(ctx->FileSaveData2, "\n");
    fflush(ctx->FileSaveData0);
    fflush(ctx->FileSaveData1);
    fflush(ctx->FileSaveData2);
}

void CDigitShowBasicDoc::SaveToFile2()
{
    DigitShowContext *ctx = GetContext();
    int i = 0, j = 0, k = 0;
    for (i = 0; i < ctx->sampling.CurrentSamplingTimes; i++)
    {
        k = 0;
        fprintf(ctx->FileSaveData0, "%.3lf    ", ctx->sampling.SavingClock / 1000000.0 * i);
        fprintf(ctx->FileSaveData1, "%.3lf    ", ctx->sampling.SavingClock / 1000000.0 * i);
        if (ctx->NumAD > 0)
        {
            for (j = 0; j < ctx->ad.Channels[0] / 2; j++)
            {
                ctx->Vtmp = BinaryToVolt(ctx->ad.RangeMax[0], ctx->ad.RangeMin[0], ctx->ad.Resolution[0],
                                         *((PLONG)ctx->pSmplData0 + i * ctx->ad.Channels[0] / 2 + j));
                ctx->Ptmp = ctx->cal.a[k] * ctx->Vtmp * ctx->Vtmp + ctx->cal.b[k] * ctx->Vtmp + ctx->cal.c[k];
                ++k;
                fprintf(ctx->FileSaveData0, "%lf    ", ctx->Vtmp);
                fprintf(ctx->FileSaveData1, "%lf    ", ctx->Ptmp);
            }
        }
        if (ctx->NumAD > 1)
        {
            for (j = 0; j < ctx->ad.Channels[1] / 2; j++)
            {
                ctx->Vtmp = BinaryToVolt(ctx->ad.RangeMax[1], ctx->ad.RangeMin[1], ctx->ad.Resolution[1],
                                         *((PLONG)ctx->pSmplData1 + i * ctx->ad.Channels[1] / 2 + j));
                ctx->Ptmp = ctx->cal.a[k] * ctx->Vtmp * ctx->Vtmp + ctx->cal.b[k] * ctx->Vtmp + ctx->cal.c[k];
                ++k;
                fprintf(ctx->FileSaveData0, "%lf    ", ctx->Vtmp);
                fprintf(ctx->FileSaveData1, "%lf    ", ctx->Ptmp);
            }
        }
        fprintf(ctx->FileSaveData0, "\n");
        fprintf(ctx->FileSaveData1, "\n");
    }
}

void CDigitShowBasicDoc::Allocate_Memory()
{
    DigitShowContext *ctx = GetContext();
    if (ctx->FlagSaveData == TRUE)
    {
        if (ctx->NumAD > 0)
        {
            ctx->hHeap0 = GetProcessHeap();
            ctx->pSmplData0 =
                HeapAlloc(ctx->hHeap0, HEAP_ZERO_MEMORY,
                          unsigned long(ctx->sampling.TotalSamplingTimes * ctx->ad.Channels[0] / 2 * sizeof(LONG)));
        }
        if (ctx->NumAD > 1)
        {
            ctx->hHeap1 = GetProcessHeap();
            ctx->pSmplData1 =
                HeapAlloc(ctx->hHeap1, HEAP_ZERO_MEMORY,
                          unsigned long(ctx->sampling.TotalSamplingTimes * ctx->ad.Channels[1] / 2 * sizeof(LONG)));
        }
    }
    if (ctx->FlagSaveData == FALSE)
    {
        if (ctx->NumAD > 0)
            HeapFree(ctx->hHeap0, 0, ctx->pSmplData0);
        if (ctx->NumAD > 1)
            HeapFree(ctx->hHeap1, 0, ctx->pSmplData1);
    }
}

//--- Control Statements ---
void CDigitShowBasicDoc::Control_DA()
{
    DigitShowContext *ctx = GetContext();
    switch (ctx->ControlID)
    {
    case 0: {
    }
    break;
    case 1: {
        ctx->runBeforeConsolidationControl();
        DA_OUTPUT();
    }
    break;
    case 2: {
        ctx->runConsolidationControl();
        DA_OUTPUT();
    }
    break;
    case 3: {
        ctx->runMonotonicLoadingCompressionExtensionControl();
        DA_OUTPUT();
    }
    break;
    case 4: {
        ctx->runMonotonicLoadingLoadingUnloadingControl();
        DA_OUTPUT();
    }
    break;
    case 5: {
                ctx->runCyclicLoadingControl();
        DA_OUTPUT();
    }
    break;
    case 6: {
        ctx->runDrainCyclicLoadingControl();
        DA_OUTPUT();
    }
    break;
    case 7: {
        ctx->runLinearEffectiveStressPathControl();
        DA_OUTPUT();
    }
    break;
    case 8: {
        DA_OUTPUT();
    }
    break;
    case 9: {
        DA_OUTPUT();
    }
    break;
    case 10: {
        DA_OUTPUT();
    }
    break;
    case 11: {
        DA_OUTPUT();
    }
    break;
    case 12: {
        DA_OUTPUT();
    }
    break;
    case 13: {
        DA_OUTPUT();
    }
    break;
    case 14: {
        DA_OUTPUT();
    }
    break;
    case 15: {
        if (ctx->controlFile.CurrentNum >= 0 && ctx->controlFile.CurrentNum < 128)
        {
            switch (ctx->controlFile.Num[ctx->controlFile.CurrentNum])
            {
                case 0:
                ctx->setMotorBrake(true);
                ctx->setMotorSpeed(0.0);
                break;
            case 1:
                ctx->runScriptedMonotonicLoadingStress();
                break;
            case 2:
                ctx->runScriptedMonotonicLoadingStrain();
                break;
            case 3:
                ctx->runScriptedCyclicLoadingStress();
                break;
            case 4:
                ctx->runScriptedCyclicLoadingStrain();
                break;
            case 5:
                ctx->runScriptedCreep();
                break;
            case 6:
                ctx->runScriptedLinearEffectiveStressPath();
                break;
            case 7:
                ctx->runScriptedCreep2();
                break;
            }
            DA_OUTPUT();
        }
    }
    break;
    }
}
void CDigitShowBasicDoc::Start_Control()
{
}

void CDigitShowBasicDoc::Stop_Control()
{
    DigitShowContext *ctx = GetContext();
    ctx->setMotorSpeed(0.0);
    // Motor Speed->0
    DA_OUTPUT();
}
