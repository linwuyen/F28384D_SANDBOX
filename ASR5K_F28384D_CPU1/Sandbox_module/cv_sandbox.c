/*
 * cv_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 */

#include "Sandbox_module/cv_sandbox.h"
#include "Sandbox_module/mcbsp_sandbox.h"
#include <SPIC_module/meas_dds_module.h>   /* g_sMeasDds + driverlib */

ST_CV_SANDBOX g_sCvSandbox;

void CvSandbox_Init(void)
{
    g_sCvSandbox.u16Source      = CV_SOURCE_DEBUG_FAKE;
    g_sCvSandbox.i16Cv          = 0;
    g_sCvSandbox.u16RawAdc      = 0U;
    g_sCvSandbox.i16FakePhase   = 0;
    g_sCvSandbox.u32UpdateCount = 0UL;
    g_sCvSandbox.u32PollCount   = 0UL;
}

void CvSandbox_Poll(void)
{
    g_sCvSandbox.u32PollCount++;

    switch (g_sCvSandbox.u16Source) {
    case CV_SOURCE_DEBUG_FAKE:
        /* Software triangle: +/-16000 swing so the watch visibly moves */
        g_sCvSandbox.i16FakePhase += 16;
        if (g_sCvSandbox.i16FakePhase > 16000) {
            g_sCvSandbox.i16FakePhase = -16000;
        }
        g_sCvSandbox.i16Cv     = g_sCvSandbox.i16FakePhase;
        g_sCvSandbox.u16RawAdc = (uint16_t)g_sCvSandbox.i16FakePhase;
        break;

    case CV_SOURCE_INTERNAL_ADC:
        /* ADCA SOC0 (DACA->ADCINA4 loopback, SOC forced by HwVerification
         * every 2.5ms). 12-bit result, centered to signed. */
        g_sCvSandbox.u16RawAdc = ADC_readResult(ADCARESULT_BASE,
                                                ADC_SOC_NUMBER0);
        g_sCvSandbox.i16Cv = (int16_t)(g_sCvSandbox.u16RawAdc) - 2048;
        break;

    case CV_SOURCE_SPIC_LTC2353:
        /* Placeholder: field is wired and updates at 100kHz once a real
         * LTC2353 drives the SPIC RX path. */
        g_sCvSandbox.u16RawAdc = (uint16_t)g_sMeasDds.i16AdcCh0Raw;
        g_sCvSandbox.i16Cv     = g_sMeasDds.i16AdcCh0Raw;
        break;

    case CV_SOURCE_MCBSP_AD7915:
        /* Current-loop CV: AD7915 sample via the MCBSP service (fake plant
         * today, real converter frames after bring-up - same call). */
        g_sCvSandbox.u16RawAdc = McbspSandbox_ReadCvAd();
        g_sCvSandbox.i16Cv     = (int16_t)(g_sCvSandbox.u16RawAdc - 32768U);
        break;

    default:
        return;
    }

    g_sCvSandbox.u32UpdateCount++;
}

int16_t CvSandbox_GetValue(void)
{
    return g_sCvSandbox.i16Cv;
}
