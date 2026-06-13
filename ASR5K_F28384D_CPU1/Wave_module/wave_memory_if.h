/*
 * wave_memory_if.h
 *
 * Created: 2026-06-11
 *
 * Wave Memory Interface - the ONLY view the DDS runtime has of wave storage.
 *
 * The DDS runtime must not include wave_platform_config.h or
 * wave_memory_backend.h. It only consumes:
 *   - the active wave pointer (NULL until a page is activated)
 *   - the ready flag
 *   - the wave length
 *
 * Whether the pointer targets internal GSRAM, home-board EMIF1 async SRAM
 * (0x00300000) or product EMIF1 SDRAM (0x80000000) is invisible here.
 */

#ifndef WAVE_MEMORY_IF_H_
#define WAVE_MEMORY_IF_H_

#include <stdint.h>

/** @brief Active wave pointer, NULL until a page has been activated. */
const volatile uint16_t * WaveMem_GetActiveWavePtr(void);

/** @brief 1 when a validated page is active and safe to read from the ISR. */
uint16_t WaveMem_IsReady(void);

/** @brief Wave page length in words (4096). */
uint16_t WaveMem_GetWaveLenWords(void);

#endif /* WAVE_MEMORY_IF_H_ */
