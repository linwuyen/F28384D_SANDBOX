/*
 * output_sink.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX_PHASE1)
 *
 * Output Sink abstraction for the DDS sample stream.
 *
 * The 100kHz ISR produces one 16-bit DAC code per cycle and hands it to
 * OutputSink_Write(). Where the code goes is decided by a runtime-switchable
 * sink mask (change g_sOutputSink.u16SinkMask from the debugger, no rebuild):
 *
 *   OUTPUT_SINK_DEBUG_VAR    - update watch variables only
 *   OUTPUT_SINK_INTERNAL_DAC - DACA shadow register (12-bit, code >> 4),
 *                              observable on the DACA pin with a scope
 *   OUTPUT_SINK_AD5543       - load g_u16TxSequenceBuf[2]; DMA CH2 ships it
 *                              to the AD5543 on the next 100kHz SPIC frame.
 *                              This sink ONLY writes the TX word - it never
 *                              touches SPIC or DMA configuration/timing.
 *
 * Multiple sinks may be enabled at once (it is an observability fan-out,
 * not an exclusive route). Core DDS logic does not know any sink exists.
 */

#ifndef SANDBOX_OUTPUT_SINK_H_
#define SANDBOX_OUTPUT_SINK_H_

#include <SPIC_module/meas_dds_module.h>   /* g_u16TxSequenceBuf, driverlib */
#include <stdint.h>

//-----------------------------------------------------------------------------
// Sink mask bits
//-----------------------------------------------------------------------------
#define OUTPUT_SINK_DEBUG_VAR     0x0001U
#define OUTPUT_SINK_INTERNAL_DAC  0x0002U
#define OUTPUT_SINK_AD5543        0x0004U

/* SANDBOX_PHASE1 default: debug variable + internal DACA (no external IC). */
#define OUTPUT_SINK_DEFAULT_MASK  (OUTPUT_SINK_DEBUG_VAR | OUTPUT_SINK_INTERNAL_DAC)

//-----------------------------------------------------------------------------
// Sink status (watch-friendly)
//-----------------------------------------------------------------------------
typedef struct {
    uint16_t u16SinkMask;    /* OUTPUT_SINK_* bits, runtime switchable      */
    uint16_t u16LastCode;    /* DEBUG_VAR_SINK: last DAC code written       */
    uint32_t u32WriteCount;  /* increments every ISR write (hook executed)  */
} ST_OUTPUT_SINK;

extern volatile ST_OUTPUT_SINK g_sOutputSink;

/** @brief Reset counters and apply the default sink mask. */
void OutputSink_Init(void);

/** @brief Deliver one DDS sample to all enabled sinks.
 *  Called from the 100kHz dmaCh1ISR (.TI.ramfunc) - kept inline so no
 *  flash-resident call is made from the RAM ISR. */
static inline void OutputSink_Write(uint16_t u16Code)
{
    g_sOutputSink.u32WriteCount++;

    if ((g_sOutputSink.u16SinkMask & OUTPUT_SINK_DEBUG_VAR) != 0U) {
        g_sOutputSink.u16LastCode = u16Code;
    }
    if ((g_sOutputSink.u16SinkMask & OUTPUT_SINK_INTERNAL_DAC) != 0U) {
        /* DACA is 12-bit; drop the 4 LSBs of the 16-bit DDS code */
        DAC_setShadowValue(DACA_BASE, (uint16_t)(u16Code >> 4));
    }
    if ((g_sOutputSink.u16SinkMask & OUTPUT_SINK_AD5543) != 0U) {
        g_u16TxSequenceBuf[2] = u16Code;
    }
}

#endif /* SANDBOX_OUTPUT_SINK_H_ */
