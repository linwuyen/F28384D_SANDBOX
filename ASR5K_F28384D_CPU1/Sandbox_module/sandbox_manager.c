/*
 * sandbox_manager.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 */

#include "Sandbox_module/sandbox_manager.h"
#include "Sandbox_module/output_sink.h"
#include "Sandbox_module/sandbox_cmd.h"
#include "Sandbox_module/epwm_sandbox.h"
#include "Sandbox_module/cv_sandbox.h"
#include "Sandbox_module/cc_sandbox.h"
#include "Sandbox_module/fsi_sandbox.h"
#include "Sandbox_module/mcbsp_sandbox.h"
#include "Sandbox_module/am3352_sandbox.h"
#include "Sandbox_module/flash_sandbox.h"
#include "Sandbox_module/m0_sandbox.h"
#include "Sandbox_module/ipc_sandbox.h"
#include "Sandbox_module/seq_sandbox.h"
#include "Wave_module/wave_memory_backend.h"
#include "dds/dds_api.h"

ST_SANDBOX_OVERVIEW g_sSandboxOverview;

/* Throttle for the slow fake generators (CV/CC/FSI): every 64 main-loop
 * passes, so counters stay human-readable in the watch window while the
 * dataflow is still clearly alive. */
#define SANDBOX_SLOW_DIV_MASK  0x3FUL

static uint32_t s_u32PollDivider = 0UL;

void Sandbox_InitAll(void)
{
    OutputSink_Init();
    SandboxCmd_Init();
    EpwmSandbox_Init();
    CvSandbox_Init();
    CcSandbox_Init();
    FsiSandbox_Init();
    McbspSandbox_Init();
    Am3352Sandbox_Init();
    FlashSandbox_Init();
    M0Sandbox_Init();
    IpcSandbox_Init();
    SeqSandbox_Init();

    s_u32PollDivider = 0UL;
    g_sSandboxOverview.u32PollCount = 0UL;
}

static void sandboxRefreshOverview(void)
{
    g_sSandboxOverview.u32Tick100k     = g_sEpwmSandbox.u32Tick100k;
    g_sSandboxOverview.u32TicksPerSec  = g_sEpwmSandbox.u32TicksPerSec;

    g_sSandboxOverview.u16WaveBackend  = g_sWaveMem.u16Backend;
    g_sSandboxOverview.u16WaveReady    = g_sWaveMem.u16Ready;
    g_sSandboxOverview.u16MemTestPass  = g_sWaveMem.u16MemTestPass;
    g_sSandboxOverview.u32DdsState     = (uint32_t)sDDS.fgState;
    g_sSandboxOverview.u16DdsIndex     = sDDS.u16RtIndex;
    g_sSandboxOverview.u16DacCode      = g_sOutputSink.u16LastCode;

    g_sSandboxOverview.i16CvValue      = g_sCvSandbox.i16Cv;
    g_sSandboxOverview.u16CvSource     = g_sCvSandbox.u16Source;
    g_sSandboxOverview.u16CcCode       = g_sCcSandbox.u16LastCode;
    g_sSandboxOverview.u16CcSink       = g_sCcSandbox.u16Sink;

    g_sSandboxOverview.u32FsiTxCount   = g_sFsiSandbox.u32TxCount;
    g_sSandboxOverview.u32FsiErrCount  = g_sFsiSandbox.u32ErrorCount;
    g_sSandboxOverview.u32McbspTxCount = g_sMcbspSandbox.u32TxCount;
    g_sSandboxOverview.u32CmdDispatch  = g_sSandboxCmd.u32DispatchCount;
    g_sSandboxOverview.u32Am3352Forward= g_sAm3352Sandbox.u32PktOkCount;
    g_sSandboxOverview.u32FlashWrites  = g_sFlashSandbox.u32WriteCount;
    g_sSandboxOverview.u32M0Xfers      = g_sM0Sandbox.u32XferCount;

    g_sSandboxOverview.u32Cpu1Hb       = g_sIpcSandbox.u32Cpu1Hb;
    g_sSandboxOverview.u32Cpu2Hb       = g_sIpcSandbox.u32Cpu2Hb;
    g_sSandboxOverview.u16Cpu2Alive    = g_sIpcSandbox.u16Cpu2Alive;

    g_sSandboxOverview.u32PollCount++;
}

void Sandbox_PollAll(void)
{
    s_u32PollDivider++;

    /* Command layer first: AM3352 protocol injector feeds the dispatcher,
     * the dispatcher feeds DDS/Wave - both settle within this pass. */
    Am3352Sandbox_Poll();
    SandboxCmd_Poll();

    /* Fast observers */
    EpwmSandbox_Poll();
    IpcSandbox_Poll();

    /* Slow fake generators. Order models the R02_3 control cycle:
     * CV refresh -> CC stages CC_DA -> FSI frame distributes (carries
     * CV_AD, R02_2 16-word layout) -> global update point commits CC_DA
     * (product: EPWM CMPC ISR @ T=8.0us) -> plant responds. */
    if ((s_u32PollDivider & SANDBOX_SLOW_DIV_MASK) == 0UL) {
        CvSandbox_Poll();
        CcSandbox_Poll();
        FsiSandbox_Poll();
        McbspSandbox_CommitCcDa();   /* R02_3 synchronized output point */
        McbspSandbox_Poll();
        SeqSandbox_Poll();
    }
    FlashSandbox_Poll();   /* self-throttled */
    M0Sandbox_Poll();      /* self-throttled */

    sandboxRefreshOverview();
}
