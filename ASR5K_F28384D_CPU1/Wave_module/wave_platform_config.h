/*
 * wave_platform_config.h
 *
 * Created: 2026-06-11
 *
 * Platform selection for the Wave Memory Backend.
 *
 * This is the ONLY file allowed to contain physical wave memory addresses.
 * The DDS runtime never sees these constants; it only consumes the active
 * wave pointer through wave_memory_if.h.
 *
 * To port between boards, change WAVE_BACKEND_SELECT below. Nothing else
 * in the DDS runtime or SPIC/AD5543 path needs to change.
 *
 * NOTE (C28x addressing): all base addresses and sizes below are in 16-bit
 * WORDS, not bytes. On C28x sizeof(uint16_t) == 1 and pointers are word
 * addresses. "1MB SRAM" therefore means 0x80000 words.
 */

#ifndef WAVE_PLATFORM_CONFIG_H_
#define WAVE_PLATFORM_CONFIG_H_

//-----------------------------------------------------------------------------
// Backend identifiers
//-----------------------------------------------------------------------------
#define WAVE_BACKEND_GSRAM_FAKE         0U  /* Internal GSRAM fake page (debug) */
#define WAVE_BACKEND_HOME_EMIF1_SRAM    1U  /* Home board IS61LV51216 async SRAM */
#define WAVE_BACKEND_PROD_EMIF1_SDRAM   2U  /* Product board IS42S16160J SDRAM   */

//-----------------------------------------------------------------------------
// [PLATFORM SELECTION] - change this single line to port
//
// SANDBOX_PHASE1 default: GSRAM_FAKE (no external hardware required).
// Switch to HOME_EMIF1_SRAM for the home-board external SRAM bring-up,
// PROD_EMIF1_SDRAM on the product board.
//-----------------------------------------------------------------------------
#ifndef WAVE_BACKEND_SELECT
#define WAVE_BACKEND_SELECT             WAVE_BACKEND_GSRAM_FAKE
#endif

//-----------------------------------------------------------------------------
// Common wave page geometry
//-----------------------------------------------------------------------------
#define WAVE_PAGE_LEN_WORDS             4096U   /* 4096-point wave page        */
#define WAVE_PAGE_TRACK_MAX             16U     /* pages tracked by valid mask */

//-----------------------------------------------------------------------------
// Per-backend physical layout
//-----------------------------------------------------------------------------
#if (WAVE_BACKEND_SELECT == WAVE_BACKEND_GSRAM_FAKE)

  /* Base address resolved at runtime from the internal GSRAM array. */
  #define WAVE_MEM_SIZE_WORDS           ((uint32_t)WAVE_PAGE_LEN_WORDS)
  #define WAVE_MEM_PAGE_COUNT           1U
  #define WAVE_MEM_NEEDS_EMIF_INIT      0

#elif (WAVE_BACKEND_SELECT == WAVE_BACKEND_HOME_EMIF1_SRAM)

  /* IS61LV51216 (512K x 16) on EMIF1 CS3n. CS3 window is exactly 512K words. */
  #define WAVE_MEM_BASE_WORD_ADDR       0x00300000UL
  #define WAVE_MEM_SIZE_WORDS           0x00080000UL  /* 512K words = 1MB */
  #define WAVE_MEM_PAGE_COUNT           ((uint16_t)(WAVE_MEM_SIZE_WORDS / WAVE_PAGE_LEN_WORDS))
  #define WAVE_MEM_NEEDS_EMIF_INIT      1

#elif (WAVE_BACKEND_SELECT == WAVE_BACKEND_PROD_EMIF1_SDRAM)

  /* IS42S16160J (16M x 16) on EMIF1 CS0 (SDRAM). SDRAM controller init is
   * provided by SysConfig Board_init(); refresh/timing validation is a
   * separate future task and is NOT covered by the home-board bring-up. */
  #define WAVE_MEM_BASE_WORD_ADDR       0x80000000UL
  #define WAVE_MEM_SIZE_WORDS           0x01000000UL  /* 16M words = 32MB */
  #define WAVE_MEM_PAGE_COUNT           ((uint16_t)(WAVE_MEM_SIZE_WORDS / WAVE_PAGE_LEN_WORDS))
  #define WAVE_MEM_NEEDS_EMIF_INIT      0   /* Board_init() owns SDRAM setup */

#else
  #error "WAVE_BACKEND_SELECT: unknown wave memory backend"
#endif

#endif /* WAVE_PLATFORM_CONFIG_H_ */
