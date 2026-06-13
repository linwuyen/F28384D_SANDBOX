/*
 * seq_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 *
 * SEQ (wave sequence) stepper stub.
 *
 * Product use: a 1000-step sequence table (35 x 32-bit items per step,
 * layout in Wave_module/wave_seq_layout.h) drives wave page switching and
 * operating parameters over time. The real SEQ runtime is M5-adjacent and
 * out of scope; this stub exercises the step record geometry and the page
 * switching contract against the Wave backend:
 *
 *   - holds a tiny fake table (SEQ_SANDBOX_FAKE_STEPS steps) in internal RAM
 *   - steps through it slowly; item[0] is interpreted as "wave page id"
 *     and item[1] as "dwell" (poll ticks), everything else free-form
 *   - on each step it (optionally) fires SANDBOX_CMD_WAVE_ACTIVATE so the
 *     full SEQ -> wave activation -> DDS pointer chain is exercised
 *     (GSRAM fake has a single page, so the activation re-targets page 0 -
 *     the call path is what matters)
 */

#ifndef SANDBOX_SEQ_SANDBOX_H_
#define SANDBOX_SEQ_SANDBOX_H_

#include <stdint.h>
#include "Wave_module/wave_seq_layout.h"

#define SEQ_SANDBOX_FAKE_STEPS   4U

typedef struct {
    uint16_t u16Enable;        /* 1 = stepper runs (default on)            */
    uint16_t u16FireActivate;  /* 1 = each step fires WAVE_ACTIVATE cmd    */
    uint16_t u16CurrentStep;   /* 0..SEQ_SANDBOX_FAKE_STEPS-1              */
    uint16_t u16DwellLeft;     /* remaining ticks in the current step      */
    uint32_t u32StepCount;     /* total steps executed                     */
    uint32_t u32WrapCount;     /* sequence restarts                        */
    uint32_t u32PollCount;
    ST_SEQ_STEP asStep[SEQ_SANDBOX_FAKE_STEPS];   /* fake table            */
} ST_SEQ_SANDBOX;

extern ST_SEQ_SANDBOX g_sSeqSandbox;

void SeqSandbox_Init(void);
void SeqSandbox_Poll(void);

#endif /* SANDBOX_SEQ_SANDBOX_H_ */
