/*
 * mcbsp_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 * Updated: 2026-06-12 - full-duplex current-loop model per the product
 *          system diagram (McBSPA 25MHz carries BOTH converters).
 *
 * MCBSP service stub for the current-loop path.
 *
 * Product use (系統圖「電流環」): McBSPA (GPIO165-168/5/7) carries
 *   - CC_DA : AD5543 16-bit DAC  (TX direction, current command out)
 *   - CV_AD : AD7915 16-bit ADC  (RX direction, current feedback in)
 * Loop: CV_AD sample -> FSI to lead unit -> current-share % back over
 * FSI -> CC_DA. The FSI leg is modeled in fsi_sandbox; this module models
 * the converter word exchange.
 *
 * Modes:
 *   MCBSP_SANDBOX_MODE_FAKE     - software plant: CV_AD sample is a fake
 *                                 feedback derived from the last CC_DA code
 *                                 (first-order lag), no peripheral access
 *   MCBSP_SANDBOX_MODE_DLB      - placeholder: MCBSP digital loopback
 *   MCBSP_SANDBOX_MODE_EXTERNAL - placeholder: real AD5543/AD7915 frames
 *
 * Porting: the future real driver implements the same
 * WriteCcDa()/ReadCvAd() pair; callers never change.
 */

#ifndef SANDBOX_MCBSP_SANDBOX_H_
#define SANDBOX_MCBSP_SANDBOX_H_

#include <stdint.h>

#define MCBSP_SANDBOX_MODE_FAKE      0U
#define MCBSP_SANDBOX_MODE_DLB       1U   /* placeholder */
#define MCBSP_SANDBOX_MODE_EXTERNAL  2U   /* placeholder */

typedef struct {
    uint16_t u16Mode;        /* MCBSP_SANDBOX_MODE_*                       */

    /* R02_3 synchronized output: CC_DA writes are STAGED, then committed
     * at the global update point (product: EPWM CMPC ISR @ T=8.0us; the
     * sandbox manager calls Commit once per slow tick after the FSI poll).
     * u16SyncMode=0 makes Write commit immediately (bypass). */
    uint16_t u16SyncMode;    /* 1 = staged commit (default), 0 = immediate */
    uint16_t u16CcDaStaged;  /* staged CC_DA awaiting the sync point       */
    uint32_t u32CommitCount;

    /* Current-loop converter model */
    uint16_t u16CcDaCode;    /* committed CC_DA code (AD5543 direction)    */
    uint16_t u16CvAdSample;  /* latest CV_AD sample (AD7915 direction)     */

    /* Legacy word-level view (kept for generic use)                       */
    uint16_t u16TxWord;      /* == last CC_DA code                         */
    uint16_t u16RxWord;      /* == last CV_AD sample                       */

    uint32_t u32TxCount;
    uint32_t u32RxCount;
    uint32_t u32ErrorCount;  /* FAKE self-check mismatches (expect 0)      */
    uint32_t u32PollCount;
} ST_MCBSP_SANDBOX;

extern ST_MCBSP_SANDBOX g_sMcbspSandbox;

void McbspSandbox_Init(void);

/** @brief CC_DA write (AD5543 direction). Staged when u16SyncMode=1. */
void McbspSandbox_WriteCcDa(uint16_t u16Code);

/** @brief Global update point (R02_3 CMPC model): commit staged CC_DA. */
void McbspSandbox_CommitCcDa(void);

/** @brief Latest CV_AD sample (AD7915 direction). */
uint16_t McbspSandbox_ReadCvAd(void);

/* Generic word aliases (CC sink and legacy callers) */
#define McbspSandbox_WriteTx(w)  McbspSandbox_WriteCcDa(w)
#define McbspSandbox_ReadRx()    McbspSandbox_ReadCvAd()

/** @brief Service poll: FAKE plant updates CV_AD from CC_DA (1st-order). */
void McbspSandbox_Poll(void);

#endif /* SANDBOX_MCBSP_SANDBOX_H_ */
