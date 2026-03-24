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

#include "DigitShowContext.h"

namespace
{
float ToDaVoltage(const DigitShowContext &ctx, int channel, double value)
{
    return float(ctx.cal.DA_a[channel] * value + ctx.cal.DA_b[channel]);
}

void RunSimpleCyclicControl(DigitShowContext &ctx, ControlData &cd)
{
    ctx.setMotorSpeed(cd.MotorSpeed);
    ctx.setMotorBrake(false);

    if (cd.flag[0] == FALSE)
    {
        if (cd.time[0] < cd.time[1])
        {
            ctx.setMotorCruchUp(false);
            if (ctx.phys.q >= cd.sigma[1])
            {
                cd.time[0] = cd.time[1];
                ctx.FlagCyclic = FALSE;
            }
        }
        if (cd.time[1] <= cd.time[0] || cd.time[0] <= cd.time[2])
        {
            if (ctx.FlagCyclic == FALSE)
            {
                ctx.setMotorCruchUp(true);
                if (ctx.phys.q <= cd.sigma[0])
                    ctx.FlagCyclic = TRUE;
            }
            if (ctx.FlagCyclic == TRUE)
            {
                ctx.setMotorCruchUp(false);
                if (ctx.phys.q >= cd.sigma[1])
                {
                    ctx.FlagCyclic = FALSE;
                    ++cd.time[0];
                }
            }
        }
        if (cd.time[0] > cd.time[2])
        {
            ctx.setMotorCruchUp(false);
        }
    }
    if (cd.flag[0] == TRUE)
    {
        if (cd.time[0] < cd.time[1])
        {
            ctx.setMotorCruchUp(true);
            if (ctx.phys.q <= cd.sigma[0])
            {
                cd.time[0] = cd.time[1];
                ctx.FlagCyclic = TRUE;
            }
        }
        if (cd.time[1] <= cd.time[0] || cd.time[0] <= cd.time[2])
        {
            if (ctx.FlagCyclic == TRUE)
            {
                ctx.setMotorCruchUp(false);
                if (ctx.phys.q >= cd.sigma[1])
                    ctx.FlagCyclic = FALSE;
            }
            if (ctx.FlagCyclic == FALSE)
            {
                ctx.setMotorCruchUp(true);
                if (ctx.phys.q <= cd.sigma[0])
                {
                    ctx.FlagCyclic = TRUE;
                    ++cd.time[0];
                }
            }
        }
        if (cd.time[0] > cd.time[2])
        {
            ctx.setMotorCruchUp(true);
        }
    }
}
} // namespace

// Singleton instance
static DigitShowContext g_Context;
static bool g_ContextInitialized = false;

DigitShowContext *GetContext()
{
    if (!g_ContextInitialized)
    {
        InitContext(&g_Context);
        g_ContextInitialized = true;
    }
    return &g_Context;
}

