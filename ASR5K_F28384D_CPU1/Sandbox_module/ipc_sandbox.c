/*
 * ipc_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 */

#include "Sandbox_module/ipc_sandbox.h"
#include "common.h"
#include "shareram.h"

ST_IPC_SANDBOX g_sIpcSandbox;

static uint32_t s_u32WindowTs   = 0UL;
static uint32_t s_u32LastCpu2Hb = 0UL;

void IpcSandbox_Init(void)
{
    g_sIpcSandbox.u32Cpu1Hb       = 0UL;
    g_sIpcSandbox.u32Cpu2Hb       = 0UL;
    g_sIpcSandbox.u16Cpu2Alive    = 0U;
    g_sIpcSandbox.u32FakeCpu2Echo = 0UL;
    g_sIpcSandbox.u16EchoIsFake   = 0U;
    g_sIpcSandbox.u32PollCount    = 0UL;
    s_u32WindowTs   = U32_UPCNTS;
    s_u32LastCpu2Hb = 0UL;
}

void IpcSandbox_Poll(void)
{
    uint32_t u32Now = U32_UPCNTS;

    g_sIpcSandbox.u32PollCount++;
    g_sIpcSandbox.u32Cpu1Hb = sAccessCPU1.u32HeartBeat_CPU1;
    g_sIpcSandbox.u32Cpu2Hb = sReadCPU2.u32HeartBeat_CPU2;

    /* Liveness check every 500ms */
    if ((u32Now - s_u32WindowTs) >= T_500MS) {
        g_sIpcSandbox.u16Cpu2Alive =
            (g_sIpcSandbox.u32Cpu2Hb != s_u32LastCpu2Hb) ? 1U : 0U;
        s_u32LastCpu2Hb = g_sIpcSandbox.u32Cpu2Hb;
        s_u32WindowTs   = u32Now;
    }

    /* Fake CPU2 echo while the real CPU2 is silent: mirror the CPU1
     * heartbeat into a CPU1-local variable (no write to CPU2 GSRAM). */
    if (g_sIpcSandbox.u16Cpu2Alive == 0U) {
        g_sIpcSandbox.u32FakeCpu2Echo = g_sIpcSandbox.u32Cpu1Hb;
        g_sIpcSandbox.u16EchoIsFake   = 1U;
    } else {
        g_sIpcSandbox.u32FakeCpu2Echo = g_sIpcSandbox.u32Cpu2Hb;
        g_sIpcSandbox.u16EchoIsFake   = 0U;
    }
}
