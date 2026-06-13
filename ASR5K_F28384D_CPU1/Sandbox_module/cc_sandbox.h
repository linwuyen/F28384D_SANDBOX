/*
 * cc_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 *
 * CC (compensation / control output) sink abstraction.
 *
 * Product use: the CC output goes to the power-stage CCDA (AD5543 behind
 * MCBSP). The core only calls CcSandbox_Write(code); the sink behind it is
 * runtime-selectable:
 *
 *   CC_SINK_DEBUG        - debug variable only
 *   CC_SINK_INTERNAL_DAC - DACB shadow (DACA is owned by the DDS output
 *                          sink), observable on the DACB pin
 *   CC_SINK_MCBSP_CCDA   - placeholder: hands the code to the MCBSP
 *                          sandbox service (future real CCDA frame path)
 *
 * In the sandbox a self-test pattern generator in CcSandbox_Poll() feeds
 * the sink with a slow ramp derived from the CV sandbox value, proving the
 * CV -> CC -> sink dataflow without any control law (the real CC algorithm
 * arrives with the System State Machine phase, which is out of scope).
 */

#ifndef SANDBOX_CC_SANDBOX_H_
#define SANDBOX_CC_SANDBOX_H_

#include <stdint.h>

#define CC_SINK_DEBUG         0U
#define CC_SINK_INTERNAL_DAC  1U
#define CC_SINK_MCBSP_CCDA    2U   /* placeholder via MCBSP sandbox */

typedef struct {
    uint16_t u16Sink;          /* CC_SINK_*, runtime switchable            */
    uint16_t u16PatternEnable; /* 1 = poll feeds CV-derived test pattern   */
    uint16_t u16LastCode;      /* last code written to the sink            */
    uint32_t u32WriteCount;
    uint32_t u32PollCount;
} ST_CC_SANDBOX;

extern ST_CC_SANDBOX g_sCcSandbox;

void CcSandbox_Init(void);
void CcSandbox_Write(uint16_t u16Code);   /* core-facing sink entry        */
void CcSandbox_Poll(void);                /* test pattern generator        */

#endif /* SANDBOX_CC_SANDBOX_H_ */