void InitContext(DigitShowContext *ctx)
{
    if (ctx == nullptr)
        return;

    // Initialize board counts
    ctx->NumAD = 1;
    ctx->NumDA = 1;
    ctx->AdMaxChannels = 0;

    // Initialize A/D board config
    memset(&ctx->ad, 0, sizeof(ctx->ad));

    // Initialize D/A board config
    memset(&ctx->da, 0, sizeof(ctx->da));

    // Initialize D/A channel assignments
    ctx->daChannel.Motor = 0;
    ctx->daChannel.MotorCruch = 1;
    ctx->daChannel.MotorSpeed = 2;
    ctx->daChannel.EP_Cell = 3;

    // Initialize sampling settings
    ctx->sampling.SavingClock = 0.0f;
    ctx->sampling.SavingTime = 300;
    ctx->sampling.TotalSamplingTimes = 0;
    ctx->sampling.CurrentSamplingTimes = 0;
    ctx->sampling.AllocatedMemory = 0.0f;
    ctx->sampling.AvSmplNum = 20;

    // Initialize flags
    ctx->FlagSetBoard = false;
    ctx->FlagSaveData = false;
    ctx->FlagFIFO = false;
    ctx->FlagCyclic = false;
    ctx->FlagCtrl = false;

    // Initialize control state
    ctx->ControlID = 0;
    ctx->NumCyclic = 0;
    ctx->TotalStepTime = 0.0;
    ctx->cyclicState.DaFlag = 0.0;
    ctx->cyclicState.RuIndicator = 0.0;
    ctx->cyclicState.MinAxialStrain = 0.0;
    ctx->cyclicState.MaxAxialStrain = 0.0;
    ctx->AmpID = 0;

    // Initialize time values
    ctx->SequentTime1 = 0;
    ctx->SequentTime2 = 0.0;
    ctx->CtrlStepTime = 0.0;

    // Initialize time intervals (ms)
    ctx->timeSettings.Interval1 = 50;
    ctx->timeSettings.Interval2 = 500;
    ctx->timeSettings.Interval3 = 1000;

    // Initialize physical values
    ctx->phys.sa = 0.0;
    ctx->phys.e_sa = 0.0;
    ctx->phys.sr = 0.0;
    ctx->phys.e_sr = 0.0;
    ctx->phys.p = 0.0;
    ctx->phys.e_p = 0.0;
    ctx->phys.q = 0.0;
    ctx->phys.u = 0.0;
    ctx->phys.ea = 0.0;
    ctx->phys.er = 0.0;
    ctx->phys.ev = 0.0;
    ctx->phys.eLDT = 0.0;
    ctx->phys.eLDT1 = 0.0;
    ctx->phys.eLDT2 = 0.0;
    ctx->height = 0.0;
    ctx->volume = 0.0;
    ctx->area = 0.0;
    ctx->Vtmp = 0.0f;
    ctx->Ptmp = 0.0;

    // Initialize memory pointers
    ctx->pSmplData0 = nullptr;
    ctx->pSmplData1 = nullptr;
    ctx->hHeap0 = nullptr;
    ctx->hHeap1 = nullptr;

    // Initialize file handles
    ctx->FileSaveData0 = nullptr;
    ctx->FileSaveData1 = nullptr;
    ctx->FileSaveData2 = nullptr;

    // Initialize error handling
    ctx->Ret = 0;
    ctx->Ret2 = 0;
    ctx->AdEvent = 0;
    memset(ctx->ErrorString, 0, sizeof(ctx->ErrorString));
    // Note: CString TextString, CTime, CTimeSpan are default-constructed by C++ runtime

    // Initialize calibration factors (default: linear y = x)
    for (int i = 0; i < 64; i++)
    {
        ctx->Vout[i] = 0.0f;
        ctx->Phyout[i] = 0.0;
        ctx->CalParam[i] = 0.0;
        ctx->cal.a[i] = 0.0;
        ctx->cal.b[i] = 1.0;
        ctx->cal.c[i] = 0.0;
    }

    // Initialize D/A output
    for (int i = 0; i < 8; i++)
    {
        ctx->DAVout[i] = 0.0f;
        ctx->cal.DA_a[i] = 0.0;
        ctx->cal.DA_b[i] = 0.0;
    }

    // Initialize specimen data
    for (int j = 0; j < 4; j++)
    {
        ctx->specimen.Diameter[j] = 50.0;
        ctx->specimen.Width[j] = 0.0;
        ctx->specimen.Depth[j] = 0.0;
        ctx->specimen.Height[j] = 150.0;
        ctx->specimen.Area[j] = 3848.451;
        ctx->specimen.Volume[j] = 577267.7;
        ctx->specimen.Weight[j] = 0.0;
        ctx->specimen.VLDT1[j] = 120;
        ctx->specimen.VLDT2[j] = 120;
    }
    ctx->specimen.Gs = 0.0;
    ctx->specimen.MembraneModulus = 0.0;
    ctx->specimen.MembraneThickness = 0.0;
    ctx->specimen.RodArea = 0.0;
    ctx->specimen.RodWeight = 0.0;

    // Initialize control data
    for (int i = 0; i < 16; i++)
    {
        ctx->control[i].p = 0.0;
        ctx->control[i].q = 0.0;
        ctx->control[i].u = 0.0;
        for (int j = 0; j < 3; j++)
        {
            ctx->control[i].flag[j] = false;
            ctx->control[i].time[j] = 0;
            ctx->control[i].sigma[j] = 0.0;
            ctx->control[i].sigmaRate[j] = 0.0;
            ctx->control[i].sigmaAmp[j] = 0.0;
            ctx->control[i].e_sigma[j] = 0.0;
            ctx->control[i].e_sigmaRate[j] = 0.0;
            ctx->control[i].e_sigmaAmp[j] = 0.0;
            ctx->control[i].strain[j] = 0.0;
            ctx->control[i].strainRate[j] = 0.0;
            ctx->control[i].strainAmp[j] = 0.0;
        }
        ctx->control[i].K0 = 1.0;
        ctx->control[i].MotorSpeed = 0.0;
        ctx->control[i].Motor = 0;
        ctx->control[i].MotorCruch = 0;
    }

    // Initialize control file data
    ctx->controlFile.CurrentNum = 0;
    for (int i = 0; i < 128; i++)
    {
        ctx->controlFile.Num[i] = 0;
        for (int j = 0; j < 10; j++)
        {
            ctx->controlFile.Para[i][j] = 0.0;
        }
    }

    // Pre-consolidation control defaults
    ctx->control[1].MotorSpeed = 100.0;
    ctx->control[1].q = 1.0;

    // Error tolerance defaults
    ctx->errTol.StressCom = 1.0;
    ctx->errTol.StressExt = -1.0;
    ctx->errTol.StressA = 0.3;

    // D/A channel assignments
    ctx->daChannel.Motor = 0;
    ctx->daChannel.MotorCruch = 1;
    ctx->daChannel.MotorSpeed = 2;
    ctx->daChannel.EP_Cell = 3;

    // D/A calibration for motor speed (V/rpm)
    ctx->cal.DA_a[ctx->daChannel.MotorSpeed] = 0.00262055;
    ctx->cal.DA_b[ctx->daChannel.MotorSpeed] = 0.0;

    // D/A calibration for cell pressure (V/kPa)
    ctx->cal.DA_a[ctx->daChannel.EP_Cell] = 0.012587;
    ctx->cal.DA_b[ctx->daChannel.EP_Cell] = 0.0;
}

void DigitShowContext::setMotorBrake(bool braking)
{
    DAVout[daChannel.Motor] = braking ? 0.0f : 5.0f;
}

void DigitShowContext::setMotorCruchUp(bool up)
{
    DAVout[daChannel.MotorCruch] = up ? 5.0f : 0.0f;
}

