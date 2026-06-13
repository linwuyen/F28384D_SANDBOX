/*
 * meas_dds_module.h
 *
 * Created: 2026-05-18
 *
 * Hardware Abstraction Layer for:
 *   - LTC2353-16 Dual-Channel 16-bit SAR ADC
 *   - AD5543 16-bit DAC
 *
 * Hardware Mapping (Product board):
 *   - SPI peripheral : SPIC (GPIO 100~103), mapped as SPIC_MEAS_DDS_BASE
 *   - EN_SDI         : GPIO 4 (Output) - Used only on Dev board simulation
 */

#ifndef MEAS_DDS_MODULE_H_
#define MEAS_DDS_MODULE_H_

#include "driverlib.h"
#include "device.h"
#include "board.h"

//=============================================================================
// [TEST MODE CONFIGURATION]
//=============================================================================
//#define MEAS_DDS_USE_SPIA_GPIO
//#define SPI_LOOPBACK_TEST

//=============================================================================
// [AUTO-DERIVED]
//=============================================================================
#ifdef MEAS_DDS_USE_SPIA_GPIO
  #define MEAS_DDS_SPI_BASE   SPIA_EXTFLASH_BASE  /* Dev board: SPIA GPIO16~19  */
#else
  #define MEAS_DDS_SPI_BASE   SPIC_MEAS_DDS_BASE  /* Production: SPIC GPIO100~103 */
#endif

//-----------------------------------------------------------------------------
// GPIO pin definitions
//-----------------------------------------------------------------------------
#define MEAS_DDS_EN_SDI_GPIO     4U

#define LTC2353_CONFIG_TX        0xFC00U
#define LTC2353_SCALE_V          (10.24f / 32768.0f)
#define AD5543_HALF_VALUE        32768U
#define AD5543_MAX_VALUE         65535U

//-----------------------------------------------------------------------------
// SPI protection constants
//-----------------------------------------------------------------------------
#define SPI_RX_POLL_MAX          500U   /* Max poll count ~2.5 us at 200 MHz */
#define SPI_TIMEOUT_FAULT_THRESHOLD 10U /* Consecutive timeouts before EV_ERROR */

//-----------------------------------------------------------------------------
// Data structure aligned with Hungarian Notation Coding Style
//-----------------------------------------------------------------------------
typedef struct {
    int16_t  i16AdcCh0Raw;       /* LTC2353 CH0 raw ADC value              */
    int16_t  i16AdcCh1Raw;       /* LTC2353 CH1 raw ADC value              */
    uint16_t u16AdcCh0Id;        /* CH0 channel ID from status bits        */
    uint16_t u16AdcCh0Ss;        /* CH0 SoftSpan from status bits          */
    uint16_t u16AdcCh1Id;        /* CH1 channel ID from status bits        */
    uint16_t u16AdcCh1Ss;        /* CH1 SoftSpan from status bits          */
    float32_t f32AdcCh0V;        /* CH0 voltage in V                       */
    float32_t f32AdcCh1V;        /* CH1 voltage in V                       */
    uint16_t u16DacOut;          /* DAC output word, written by ISR        */
    uint16_t u16InitDone;        /* Module init complete flag              */
    uint16_t u16IsrCounter;      /* ISR execution counter                  */
    uint16_t u16SpiTimeoutCnt;   /* SPI RX poll timeout counter            */
} ST_MEAS_DDS;

//-----------------------------------------------------------------------------
// SPIC TX/RX buffers
//-----------------------------------------------------------------------------
extern volatile uint16_t g_u16TxSequenceBuf[3];
extern volatile uint16_t g_u16SpiRxBuf[3];

//-----------------------------------------------------------------------------
// External instance and API
//-----------------------------------------------------------------------------
extern ST_MEAS_DDS g_sMeasDds;

void initMeasDds(void);
__interrupt void dmaCh1ISR(void);

#endif /* MEAS_DDS_MODULE_H_ */
