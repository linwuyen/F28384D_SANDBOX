/*
 * sandbox_manager.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 *
 * Single entry point for every sandbox module: one init call, one poll
 * call from the main loop. Also maintains g_sSandboxOverview - a compact
 * "minimal debug table" aggregating the key state of every module so a
 * single watch expression shows the whole sandbox dataflow (and is the
 * natural future mapping target for the SCI/Modbus debug readout, without
 * touching mb_slave).
 */

#ifndef SANDBOX_MANAGER_H_
#define SANDBOX_MANAGER_H_

#include <stdint.h>

//-----------------------------------------------------------------------------
// Minimal debug table (one-stop watch: g_sSandboxOverview)
//-----------------------------------------------------------------------------
typedef struct {
    /* EPWM / runtime tick */
    uint32_t u32Tick100k;       /* 100kHz tick (CPU1 HB mirror)            */
    uint32_t u32TicksPerSec;    /* measured, expect ~100000                */

    /* Wave / DDS */
    uint16_t u16WaveBackend;    /* 0=GSRAM_FAKE 1=HOME_SRAM 2=PROD_SDRAM   */
    uint16_t u16WaveReady;
    uint16_t u16MemTestPass;
    uint32_t u32DdsState;       /* sDDS.fgState (0x08 = RUNNING)           */
    uint16_t u16DdsIndex;       /* 0..4095 cycling                         */
    uint16_t u16DacCode;        /* last DDS output code                    */

    /* CV / CC */
    int16_t  i16CvValue;
    uint16_t u16CvSource;
    uint16_t u16CcCode;
    uint16_t u16CcSink;

    /* Comms stubs */
    uint32_t u32FsiTxCount;
    uint32_t u32FsiErrCount;
    uint32_t u32McbspTxCount;
    uint32_t u32CmdDispatch;    /* sandbox dispatcher accepted commands    */
    uint32_t u32Am3352Forward;  /* AM3352 injector forwarded commands      */
    uint32_t u32FlashWrites;
    uint32_t u32M0Xfers;

    /* IPC */
    uint32_t u32Cpu1Hb;
    uint32_t u32Cpu2Hb;
    uint16_t u16Cpu2Alive;

    uint32_t u32PollCount;
} ST_SANDBOX_OVERVIEW;

extern ST_SANDBOX_OVERVIEW g_sSandboxOverview;

/** @brief Init every sandbox module (incl. output sink + cmd dispatcher). */
void Sandbox_InitAll(void);

/** @brief Poll every sandbox module + refresh the overview table. */
void Sandbox_PollAll(void);

#endif /* SANDBOX_MANAGER_H_ */