void DigitShowContext::setMotorSpeed(double rpm)
{
    DAVout[daChannel.MotorSpeed] = ToDaVoltage(*this, daChannel.MotorSpeed, rpm);
}

void DigitShowContext::adjustEpCell(double delta)
{
    DAVout[daChannel.EP_Cell] += float(delta);
}

void DigitShowContext::runBeforeConsolidationControl()
{
    //---Before Consolidation: Keep the specimen isotropic condition by Motor Control.---
    // ctx->control[1].q: Reference Error Stress (kPa).
    // ctx->control[1].MotorSpeed: The Maximum Motor Speed (rpm).
    setMotorBrake(false);
    if (phys.q > errTol.StressCom)
    {
        setMotorCruchUp(true);
        if (phys.q > control[1].q)
        {
            setMotorSpeed(control[1].MotorSpeed);
        }
        if (phys.q <= control[1].q)
        {
            setMotorSpeed((phys.q / control[1].q) * control[1].MotorSpeed);
        }
    }
    else if (phys.q < errTol.StressExt)
    {
        setMotorCruchUp(false);
        if (phys.q < -control[1].q)
        {
            setMotorSpeed(control[1].MotorSpeed);
        }
        if (phys.q >= -control[1].q)
        {
            setMotorSpeed((-phys.q / control[1].q) * control[1].MotorSpeed);
        }
    }
    else
    {
        setMotorSpeed(0.0);
    }
}

void DigitShowContext::runConsolidationControl()
{
    // Consolidation (Motor Control):
    // ctx->control[2].e_sigma[0]:	Target Axial Effectve Stress,
    // ctx->control[2].K0:			K0 value,
    // ctx->control[2].sigmaRate[2]:	Increase Rate of Cell Pressure
    // ctx->control[2].MotorSpeed:	Motor Speed
    setMotorBrake(false);
    setMotorSpeed(control[2].MotorSpeed);

    if (phys.e_sr < control[2].e_sigma[0] * control[2].K0 - errTol.StressA)
    {
        adjustEpCell(cal.DA_a[daChannel.EP_Cell] * control[2].sigmaRate[2] / 60.0 *
                     static_cast<double>(timeSettings.Interval2) / 1000.0);
    }
    if (phys.e_sr > control[2].e_sigma[0] * control[2].K0 + errTol.StressA)
    {
        adjustEpCell(-cal.DA_a[daChannel.EP_Cell] * control[2].sigmaRate[2] / 60.0 *
                     static_cast<double>(timeSettings.Interval2) / 1000.0);
    }
    if (phys.e_sa < phys.e_sr / control[2].K0 + errTol.StressExt)
    {
        setMotorCruchUp(false);
    }
    else if (phys.e_sa > phys.e_sr / control[2].K0 + errTol.StressCom)
    {
        setMotorCruchUp(true);
    }
    else
    {
        setMotorSpeed(0.0);
    }
}

void DigitShowContext::runMonotonicLoadingCompressionExtensionControl()
{
    // Monotonic Loading (Motor Control)
    // ctx->control[3].MotorSpeed:	Motor Speed
    // ctx->control[3].MotorCruch:	Compression:1 /Extension:0
    // ctx->control[3].flag[0]:		Monotonic_Loading:0 /Creep:1
    // ctx->control[3].sigma[0];		Limiter
    setMotorBrake(false);
    setMotorSpeed(control[3].MotorSpeed);

    if (control[3].flag[0] == FALSE)
    {
        if (control[3].MotorCruch == 0)
        {
            setMotorCruchUp(false);
            if (phys.q >= control[3].q)
                control[3].flag[0] = TRUE;
        }
        if (control[3].MotorCruch == 1)
        {
            setMotorCruchUp(true);
            if (phys.q <= control[3].q)
                control[3].flag[0] = TRUE;
        }
    }
    if (control[3].flag[0] == TRUE)
    {
        if (control[3].MotorCruch == 0)
        {
            setMotorCruchUp(false);
            if (phys.q >= control[3].q + errTol.StressExt)
                setMotorSpeed(0.0);
        }
        if (control[3].MotorCruch == 1)
        {
            setMotorCruchUp(true);
            if (phys.q <= control[3].q + errTol.StressCom)
                setMotorSpeed(0.0);
        }
    }
}

void DigitShowContext::runMonotonicLoadingLoadingUnloadingControl()
{
    // Monotonic Loading (Motor Control)
    // ctx->control[4].MotorSpeed:	Motor Speed
    // ctx->control[4].MotorCruch:	Cruch Loading:1 /Unloading:0
    // ctx->control[4].flag:			Loading:0 /Creep:1
    // ctx->control[4].sigma[0];		Limiter
    setMotorSpeed(control[4].MotorSpeed);
    setMotorBrake(false);

    if (control[4].flag[0] == FALSE)
    {
        if (control[4].MotorCruch == 0)
        {
            setMotorCruchUp(false);
            if (phys.q >= control[4].q)
                control[4].flag[0] = TRUE;
        }
        if (control[4].MotorCruch == 1)
        {
            setMotorCruchUp(true);
            if (phys.q <= control[4].q)
                control[4].flag[0] = TRUE;
        }
    }
    if (control[4].flag[0] == TRUE)
    {
        if (control[4].MotorCruch == 0)
        {
            setMotorCruchUp(false);
            if (phys.q >= control[4].q + errTol.StressExt)
                setMotorSpeed(0.0);
        }
        if (control[4].MotorCruch == 1)
        {
            setMotorCruchUp(true);
            if (phys.q <= control[4].q + errTol.StressCom)
                setMotorSpeed(0.0);
        }
    }
}

