/*
 * mcbsp_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 * Updated: 2026-06-12 - fake current-loop plant (CC_DA -> CV_AD lag).
 */

#include "Sandbox_module/mcbsp_sandbox.h"

ST_MCBSP_SANDBOX g_sMcbspSandbox;

/* Fake plant state: CV tracks CC with a first-order lag so the loop is
 * visibly causal in the watch window (CV moves toward CC, not jumps). */
static uint16_t s_u16PlantState = 32768U;

void McbspSandbox_Init(void)
{
    g_sMcbspSandbox.u16Mode        = MCBSP_SANDBOX_MODE_FAKE;
    g_sMcbspSandbox.u16SyncMode    = 1U;       /* R02_3 staged commit     */
    g_sMcbspSandbox.u16CcDaStaged  = 32768U;
    g_sMcbspSandbox.u32CommitCount = 0UL;
    g_sMcbspSandbox.u16CcDaCode    = 32768U;
    g_sMcbspSandbox.u16CvAdSample  = 32768U;
    g_sMcbspSandbox.u16TxWord      = 32768U;
    g_sMcbspSandbox.u16RxWord      = 32768U;
    g_sMcbspSandbox.u32TxCount     = 0UL;
    g_sMcbspSandbox.u32RxCount     = 0UL;
    g_sMcbspSandbox.u32ErrorCount  = 0UL;
    g_sMcbspSandbox.u32PollCount   = 0UL;
    s_u16PlantState = 32768U;
}

void McbspSandbox_WriteCcDa(uint16_t u16Code)
{
    g_sMcbspSandbox.u16CcDaStaged = u16Code;
    g_sMcbspSandbox.u32TxCount++;
    if (g_sMcbspSandbox.u16SyncMode == 0U) {
        McbspSandbox_CommitCcDa();
    }
}

void McbspSandbox_CommitCcDa(void)
{
    /* Product: this body runs in the EPWM CMPC ISR at T=8.0us so every
     * node's AD5543 updates simultaneously (R02_3 6.2). */
    g_sMcbspSandbox.u16CcDaCode = g_sMcbspSandbox.u16CcDaStaged;
    g_sMcbspSandbox.u16TxWord   = g_sMcbspSandbox.u16CcDaCode;
    g_sMcbspSandbox.u32CommitCount++;
}

uint16_t McbspSandbox_ReadCvAd(void)
{
    return g_sMcbspSandbox.u16CvAdSample;
}

void McbspSandbox_Poll(void)
{
    g_sMcbspSandbox.u32PollCount++;

    switch (g_sMcbspSandbox.u16Mode) {
    case MCBSP_SANDBOX_MODE_FAKE: {
        /* First-order lag: state += (cc - state) / 8 */
        int32_t s32Err = (int32_t)g_sMcbspSandbox.u16CcDaCode
                       - (int32_t)s_u16PlantState;
        s_u16PlantState = (uint16_t)((int32_t)s_u16PlantState + (s32Err >> 3));

        g_sMcbspSandbox.u16CvAdSample = s_u16PlantState;
        g_sMcbspSandbox.u16RxWord     = s_u16PlantState;
        g_sMcbspSandbox.u32RxCount++;

        /* Self-check: with no new CC writes the plant must converge, i.e.
         * |cv - cc| shrinks; a stuck plant is flagged once converged-and-
         * wrong. Cheap sanity, expect 0. */
        if ((s32Err == 0L) &&
            (g_sMcbspSandbox.u16CvAdSample != g_sMcbspSandbox.u16CcDaCode)) {
            g_sMcbspSandbox.u32ErrorCount++;
        }
        break;
    }

    case MCBSP_SANDBOX_MODE_DLB:
    case MCBSP_SANDBOX_MODE_EXTERNAL:
    default:
        /* Hardware modes are future bring-up placeholders */
        break;
    }
}
