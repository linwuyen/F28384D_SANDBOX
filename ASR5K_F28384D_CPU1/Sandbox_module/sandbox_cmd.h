/*
 * sandbox_cmd.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX_PHASE1)
 *
 * Software Command Injector - drives the DDS/Wave sandbox without an AM3352.
 *
 * Two ways to fire a command (both end in the same mailbox):
 *   1. Debugger: write g_sSandboxCmd.u32Param, then u16Cmd, then
 *      u16Pending = 1. The main loop dispatches it on the next pass.
 *   2. Code: SandboxCmd_Inject(cmd, param) - same mailbox, used later by
 *      the SPIB loopback path to map real protocol commands onto sandbox
 *      commands without touching the official SPIB protocol.
 *
 * Dispatch happens in main-loop context only (never in the 100kHz ISR), so
 * every handler is plain DDS_/WaveMem_ API use with no ISR race. The active
 * wave pointer republish on WAVE_ACTIVATE is a single 32-bit write (atomic
 * on C28x).
 */

#ifndef SANDBOX_CMD_H_
#define SANDBOX_CMD_H_

#include <stdint.h>

//-----------------------------------------------------------------------------
// Command codes
//-----------------------------------------------------------------------------
typedef enum {
    SANDBOX_CMD_NONE            = 0U,
    SANDBOX_CMD_OUTPUT_ON       = 1U,  /* param unused                        */
    SANDBOX_CMD_OUTPUT_OFF      = 2U,  /* param unused                        */
    SANDBOX_CMD_DDS_FREQ        = 3U,  /* param = frequency x100 (0=DC mode)  */
    SANDBOX_CMD_DDS_AMP         = 4U,  /* param = amplitude 0..65535          */
    SANDBOX_CMD_DDS_OFFSET      = 5U,  /* param = offset 0..65535             */
    SANDBOX_CMD_WAVE_ACTIVATE   = 6U,  /* param = wave page id                */

    /* R01_1 full DDS feature set (ramps / delays / phases).
     * Ramp param packing: up_ms<<16 | down_ms (1..60000 each);
     *                     param == 0 disables the ramp.
     * Delay param: milliseconds 0..60000 (0 = no delay = "disable").
     * Phase param: degrees x10, 0..3600; 0xFFFFFFFF = immediate (-1). */
    SANDBOX_CMD_DDS_AMP_RAMP    = 7U,
    SANDBOX_CMD_DDS_OFFSET_RAMP = 8U,
    SANDBOX_CMD_DDS_FREQ_RAMP   = 9U,
    SANDBOX_CMD_DDS_DELAY_ON    = 10U,
    SANDBOX_CMD_DDS_DELAY_OFF   = 11U,
    SANDBOX_CMD_DDS_PHASE_ON    = 12U,
    SANDBOX_CMD_DDS_PHASE_OFF   = 13U
} EN_SANDBOX_CMD;

#define SANDBOX_CMD_MAX           13U
#define SANDBOX_PHASE_IMMEDIATE   0xFFFFFFFFUL

//-----------------------------------------------------------------------------
// Command mailbox (watch-friendly)
//-----------------------------------------------------------------------------
typedef struct {
    volatile uint16_t u16Cmd;       /* command to execute (EN_SANDBOX_CMD)   */
    volatile uint32_t u32Param;     /* command parameter                     */
    volatile uint16_t u16Pending;   /* write 1 to fire; cleared on dispatch  */

    uint16_t u16LastCmd;            /* last dispatched command               */
    uint32_t u32LastParam;          /* last dispatched parameter             */
    uint16_t u16LastResult;         /* 1 = accepted, 0 = rejected            */
    uint32_t u32DispatchCount;      /* total accepted commands               */
    uint32_t u32RejectCount;        /* total rejected commands               */
} ST_SANDBOX_CMD;

extern ST_SANDBOX_CMD g_sSandboxCmd;

/** @brief Reset the mailbox and counters. */
void SandboxCmd_Init(void);

/** @brief Queue a command from code (software injector entry point). */
void SandboxCmd_Inject(uint16_t u16Cmd, uint32_t u32Param);

/** @brief Dispatch a pending command. Call from the main loop. */
void SandboxCmd_Poll(void);

#endif /* SANDBOX_CMD_H_ */