void DigitShowContext::runCyclicLoadingControl()
{
    RunSimpleCyclicControl(*this, control[5]);
}

void DigitShowContext::runDrainCyclicLoadingControl()
{
    RunSimpleCyclicControl(*this, control[6]);
}

void DigitShowContext::runLinearEffectiveStressPathControl()
{
    setMotorBrake(false);
    setMotorSpeed(control[7].MotorSpeed);

    if (control[7].sigma[1] == control[7].e_sigma[1])
    {
        adjustEpCell(0.2 * cal.DA_a[daChannel.EP_Cell] * (control[7].e_sigma[1] - phys.e_sr));
        if (phys.e_sa > control[7].e_sigma[0] + errTol.StressCom)
            setMotorCruchUp(true);
        else if (phys.e_sa < control[7].e_sigma[0] + errTol.StressExt)
            setMotorCruchUp(false);
        else
            setMotorSpeed(0.0);
    }
    if (control[7].sigma[1] < control[7].e_sigma[1])
    {
        if (phys.e_sr >= control[7].e_sigma[1])
        {
            adjustEpCell(-0.2 * cal.DA_a[daChannel.EP_Cell] * (phys.e_sr - control[7].e_sigma[1]));
        }
        if (phys.e_sr < control[7].e_sigma[1])
        {
            adjustEpCell(cal.DA_a[daChannel.EP_Cell] * fabs(control[7].sigmaRate[0]) / 60.0 *
                         static_cast<double>(timeSettings.Interval2) / 1000.0);
        }
        if (phys.e_sa > (control[7].e_sigma[0] - control[7].sigma[0]) / (control[7].e_sigma[1] - control[7].sigma[1]) *
                                (phys.e_sr - control[7].sigma[1]) +
                            control[7].sigma[0] + errTol.StressCom)
        {
            setMotorCruchUp(true);
        }
        else if (phys.e_sa < (control[7].e_sigma[0] - control[7].sigma[0]) /
                                     (control[7].e_sigma[1] - control[7].sigma[1]) * (phys.e_sr - control[7].sigma[1]) +
                                 control[7].sigma[0] + errTol.StressExt)
        {
            setMotorCruchUp(false);
        }
        else
        {
            setMotorSpeed(0.0);
        }
    }
    if (control[7].sigma[1] > control[7].e_sigma[1])
    {
        if (phys.e_sr > control[7].e_sigma[1])
        {
            adjustEpCell(-cal.DA_a[daChannel.EP_Cell] * fabs(control[7].sigmaRate[0]) / 60.0 *
                         static_cast<double>(timeSettings.Interval2) / 1000.0);
        }
        if (phys.e_sr <= control[7].e_sigma[1])
        {
            adjustEpCell(0.2 * cal.DA_a[daChannel.EP_Cell] * (control[7].e_sigma[1] - phys.e_sr));
        }
        if (phys.e_sa > (control[7].e_sigma[0] - control[7].sigma[0]) / (control[7].e_sigma[1] - control[7].sigma[1]) *
                                (phys.e_sr - control[7].sigma[1]) +
                            control[7].sigma[0] + errTol.StressCom)
        {
            setMotorCruchUp(true);
        }
        else if (phys.e_sa < (control[7].e_sigma[0] - control[7].sigma[0]) /
                                     (control[7].e_sigma[1] - control[7].sigma[1]) * (phys.e_sr - control[7].sigma[1]) +
                                 control[7].sigma[0] + errTol.StressExt)
        {
            setMotorCruchUp(false);
        }
        else
        {
            setMotorSpeed(0.0);
        }
    }
}

void DigitShowContext::runScriptedMonotonicLoadingStress()
{
    TotalStepTime += CtrlStepTime / 60.0;
    setMotorBrake(false);
    setMotorSpeed(controlFile.Para[controlFile.CurrentNum][1]);
    // 2020.7　側圧一定
    if (phys.sr >= controlFile.Para[controlFile.CurrentNum][3] + errTol.StressA)
    {
        adjustEpCell(-0.1 * cal.DA_a[daChannel.EP_Cell] * (phys.sr - controlFile.Para[controlFile.CurrentNum][3]));
    }
    if (phys.sr <= controlFile.Para[controlFile.CurrentNum][3] - errTol.StressA)
    {
        adjustEpCell(0.1 * cal.DA_a[daChannel.EP_Cell] * (controlFile.Para[controlFile.CurrentNum][3] - phys.sr));
    }
    if (controlFile.Para[controlFile.CurrentNum][0] == 0.0)
    {
        if (phys.q <= controlFile.Para[controlFile.CurrentNum][2])
        {
            setMotorCruchUp(false);
        }
        else
        {
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
        }
    }
    else if (controlFile.Para[controlFile.CurrentNum][0] == 1.0)
    {
        if (phys.q >= controlFile.Para[controlFile.CurrentNum][2])
        {
            setMotorCruchUp(true);
        }
        else
        {
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
        }
    }
}

