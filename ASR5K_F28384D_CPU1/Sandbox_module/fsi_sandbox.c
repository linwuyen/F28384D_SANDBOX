/*
 * fsi_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 * Updated: 2026-06-13 - R02_2 standard 16-word M+3S frame; master payload
 *          wired from the shareram M_Ref block; fake S1..S3 slot fill.
 */

#include "Sandbox_module/fsi_sandbox.h"
#include "Sandbox_module/mcbsp_sandbox.h"
#include "shareram.h"

ST_FSI_SANDBOX g_sFsiSandbox;

static uint16_t s_u16FakeSlaveTick = 0U;

void FsiSandbox_Init(void)
{
    uint16_t u16I;

    g_sFsiSandbox.u16Mode       = FSI_SANDBOX_MODE_FAKE;
    g_sFsiSandbox.u16NodeId     = 0U;   /* FAKE: this node is the Master   */
    g_sFsiSandbox.u16NodeCount  = 1U;   /* FAKE: instant discovery, 1 node */
    g_sFsiSandbox.u16EnSlave    = 1U;   /* TX enable model (GPIO3)         */
    g_sFsiSandbox.u16DlyTap     = 16U;  /* fake centered training tap      */
    g_sFsiSandbox.u32TxCount    = 0UL;
    g_sFsiSandbox.u32RxCount    = 0UL;
    g_sFsiSandbox.u32ErrorCount = 0UL;
    g_sFsiSandbox.u32PollCount  = 0UL;
    for (u16I = 0U; u16I < FSI_SANDBOX_FRAME_WORDS; u16I++) {
        g_sFsiSandbox.au16TxFrame[u16I] = 0U;
        g_sFsiSandbox.au16RxFrame[u16I] = 0U;
    }
    s_u16FakeSlaveTick = 0U;
}

uint16_t FsiSandbox_GetRxCvAd(void)
{
    return g_sFsiSandbox.au16RxFrame[FSI_FRM_CV_AD];
}

void FsiSandbox_Poll(void)
{
    uint16_t u16I;

    g_sFsiSandbox.u32PollCount++;

    if (g_sFsiSandbox.u16Mode != FSI_SANDBOX_MODE_FAKE) {
        /* HW_LOOP / DAISY (incl. RX delay-line training and token-passing
         * discovery per R02_1) are real-FSI bring-up placeholders. */
        return;
    }

    /* Master payload (words 0..6) - straight from the shareram M_Ref block
     * the 100kHz ISR maintains, plus CV_AD from the MCBSP service. */
    g_sFsiSandbox.au16TxFrame[FSI_FRM_L1_REF]  = sAccessCPU1.u16L1_Ref;
    g_sFsiSandbox.au16TxFrame[FSI_FRM_L2_REF]  = sAccessCPU1.u16L2_Ref;
    g_sFsiSandbox.au16TxFrame[FSI_FRM_L3_REF]  = sAccessCPU1.u16L3_Ref;
    g_sFsiSandbox.au16TxFrame[FSI_FRM_CV_AD]   = McbspSandbox_ReadCvAd();
    g_sFsiSandbox.au16TxFrame[FSI_FRM_ID_FUNC] = sAccessCPU1.u16FuncID;
    g_sFsiSandbox.au16TxFrame[FSI_FRM_DATA_H]  = sAccessCPU1.u16Data_H;
    g_sFsiSandbox.au16TxFrame[FSI_FRM_DATA_L]  = sAccessCPU1.u16Data_L;

    /* Fake S1..S3 overwrite their slots like the DMA 3-stage assembly
     * would: recognizable Vout, Iout tracking CV, alive Status counter. */
    s_u16FakeSlaveTick++;
    for (u16I = 0U; u16I < 3U; u16I++) {
        uint16_t u16Base = (uint16_t)(FSI_FRM_S1_VOUT + (u16I * 3U));
        g_sFsiSandbox.au16TxFrame[u16Base]      =
            (uint16_t)(0x0E10U + u16I);                       /* Vout    */
        g_sFsiSandbox.au16TxFrame[u16Base + 1U] =
            g_sFsiSandbox.au16TxFrame[FSI_FRM_CV_AD];         /* Iout    */
        g_sFsiSandbox.au16TxFrame[u16Base + 2U] =
            (uint16_t)(((u16I + 1U) << 12) | (s_u16FakeSlaveTick & 0x0FFFU));
    }
    g_sFsiSandbox.u32TxCount++;

    /* Software wire (loop closes back to this node). FSI hardware CRC is
     * modeled as never-failing here; the counter exists for the real
     * modes. */
    for (u16I = 0U; u16I < FSI_SANDBOX_FRAME_WORDS; u16I++) {
        g_sFsiSandbox.au16RxFrame[u16I] = g_sFsiSandbox.au16TxFrame[u16I];
    }
    g_sFsiSandbox.u32RxCount++;
}
