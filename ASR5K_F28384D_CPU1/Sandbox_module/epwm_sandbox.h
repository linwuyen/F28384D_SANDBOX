/*
 * epwm_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 *
 * EPWM tick observer. The 100kHz tick source itself is the existing
 * EPWM1(MEAS_CNV) -> ADCA1 -> DMA -> dmaCh1ISR chain and is NOT touched
 * here; this module only derives an observable tick state from the chain
 * (sAccessCPU1.u32HeartBeat_CPU1 increments once per 100kHz cycle).
 */

#ifndef SANDBOX_EPWM_SANDBOX_H_
#define SANDBOX_EPWM_SANDBOX_H_

#include <stdint.h>

typedef struct {
    uint32_t u32Tick100k;      /* live 100kHz tick (mirror of CPU1 HB)     */
    uint32_t u32LastSnapshot;  /* tick value at last 500ms window          */
    uint32_t u32TicksPerSec;   /* measured tick rate (expect ~100000)      */
    uint16_t u16TickAlive;     /* 1 = tick advanced since last poll window */
    uint32_t u32PollCount;
} ST_EPWM_SANDBOX;

extern ST_EPWM_SANDBOX g_sEpwmSandbox;

void EpwmSandbox_Init(void);
void EpwmSandbox_Poll(void);   /* call from main loop */

#endif /* SANDBOX_EPWM_SANDBOX_H_ */