void DigitShowContext::runScriptedMonotonicLoadingStrain()
{
    TotalStepTime += CtrlStepTime / 60.0;
    setMotorBrake(false);
    setMotorSpeed(controlFile.Para[controlFile.CurrentNum][1]);

    // 2020.7　側圧一定
    if (phys.sr >= controlFile.Para[controlFile.CurrentNum][5] + errTol.StressA)
    {
        adjustEpCell(-0.1 * cal.DA_a[daChannel.EP_Cell] * (phys.sr - controlFile.Para[controlFile.CurrentNum][5]));
    }
    if (phys.sr <= controlFile.Para[controlFile.CurrentNum][5] - errTol.StressA)
    {
        adjustEpCell(0.1 * cal.DA_a[daChannel.EP_Cell] * (controlFile.Para[controlFile.CurrentNum][5] - phys.sr));
    }
    if (controlFile.Para[controlFile.CurrentNum][0] == 0.0)
    {
        if (phys.ea <= controlFile.Para[controlFile.CurrentNum][2])
        {
            setMotorCruchUp(false);
        }
        if (phys.ea > controlFile.Para[controlFile.CurrentNum][2])
        { // 2021.3
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
        }
        else if (TotalStepTime >= controlFile.Para[controlFile.CurrentNum][4])
        { // 2021.3
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
        }
        else if (Phyout[0] > controlFile.Para[controlFile.CurrentNum][3])
        { // 2021.3
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
        }
    }
    else if (controlFile.Para[controlFile.CurrentNum][0] == 1.0)
    {
        if (phys.ea >= controlFile.Para[controlFile.CurrentNum][2])
        {
            setMotorCruchUp(true);
        }
        else
        {
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
        }
    }
}

