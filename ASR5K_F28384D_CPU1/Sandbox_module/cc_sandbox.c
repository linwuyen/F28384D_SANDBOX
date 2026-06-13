/*
 * cc_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 */

#include "Sandbox_module/cc_sandbox.h"
#include "Sandbox_module/cv_sandbox.h"
#include "Sandbox_module/mcbsp_sandbox.h"
#include "driverlib.h"

ST_CC_SANDBOX g_sCcSandbox;

void CcSandbox_Init(void)
{
    /* Default = MCBSP stub so the modeled current loop (CC_DA -> plant ->
     * CV_AD -> FSI) is alive out of the box; pure software, no hardware
     * touched. Switch to 1 (DACB) only when probing with a scope. */
    g_sCcSandbox.u16Sink          = CC_SINK_MCBSP_CCDA;
    g_sCcSandbox.u16PatternEnable = 1U;
    g_sCcSandbox.u16LastCode      = 0U;
    g_sCcSandbox.u32WriteCount    = 0UL;
    g_sCcSandbox.u32PollCount     = 0UL;
}

void CcSandbox_Write(uint16_t u16Code)
{
    g_sCcSandbox.u16LastCode = u16Code;
    g_sCcSandbox.u32WriteCount++;

    switch (g_sCcSandbox.u16Sink) {
    case CC_SINK_INTERNAL_DAC:
        /* DACB (12-bit). DACA belongs to the DDS output sink. */
        DAC_setShadowValue(DACB_BASE, (uint16_t)(u16Code >> 4));
        break;

    case CC_SINK_MCBSP_CCDA:
        /* Placeholder real path: word handed to the MCBSP service stub */
        McbspSandbox_WriteTx(u16Code);
        break;

    case CC_SINK_DEBUG:
    default:
        /* debug variable only (already stored above) */
        break;
    }
}

void CcSandbox_Poll(void)
{
    g_sCcSandbox.u32PollCount++;

    /* Sandbox test pattern: map the CV value (+/-32768) onto a unipolar
     * 16-bit CC code. Proves CV source -> CC sink dataflow end to end. */
    if (g_sCcSandbox.u16PatternEnable != 0U) {
        int32_t s32Code = (int32_t)CvSandbox_GetValue() + 32768L;
        CcSandbox_Write((uint16_t)s32Code);
    }
}
