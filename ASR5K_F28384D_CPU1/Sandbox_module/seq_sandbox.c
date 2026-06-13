/*
 * seq_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 */

#include "Sandbox_module/seq_sandbox.h"
#include "Sandbox_module/sandbox_cmd.h"

ST_SEQ_SANDBOX g_sSeqSandbox;

void SeqSandbox_Init(void)
{
    uint16_t u16Step;
    uint16_t u16Item;

    g_sSeqSandbox.u16Enable       = 1U;
    g_sSeqSandbox.u16FireActivate = 0U;   /* default observe-only          */
    g_sSeqSandbox.u16CurrentStep  = 0U;
    g_sSeqSandbox.u32StepCount    = 0UL;
    g_sSeqSandbox.u32WrapCount    = 0UL;
    g_sSeqSandbox.u32PollCount    = 0UL;

    /* Fake table: item[0]=page id (always 0 on GSRAM fake), item[1]=dwell
     * in slow-poll ticks, item[2..] recognizable pattern */
    for (u16Step = 0U; u16Step < SEQ_SANDBOX_FAKE_STEPS; u16Step++) {
        g_sSeqSandbox.asStep[u16Step].au32Item[0] = 0UL;
        g_sSeqSandbox.asStep[u16Step].au32Item[1] = 8UL + (uint32_t)u16Step * 4UL;
        for (u16Item = 2U; u16Item < SEQ_LAYOUT_ITEMS_PER_STEP; u16Item++) {
            g_sSeqSandbox.asStep[u16Step].au32Item[u16Item] =
                ((uint32_t)u16Step << 16) | u16Item;
        }
    }
    g_sSeqSandbox.u16DwellLeft =
        (uint16_t)g_sSeqSandbox.asStep[0].au32Item[1];
}

void SeqSandbox_Poll(void)
{
    g_sSeqSandbox.u32PollCount++;

    if (g_sSeqSandbox.u16Enable == 0U) {
        return;
    }

    if (g_sSeqSandbox.u16DwellLeft > 0U) {
        g_sSeqSandbox.u16DwellLeft--;
        return;
    }

    /* Advance to the next step */
    g_sSeqSandbox.u16CurrentStep++;
    if (g_sSeqSandbox.u16CurrentStep >= SEQ_SANDBOX_FAKE_STEPS) {
        g_sSeqSandbox.u16CurrentStep = 0U;
        g_sSeqSandbox.u32WrapCount++;
    }
    g_sSeqSandbox.u16DwellLeft = (uint16_t)
        g_sSeqSandbox.asStep[g_sSeqSandbox.u16CurrentStep].au32Item[1];
    g_sSeqSandbox.u32StepCount++;

    /* Exercise the SEQ -> wave activation -> DDS chain on demand */
    if (g_sSeqSandbox.u16FireActivate != 0U) {
        SandboxCmd_Inject((uint16_t)SANDBOX_CMD_WAVE_ACTIVATE,
            g_sSeqSandbox.asStep[g_sSeqSandbox.u16CurrentStep].au32Item[0]);
    }
}