void DigitShowContext::runScriptedCyclicLoadingStress()
{
    TotalStepTime += CtrlStepTime / 60.0;
    setMotorBrake(false);
    setMotorSpeed(controlFile.Para[controlFile.CurrentNum][1]);
    // 2020.7　側圧一定
    if (phys.sr >= controlFile.Para[controlFile.CurrentNum][6] + errTol.StressA)
    {
        adjustEpCell(-0.1 * cal.DA_a[daChannel.EP_Cell] * (phys.sr - controlFile.Para[controlFile.CurrentNum][6]));
    }
    if (phys.sr <= controlFile.Para[controlFile.CurrentNum][6] - errTol.StressA)
    {
        adjustEpCell(0.1 * cal.DA_a[daChannel.EP_Cell] * (controlFile.Para[controlFile.CurrentNum][6] - phys.sr));
    }
    if (controlFile.Para[controlFile.CurrentNum][0] == 0.0)
    {
        if (NumCyclic == 0)
        {
            FlagCyclic = FALSE;
            NumCyclic = 1;
        }
        if (NumCyclic != 0 && NumCyclic <= controlFile.Para[controlFile.CurrentNum][4])
        {
            if (FlagCyclic == FALSE)
            {
                setMotorCruchUp(true);
                // 2020.4 最終的に軸ひずみの最小値（マイナスの値）がcyclicState.MinAxialStrainに割り当てられる
                cyclicState.MinAxialStrain = phys.ea;
                if (phys.u >= controlFile.Para[controlFile.CurrentNum][7])
                {                                   // 2021.3
                    cyclicState.RuIndicator = 0.95; // 2021.3
                }
                if (phys.q <= controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) < controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                    FlagCyclic = TRUE; // 2021.3
                if (phys.q <= controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) < controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                { // 2021.3
                    FlagCyclic = TRUE;
                }
                if (phys.q <= controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) >= controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                { // 2021.3
                    FlagCyclic = TRUE;
                }
                if (phys.q <= controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) >= controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                { // 2021.3
                    FlagCyclic = TRUE;
                    controlFile.Para[controlFile.CurrentNum][4] = NumCyclic;
                }
                if (phys.q > controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) >= 2.4 * controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                { // 2021.3
                    FlagCyclic = TRUE;
                }
                if (phys.q > controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) >= 2.4 * controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                { // 2021.3
                    FlagCyclic = TRUE;
                    controlFile.Para[controlFile.CurrentNum][4] = NumCyclic;
                }
            }
            if (FlagCyclic == TRUE)
            {
                setMotorCruchUp(false);

                // 2020.4 最終的に軸ひずみの最小値（マイナスの値）がcyclicState.MinAxialStrainに割り当てられる
                cyclicState.MaxAxialStrain = phys.ea;
                if (phys.u >= controlFile.Para[controlFile.CurrentNum][7])
                {                                   // 2021.3
                    cyclicState.RuIndicator = 0.95; // 2021.3
                }
                if (phys.q >= controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) < controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                    ++NumCyclic;
                }
                // 2021.3 現在の軸ひずみとの差分がDAを超えるか否か。
                if (phys.q >= controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) < controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                {
                    FlagCyclic = FALSE;
                    ++NumCyclic;
                }
                if (phys.q >= controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) >= controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                    ++NumCyclic;
                }
                if (phys.q >= controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) >= controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                    --NumCyclic;
                }
                if (phys.q < controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) >= 2.4 * controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                    ++NumCyclic;
                }
                if (phys.q < controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) >= 2.4 * controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                    --NumCyclic;
                }
            }
        }
        if (NumCyclic > controlFile.Para[controlFile.CurrentNum][4])
        {
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
            NumCyclic = 0;
        }
    }
    else if (controlFile.Para[controlFile.CurrentNum][0] == 1.0)
    {
        if (NumCyclic == 0)
        {
            FlagCyclic = TRUE;
            NumCyclic = 1;
        }
        if (NumCyclic != 0 && NumCyclic <= controlFile.Para[controlFile.CurrentNum][4])
        {
            if (FlagCyclic == FALSE)
            {
                setMotorCruchUp(true);
                cyclicState.MinAxialStrain = phys.ea;
                if (phys.u >= controlFile.Para[controlFile.CurrentNum][7])
                {                                   // 2021.3
                    cyclicState.RuIndicator = 0.95; // 2021.3
                } // 2021.3
                if ((cyclicState.MaxAxialStrain - phys.ea) >= controlFile.Para[controlFile.CurrentNum][5] ||
                    (phys.ea - cyclicState.MinAxialStrain) >= controlFile.Para[controlFile.CurrentNum][5])
                {                             // 2021.6
                    cyclicState.DaFlag = 1.0; // 2021.6
                }
                if (phys.q <= controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) < controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                {                      // 2021.3
                    FlagCyclic = TRUE; // 2021.3
                    ++NumCyclic;       // 2021.3
                } // 2021.3
                if (phys.q <= controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) < controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                {                      // 2021.3
                    FlagCyclic = TRUE; // 2021.3
                    ++NumCyclic;       // 2021.3
                } // 2021.3
                if (phys.q <= controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) >= controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                {                      // 2021.3
                    FlagCyclic = TRUE; // 2021.3
                    ++NumCyclic;       // 2021.3
                } // 2021.3
                if (phys.q <= controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) >= controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                {                      // 2021.3
                    FlagCyclic = TRUE; // 2021.3
                                       // 2021.3 問題なく機能していることを確認
                    controlFile.Para[controlFile.CurrentNum][4] = NumCyclic - 1;
                } // 2021.3
                if (phys.q > controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) >= 2.4 * controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                {                      // 2021.3
                    FlagCyclic = TRUE; // 2021.3
                    ++NumCyclic;       // 2021.3
                } // 2021.3
                if (phys.q > controlFile.Para[controlFile.CurrentNum][2] &&
                    (cyclicState.MaxAxialStrain - phys.ea) >= 2.4 * controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                {                                                                // 2021.3
                    FlagCyclic = TRUE;                                           // 2021.3
                    controlFile.Para[controlFile.CurrentNum][4] = NumCyclic - 1; // 2021.3
                }
            }
            if (FlagCyclic == TRUE)
            {
                setMotorCruchUp(false);
                cyclicState.MaxAxialStrain = phys.ea;
                if (phys.u >= controlFile.Para[controlFile.CurrentNum][7])
                {                                   // 2021.3
                    cyclicState.RuIndicator = 0.95; // 2021.3
                } // 2021.3
                if ((cyclicState.MaxAxialStrain - phys.ea) >= controlFile.Para[controlFile.CurrentNum][5] ||
                    (phys.ea - cyclicState.MinAxialStrain) >= controlFile.Para[controlFile.CurrentNum][5])
                {                             // 2021.6
                    cyclicState.DaFlag = 1.0; // 2021.6
                }
                if (phys.q >= controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) < controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                {                       // 2021.3
                    FlagCyclic = FALSE; // 2021.3
                } // 2021.3
                if (phys.q >= controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) < controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                }
                if (phys.q >= controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) >= controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                }
                if (phys.q >= controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) >= controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                    controlFile.Para[controlFile.CurrentNum][4] = NumCyclic;
                }
                if (phys.q < controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) >= 2.4 * controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator < 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                }
                if (phys.q < controlFile.Para[controlFile.CurrentNum][3] &&
                    (phys.ea - cyclicState.MinAxialStrain) >= 2.4 * controlFile.Para[controlFile.CurrentNum][5] &&
                    cyclicState.RuIndicator == 0.95)
                { // 2021.3
                    FlagCyclic = FALSE;
                    controlFile.Para[controlFile.CurrentNum][4] = NumCyclic;
                }
                if (phys.q <= 1.0 && cyclicState.DaFlag == 1.0 && cyclicState.RuIndicator == 0.95)
                { // 2021.6
                    ++controlFile.CurrentNum;
                    TotalStepTime = 0.0;
                    NumCyclic = 0;
                }
            }
        }
        if (NumCyclic > controlFile.Para[controlFile.CurrentNum][4])
        {
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
            NumCyclic = 0;
        }
    }
}

