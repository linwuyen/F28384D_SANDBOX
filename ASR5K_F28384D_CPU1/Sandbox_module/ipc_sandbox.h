/*
 * ipc_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 *
 * IPC / CPU1-CPU2 shared RAM observer with fake-CPU2 echo.
 *
 * CPU1 heartbeat: incremented by the existing 100kHz dmaCh1ISR
 * (sAccessCPU1.u32HeartBeat_CPU1) - untouched here.
 * CPU2 heartbeat: read from the CPU2-owned shared region (sReadCPU2).
 *
 * If CPU2 firmware is silent (heartbeat not advancing), this module
 * provides a CPU1-side FAKE echo so downstream consumers and the watch
 * window still see a "CPU2-like" signal. The fake NEVER writes into the
 * CPU2-owned GSRAM region (CPU1 is read-only there by MemCfg master
 * select); it lives entirely in g_sIpcSandbox.
 */

#ifndef SANDBOX_IPC_SANDBOX_H_
#define SANDBOX_IPC_SANDBOX_H_

#include <stdint.h>

typedef struct {
    uint32_t u32Cpu1Hb;        /* snapshot of CPU1 heartbeat (100kHz)      */
    uint32_t u32Cpu2Hb;        /* snapshot of CPU2 heartbeat               */
    uint16_t u16Cpu2Alive;     /* 1 = CPU2 heartbeat advanced last window  */
    uint32_t u32FakeCpu2Echo;  /* fake echo counter when CPU2 is silent    */
    uint16_t u16EchoIsFake;    /* 1 = echo currently faked by CPU1         */
    uint32_t u32PollCount;
} ST_IPC_SANDBOX;

extern ST_IPC_SANDBOX g_sIpcSandbox;

void IpcSandbox_Init(void);
void IpcSandbox_Poll(void);

#endif /* SANDBOX_IPC_SANDBOX_H_ */
