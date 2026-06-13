/*
 * epwm_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 */

#include "Sandbox_module/epwm_sandbox.h"
#include "common.h"
#include "shareram.h"

ST_EPWM_SANDBOX g_sEpwmSandbox;

static uint32_t s_u32WindowTs = 0UL;

void EpwmSandbox_Init(void)
{
    g_sEpwmSandbox.u32Tick100k     = 0UL;
    g_sEpwmSandbox.u32LastSnapshot = 0UL;
    g_sEpwmSandbox.u32TicksPerSec  = 0UL;
    g_sEpwmSandbox.u16TickAlive    = 0U;
    g_sEpwmSandbox.u32PollCount    = 0UL;
    s_u32WindowTs = U32_UPCNTS;
}

void EpwmSandbox_Poll(void)
{
    uint32_t u32Now = U32_UPCNTS;

    g_sEpwmSandbox.u32PollCount++;
    g_sEpwmSandbox.u32Tick100k = sAccessCPU1.u32HeartBeat_CPU1;

    /* Measure tick rate over a 500ms window (x2 -> ticks per second) */
    if ((u32Now - s_u32WindowTs) >= T_500MS) {
        uint32_t u32Delta = g_sEpwmSandbox.u32Tick100k
                          - g_sEpwmSandbox.u32LastSnapshot;
        g_sEpwmSandbox.u32TicksPerSec  = u32Delta * 2UL;
        g_sEpwmSandbox.u16TickAlive    = (u32Delta != 0UL) ? 1U : 0U;
        g_sEpwmSandbox.u32LastSnapshot = g_sEpwmSandbox.u32Tick100k;
        s_u32WindowTs = u32Now;
    }
}
