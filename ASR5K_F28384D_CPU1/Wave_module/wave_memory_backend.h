/*
 * wave_memory_backend.h
 *
 * Created: 2026-06-11
 *
 * Wave Memory Backend - page management on top of the platform-selected
 * physical storage (GSRAM fake page / Home EMIF1 SRAM / Product EMIF1 SDRAM).
 *
 * Owns: page address calculation, page valid flags, page activation and the
 * active wave pointer. Used by the wave producer path (startup sine build,
 * later the M5 SPIB download path). The DDS runtime only uses
 * wave_memory_if.h.
 */

#ifndef WAVE_MEMORY_BACKEND_H_
#define WAVE_MEMORY_BACKEND_H_

#include <stdint.h>
#include "Wave_module/wave_memory_if.h"
#include "Wave_module/wave_platform_config.h"

//-----------------------------------------------------------------------------
// Backend status structure (Hungarian Notation, watch-friendly)
//-----------------------------------------------------------------------------
typedef struct {
    uint16_t u16Backend;          /* WAVE_BACKEND_* identifier               */
    uint32_t u32BaseWordAddr;     /* physical base (word address), info only */
    uint32_t u32SizeWords;        /* total capacity in words                 */
    uint16_t u16PageCount;        /* total page capacity                     */

    uint16_t u16MemTestDone;      /* 1 after WaveMem_MemTest() ran           */
    uint16_t u16MemTestPass;      /* 1 = pass, 0 = fail                      */
    uint32_t u32MemTestFailAddr;  /* failing word address (valid on fail)    */
    uint16_t u16MemTestExpect;    /* expected pattern at failure             */
    uint16_t u16MemTestRead;      /* read-back value at failure              */

    uint16_t u16PageValidMask;    /* bit N = page N validated (N < 16)       */
    uint16_t u16ActivePageId;     /* currently active page                   */
    uint16_t u16PageChecksum;     /* additive checksum of last validated page*/
    uint16_t u16Ready;            /* 1 = active pointer safe for ISR reads   */

    const volatile uint16_t *pu16ActiveWave; /* active wave pointer          */
} ST_WAVE_MEM;

extern ST_WAVE_MEM g_sWaveMem;

//-----------------------------------------------------------------------------
// Backend API (producer / startup side)
//-----------------------------------------------------------------------------

/** @brief Init backend (runs platform EMIF init when required). Call after
 *         Board_init() and before any other WaveMem_* call. */
void WaveMem_Init(void);

/** @brief Destructive memory test (data bus walk, address bus, page 0
 *         patterns). Run BEFORE writing wave data. Returns 1 on pass. */
uint16_t WaveMem_MemTest(void);

/** @brief Write pointer for a page, NULL if pageId is out of range. */
volatile uint16_t * WaveMem_GetPageWritePtr(uint16_t u16PageId);

/** @brief Verify page content (read-back checksum, non-flat check) and mark
 *         it valid. Returns 1 on success. */
uint16_t WaveMem_ValidatePage(uint16_t u16PageId);

/** @brief Activate a validated page: publishes the active wave pointer
 *         (atomic 32-bit write) then sets ready. Returns 1 on success. */
uint16_t WaveMem_ActivatePage(uint16_t u16PageId);

#endif /* WAVE_MEMORY_BACKEND_H_ */
