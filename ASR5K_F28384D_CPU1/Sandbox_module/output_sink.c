/*
 * output_sink.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX_PHASE1)
 */

#include "Sandbox_module/output_sink.h"

/* Global sink state. volatile: u16SinkMask is poked from the debugger. */
volatile ST_OUTPUT_SINK g_sOutputSink = {
    OUTPUT_SINK_DEFAULT_MASK, 0U, 0UL
};

void OutputSink_Init(void)
{
    g_sOutputSink.u16SinkMask   = OUTPUT_SINK_DEFAULT_MASK;
    g_sOutputSink.u16LastCode   = 0U;
    g_sOutputSink.u32WriteCount = 0UL;
}
