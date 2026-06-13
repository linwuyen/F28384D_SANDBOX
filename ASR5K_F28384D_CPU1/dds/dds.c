
/*
 * dds.c - Direct Digital Synthesis with Complete State Machine
 *
 * Created on: January 12, 2026
 * Author: Cody Chen
 *
 * FSM States Flow:
 *   1. STOPPED (Enable required) -> DELAY_ON
 *   2. DELAY_ON (counting, can interrupt to STOPPED if Disable)
 *   3. PHASE_ON (find start angle, 0.0~360.0 degrees) -> RUNNING
 *   4. RUNNING (Amp/Offset/Freq with Ramp control) -> DELAY_OFF if Disable
 *   5. DELAY_OFF (counting, can return to RUNNING if Enable)
 *   6. PHASE_OFF (find stop angle, 0.0~360.0 degrees) -> STOPPED
 */

#include "dds_buildTable.h"
#include "dds_common.h"
#include "dds_core.h"
#include "Wave_module/wave_memory_backend.h"
#include <stdint.h>

/** @brief Wave page used as the DDS sine source in this milestone */
#define DDS_WAVE_PAGE_ID 0U

/** @brief Global DDS context instance */
ST_DDS sDDS;

DDS_STAT initDDS(HAL_DDS hal) {
  // Sine table is built into the Wave Memory Backend page (GSRAM fake page,
  // home-board EMIF1 SRAM or product EMIF1 SDRAM - selected by
  // wave_platform_config.h, invisible to this runtime).
  volatile uint16_t *pu16Page = WaveMem_GetPageWritePtr(DDS_WAVE_PAGE_ID);

  if (pu16Page == (volatile uint16_t *)0) {
    return DDS_STATE_ERROR;
  }

  // Build 4096-point table one entry per call (true non-blocking)
  uint16_t bComplete = buildSineTable(pu16Page, &hal->u16TableIndex);

  if (bComplete) {
    // Wave Validation -> Wave Activation -> attach active pointer
    if (WaveMem_ValidatePage(DDS_WAVE_PAGE_ID) == 0U) {
      return DDS_STATE_ERROR;
    }
    if (WaveMem_ActivatePage(DDS_WAVE_PAGE_ID) == 0U) {
      return DDS_STATE_ERROR;
    }
    hal->pu16WaveTable = WaveMem_GetActiveWavePtr();

    // 4096-point table initialization complete
    hal->bTableReady = 1;
    hal->bInitComplete = 1;
    sDDS.fgRecordState = DDS_STATE_INIT_TABLE;
    // Default to STOPPED state instead of RUNNING
    return DDS_STATE_STOPPED;
  }

  // Continue building table (one entry per polling cycle, 4096 cycles total)
  return DDS_STATE_INIT_TABLE;
}

/**
 * @brief Main DDS polling function - Called from timetask.c
 * @note Handles table initialization only
 */
void runDDS(void) {
  switch (sDDS.fgState) {
  case DDS_STATE_IDLE:
  case DDS_STATE_INIT_TABLE:
    sDDS.fgState = initDDS(&sDDS);
    break;
  case DDS_STATE_RUNNING:
  case DDS_STATE_STOPPED:
  case DDS_STATE_DELAY_ON:
  case DDS_STATE_DELAY_OFF:
  case DDS_STATE_PHASE_OFF:
  case DDS_STATE_STARTED:
  case DDS_STATE_AMP_RAMP_DOWN:
    // State machine processing happens in stepDDS() called from ISR
    break;
  default:
    sDDS.fgState = DDS_STATE_ERROR;
    break;
  }
}
