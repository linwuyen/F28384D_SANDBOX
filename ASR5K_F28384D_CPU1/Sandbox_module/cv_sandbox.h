/*
 * cv_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 *
 * CV (control variable / feedback) source abstraction.
 *
 * Product use: CV feedback comes from the LTC2353 via the SPIC 100kHz chain
 * (and later the power-stage CVAD path). The core never reads hardware
 * directly - it calls CvSandbox_GetValue(), and the source behind it is
 * runtime-selectable:
 *
 *   CV_SOURCE_DEBUG_FAKE   - software triangle wave (no hardware at all)
 *   CV_SOURCE_INTERNAL_ADC - ADCA SOC0 result (DACA->ADCINA4 loopback path
 *                            already exercised by HwVerification)
 *   CV_SOURCE_SPIC_LTC2353 - placeholder: reads g_sMeasDds.i16AdcCh0Raw,
 *                            live once a real LTC2353 is connected
 */

#ifndef SANDBOX_CV_SANDBOX_H_
#define SANDBOX_CV_SANDBOX_H_

#include <stdint.h>

#define CV_SOURCE_DEBUG_FAKE     0U
#define CV_SOURCE_INTERNAL_ADC   1U
#define CV_SOURCE_SPIC_LTC2353   2U   /* placeholder until LTC2353 present */
#define CV_SOURCE_MCBSP_AD7915   3U   /* current-loop CV_AD via MCBSP model */

typedef struct {
    uint16_t u16Source;       /* CV_SOURCE_*, runtime switchable           */
    int16_t  i16Cv;           /* latest CV value (signed)                  */
    uint16_t u16RawAdc;       /* raw value of the selected source          */
    int16_t  i16FakePhase;    /* DEBUG_FAKE triangle state                 */
    uint32_t u32UpdateCount;
    uint32_t u32PollCount;
} ST_CV_SANDBOX;

extern ST_CV_SANDBOX g_sCvSandbox;

void CvSandbox_Init(void);
void CvSandbox_Poll(void);            /* main loop: refresh i16Cv          */
int16_t CvSandbox_GetValue(void);     /* core-facing read                  */

#endif /* SANDBOX_CV_SANDBOX_H_ */