void DigitShowContext::runScriptedCyclicLoadingStrain()
{
    TotalStepTime += CtrlStepTime / 60.0;
    setMotorBrake(false);
    setMotorSpeed(controlFile.Para[controlFile.CurrentNum][1]);
    // 2020.7　側圧一定
    if (phys.sr >= controlFile.Para[controlFile.CurrentNum][5] + errTol.StressA)
    {
        adjustEpCell(-0.1 * cal.DA_a[daChannel.EP_Cell] * (phys.sr - controlFile.Para[controlFile.CurrentNum][5]));
    }
    if (phys.sr <= controlFile.Para[controlFile.CurrentNum][5] - errTol.StressA)
    {
        adjustEpCell(0.1 * cal.DA_a[daChannel.EP_Cell] * (controlFile.Para[controlFile.CurrentNum][5] - phys.sr));
    }
    if (controlFile.Para[controlFile.CurrentNum][0] == 0.0)
    {
        if (NumCyclic == 0)
        {
            FlagCyclic = FALSE;
            NumCyclic = 1;
        }
        if (NumCyclic != 0 && NumCyclic <= controlFile.Para[controlFile.CurrentNum][4])
        {
            if (FlagCyclic == FALSE)
            {
                setMotorCruchUp(true);
                if (phys.ea <= controlFile.Para[controlFile.CurrentNum][2])
                    FlagCyclic = TRUE;
            }
            if (FlagCyclic == TRUE)
            {
                setMotorCruchUp(false);
                if (phys.ea >= controlFile.Para[controlFile.CurrentNum][3])
                {
                    FlagCyclic = FALSE;
                    ++NumCyclic;
                }
            }
        }
        if (NumCyclic > controlFile.Para[controlFile.CurrentNum][4])
        {
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
            NumCyclic = 0;
        }
    }
    else if (controlFile.Para[controlFile.CurrentNum][0] == 1.0)
    {
        if (NumCyclic == 0)
        {
            FlagCyclic = TRUE;
            NumCyclic = 1;
        }
        if (NumCyclic != 0 && NumCyclic <= controlFile.Para[controlFile.CurrentNum][4])
        {
            if (FlagCyclic == FALSE)
            {
                setMotorCruchUp(true);
                if (phys.ea <= controlFile.Para[controlFile.CurrentNum][2])
                {
                    FlagCyclic = TRUE;
                    ++NumCyclic;
                }
            }
            if (FlagCyclic == TRUE)
            {
                setMotorCruchUp(false);
                if (phys.ea >= controlFile.Para[controlFile.CurrentNum][3])
                    FlagCyclic = FALSE;
            }
        }
        if (NumCyclic > controlFile.Para[controlFile.CurrentNum][4])
        {
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
            NumCyclic = 0;
        }
    }
}

void DigitShowContext::runScriptedCreep()
{
    TotalStepTime += CtrlStepTime / 60.0;
    setMotorBrake(false);
    setMotorSpeed(controlFile.Para[controlFile.CurrentNum][0]);
    // 2020.7　側圧一定
    if (phys.sr >= controlFile.Para[controlFile.CurrentNum][3] + errTol.StressA)
    {
        adjustEpCell(-0.1 * cal.DA_a[daChannel.EP_Cell] * (phys.sr - controlFile.Para[controlFile.CurrentNum][3]));
    }
    if (phys.sr <= controlFile.Para[controlFile.CurrentNum][3] - errTol.StressA)
    {
        adjustEpCell(0.1 * cal.DA_a[daChannel.EP_Cell] * (controlFile.Para[controlFile.CurrentNum][3] - phys.sr));
    }
    if (phys.q >= controlFile.Para[controlFile.CurrentNum][1] + errTol.StressCom)
    {
        setMotorCruchUp(true);
    }
    else if (phys.q <= controlFile.Para[controlFile.CurrentNum][1] + errTol.StressExt)
    {
        setMotorCruchUp(false);
        // 2020.5 エラーストレスの大きさに応じたRPMの自動変更
        setMotorSpeed(controlFile.Para[controlFile.CurrentNum][0] *
                      (phys.q - controlFile.Para[controlFile.CurrentNum][1]) / errTol.StressExt / 2);
    }
    else
    {
        setMotorSpeed(0.0); // RPM->0
    }
    if (TotalStepTime >= controlFile.Para[controlFile.CurrentNum][2])
    {
        ++controlFile.CurrentNum;
        TotalStepTime = 0.0;
    }
}

