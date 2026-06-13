/*
 * fsi_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 * Updated: 2026-06-13 - frame layout aligned to the ADOPTED standard
 *          R02_2_FSI_OPT_PACKAGE (16 words, 1 Master + 3 Slaves), per
 *          R02_4 conclusion. Node discovery / skew calibration modeled
 *          per R02_1.
 *
 * FSI daisy-chain communication stub.
 *
 * Product chain (R02_1): Master TX -> S1 -> S2 (-> S3) -> Master RX over
 * LVDS/RJ45, 50MHz DDR (200Mbps), single aggregated 16-word frame per
 * 10us control cycle. Hardware CRC is provided by the FSI peripheral.
 *
 * R02_2 standard frame (16 words, full M+3S):
 *   [0]  L1_Ref    [1] L2_Ref   [2] L3_Ref     (Master -> all)
 *   [3]  CV_AD     [4] ID_Func  [5] Data_H  [6] Data_L
 *   [7..9]   S1: Vout, Iout, Status
 *   [10..12] S2: Vout, Iout, Status
 *   [13..15] S3: Vout, Iout, Status
 * Master payload fields map 1:1 onto the existing shareram M_Ref block
 * (sAccessCPU1.u16L1_Ref/u16L2_Ref/u16L3_Ref/u16FuncID/u16Data_H/u16Data_L)
 * plus CV_AD from the MCBSP service.
 *
 * Modes:
 *   FSI_SANDBOX_MODE_FAKE    - software wire: this node acts as Master,
 *                              fake S1..S3 fill their slots; "CRC" cannot
 *                              fail (counter stays 0)
 *   FSI_SANDBOX_MODE_HW_LOOP - placeholder: FSITXA->FSIRXA loopback
 *   FSI_SANDBOX_MODE_DAISY   - placeholder: real chain incl. RX_DLY_LINE
 *                              skew training and token-passing discovery
 */

#ifndef SANDBOX_FSI_SANDBOX_H_
#define SANDBOX_FSI_SANDBOX_H_

#include <stdint.h>

#define FSI_SANDBOX_MODE_FAKE     0U
#define FSI_SANDBOX_MODE_HW_LOOP  1U   /* placeholder */
#define FSI_SANDBOX_MODE_DAISY    2U   /* placeholder */

#define FSI_SANDBOX_FRAME_WORDS   16U  /* R02_2 standard, FSI hw maximum */

/* R02_2 word offsets */
#define FSI_FRM_L1_REF     0U
#define FSI_FRM_L2_REF     1U
#define FSI_FRM_L3_REF     2U
#define FSI_FRM_CV_AD      3U
#define FSI_FRM_ID_FUNC    4U   /* NodeID(4b) + FuncID(12b) */
#define FSI_FRM_DATA_H     5U
#define FSI_FRM_DATA_L     6U
#define FSI_FRM_S1_VOUT    7U
#define FSI_FRM_S1_IOUT    8U
#define FSI_FRM_S1_STAT    9U
#define FSI_FRM_S2_VOUT    10U
#define FSI_FRM_S2_IOUT    11U
#define FSI_FRM_S2_STAT    12U
#define FSI_FRM_S3_VOUT    13U
#define FSI_FRM_S3_IOUT    14U
#define FSI_FRM_S3_STAT    15U

typedef struct {
    uint16_t u16Mode;          /* FSI_SANDBOX_MODE_*                       */

    /* R02_1 initialization model (token passing / training placeholders) */
    uint16_t u16NodeId;        /* this node id (FAKE: 0 = Master)          */
    uint16_t u16NodeCount;     /* discovered nodes (FAKE: 1)               */
    uint16_t u16EnSlave;       /* GPIO3 EN_SLAVE state model               */
    uint16_t u16DlyTap;        /* RX_DLY_LINE_CTRL tap (training result)   */

    uint16_t au16TxFrame[FSI_SANDBOX_FRAME_WORDS];
    uint16_t au16RxFrame[FSI_SANDBOX_FRAME_WORDS];  /* last_frame          */

    uint32_t u32TxCount;
    uint32_t u32RxCount;
    uint32_t u32ErrorCount;    /* modeled CRC/frame errors (expect 0)      */
    uint32_t u32PollCount;
} ST_FSI_SANDBOX;

extern ST_FSI_SANDBOX g_sFsiSandbox;

void FsiSandbox_Init(void);
void FsiSandbox_Poll(void);

/** @brief CV_AD broadcast word from the last received frame (the value a
 *  node uses to update its CC_DA per R02_3 loop). */
uint16_t FsiSandbox_GetRxCvAd(void);

#endif /* SANDBOX_FSI_SANDBOX_H_ */