// 2020.4　有効応力から全応力へ変更（B.P.を自動で上げられるように）
void DigitShowContext::runScriptedLinearEffectiveStressPath()
{
    TotalStepTime += CtrlStepTime / 60.0;
    setMotorBrake(false);
    setMotorSpeed(controlFile.Para[controlFile.CurrentNum][4]);
    if (controlFile.Para[controlFile.CurrentNum][1] == controlFile.Para[controlFile.CurrentNum][3])
    {
        adjustEpCell(0.2 * cal.DA_a[daChannel.EP_Cell] * (controlFile.Para[controlFile.CurrentNum][3] - phys.sr));
        if (phys.sa > controlFile.Para[controlFile.CurrentNum][2] + errTol.StressCom)
        {
            setMotorCruchUp(true);
        }
        else if (phys.sa < controlFile.Para[controlFile.CurrentNum][2] + errTol.StressExt)
        {
            setMotorCruchUp(false);
        }
        else
        {
            ++controlFile.CurrentNum;
            TotalStepTime = 0.0;
        }
    }
    else if (controlFile.Para[controlFile.CurrentNum][1] < controlFile.Para[controlFile.CurrentNum][3])
    {
        if (phys.sr >= controlFile.Para[controlFile.CurrentNum][3] - errTol.StressA)
        {
            adjustEpCell(-0.2 * cal.DA_a[daChannel.EP_Cell] * (phys.sr - controlFile.Para[controlFile.CurrentNum][3]));
        }
        if (phys.sr < controlFile.Para[controlFile.CurrentNum][3] - errTol.StressA)
        {
            adjustEpCell(cal.DA_a[daChannel.EP_Cell] * fabs(controlFile.Para[controlFile.CurrentNum][5]) / 60.0 *
                         static_cast<double>(timeSettings.Interval2) / 1000.0);
        }
        if (phys.sa >
            (controlFile.Para[controlFile.CurrentNum][2] - controlFile.Para[controlFile.CurrentNum][0]) /
                    (controlFile.Para[controlFile.CurrentNum][3] - controlFile.Para[controlFile.CurrentNum][1]) *
                    (phys.sr - controlFile.Para[controlFile.CurrentNum][1]) +
                controlFile.Para[controlFile.CurrentNum][0] + errTol.StressCom)
        {
            setMotorCruchUp(true);
        }
        else if (phys.sa <
                 (controlFile.Para[controlFile.CurrentNum][2] - controlFile.Para[controlFile.CurrentNum][0]) /
                         (controlFile.Para[controlFile.CurrentNum][3] - controlFile.Para[controlFile.CurrentNum][1]) *
                         (phys.sr - controlFile.Para[controlFile.CurrentNum][1]) +
                     controlFile.Para[controlFile.CurrentNum][0] + errTol.StressExt)
        {
            setMotorCruchUp(false);
        }
        else
        {
            setMotorSpeed(0.0); // RPM -> 0
            if (fabs(phys.sr - controlFile.Para[controlFile.CurrentNum][3]) <= errTol.StressA)
            {
                ++controlFile.CurrentNum;
                TotalStepTime = 0.0;
            }
        }
    }
    else if (controlFile.Para[controlFile.CurrentNum][1] > controlFile.Para[controlFile.CurrentNum][3])
    {
        if (phys.sr > controlFile.Para[controlFile.CurrentNum][3] + errTol.StressA)
        {
            adjustEpCell(-cal.DA_a[daChannel.EP_Cell] * fabs(controlFile.Para[controlFile.CurrentNum][5]) / 60.0 *
                         static_cast<double>(timeSettings.Interval2) / 1000.0);
        }
        if (phys.sr <= controlFile.Para[controlFile.CurrentNum][3] + errTol.StressA)
        {
            adjustEpCell(0.2 * cal.DA_a[daChannel.EP_Cell] * (controlFile.Para[controlFile.CurrentNum][3] - phys.sr));
        }
        if (phys.sa >
            (controlFile.Para[controlFile.CurrentNum][2] - controlFile.Para[controlFile.CurrentNum][0]) /
                    (controlFile.Para[controlFile.CurrentNum][3] - controlFile.Para[controlFile.CurrentNum][1]) *
                    (phys.sr - controlFile.Para[controlFile.CurrentNum][1]) +
                controlFile.Para[controlFile.CurrentNum][0] + errTol.StressCom)
        {
            setMotorCruchUp(true);
        }
        else if (phys.sa <
                 (controlFile.Para[controlFile.CurrentNum][2] - controlFile.Para[controlFile.CurrentNum][0]) /
                         (controlFile.Para[controlFile.CurrentNum][3] - controlFile.Para[controlFile.CurrentNum][1]) *
                         (phys.sr - controlFile.Para[controlFile.CurrentNum][1]) +
                     controlFile.Para[controlFile.CurrentNum][0] + errTol.StressExt)
        {
            setMotorCruchUp(false);
        }
        else
        {
            setMotorSpeed(0.0); // RPM -> 0
            if (fabs(phys.sr - controlFile.Para[controlFile.CurrentNum][3]) <= errTol.StressA)
            {
                ++controlFile.CurrentNum;
                TotalStepTime = 0.0;
            }
        }
    }
}

void DigitShowContext::runScriptedCreep2()
{
    TotalStepTime += CtrlStepTime / 60.0;
    setMotorBrake(false);
    setMotorSpeed(controlFile.Para[controlFile.CurrentNum][0]);
    // 2020.7　側圧一定
    if (phys.sr >= controlFile.Para[controlFile.CurrentNum][3] + errTol.StressA)
    {
        adjustEpCell(-0.1 * cal.DA_a[daChannel.EP_Cell] * (phys.sr - controlFile.Para[controlFile.CurrentNum][3]));
    }
    if (phys.sr <= controlFile.Para[controlFile.CurrentNum][3] - errTol.StressA)
    {
        adjustEpCell(0.1 * cal.DA_a[daChannel.EP_Cell] * (controlFile.Para[controlFile.CurrentNum][3] - phys.sr));
    }
    if (phys.q <= controlFile.Para[controlFile.CurrentNum][1] + errTol.StressExt)
    {
        setMotorCruchUp(false);
        // 2020.5 エラーストレスの大きさに応じたRPMの自動変更
        setMotorSpeed(controlFile.Para[controlFile.CurrentNum][0] *
                      (phys.q - controlFile.Para[controlFile.CurrentNum][1]) / errTol.StressExt / 2);
    }
    else
    {
        setMotorSpeed(0.0); // RPM->0
    }
    if (TotalStepTime >= controlFile.Para[controlFile.CurrentNum][2])
    {
        ++controlFile.CurrentNum;
        TotalStepTime = 0.0;
    }
}
