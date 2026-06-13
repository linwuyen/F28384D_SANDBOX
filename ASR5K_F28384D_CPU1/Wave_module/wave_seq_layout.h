/*
 * wave_seq_layout.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 *
 * Product wave/sequence memory layout (系統圖「波形資料」):
 *
 *   WAVE: 4096 points x 16 bits x 256 pages = 16 Mbit (1M words)
 *   SEQ : 35 items x 32 bits x 1000 steps   = 1.12 Mbit (70k words)
 *
 * Both live in the external wave memory behind the Wave Memory Backend
 * (product: EMIF1 CS0 SDRAM @ 0x80000000). Word offsets below are relative
 * to the backend base so the layout is identical on every backend that is
 * large enough (GSRAM fake holds a single page only; the home-board SRAM
 * holds 128 pages witout the SEQ region... see capacity notes).
 *
 * Layout (word offsets, C28x word addressing):
 *   0x000000 .. 0x0FFFFF : wave pages 0..255 (page N at N * 4096)
 *   0x100000 .. 0x1116EF : SEQ table, 1000 steps x ST_SEQ_STEP (70 words)
 *
 * Capacity notes:
 *   - PROD SDRAM 16M words: full layout fits with room to spare.
 *   - HOME SRAM 512K words: 127 wave pages + no SEQ region, or trade
 *     pages for SEQ during home testing (sandbox decision, not core).
 *   - GSRAM fake: 1 page, SEQ table faked in internal RAM by seq_sandbox.
 *
 * This header defines layout only - no storage, no access code. The SEQ
 * runtime (M5-adjacent) consumes it later; seq_sandbox exercises the step
 * record format today.
 */

#ifndef WAVE_SEQ_LAYOUT_H_
#define WAVE_SEQ_LAYOUT_H_

#include <stdint.h>

/* WAVE region */
#define WAVE_LAYOUT_PAGE_LEN_WORDS    4096U
#define WAVE_LAYOUT_PAGE_COUNT_PROD   256U
#define WAVE_LAYOUT_WAVE_REGION_WORDS \
    ((uint32_t)WAVE_LAYOUT_PAGE_LEN_WORDS * WAVE_LAYOUT_PAGE_COUNT_PROD)

/* SEQ region */
#define SEQ_LAYOUT_ITEMS_PER_STEP     35U
#define SEQ_LAYOUT_STEP_COUNT         1000U
#define SEQ_LAYOUT_WORDS_PER_STEP     (SEQ_LAYOUT_ITEMS_PER_STEP * 2U)
#define SEQ_LAYOUT_REGION_BASE_WORDS  WAVE_LAYOUT_WAVE_REGION_WORDS
#define SEQ_LAYOUT_REGION_WORDS       \
    ((uint32_t)SEQ_LAYOUT_WORDS_PER_STEP * SEQ_LAYOUT_STEP_COUNT)

/** @brief One sequence step: 35 x 32-bit items (70 words on C28x). Item
 *  meanings are assigned by the future SEQ runtime spec; the sandbox only
 *  exercises the record geometry. */
typedef struct {
    uint32_t au32Item[SEQ_LAYOUT_ITEMS_PER_STEP];
} ST_SEQ_STEP;

#endif /* WAVE_SEQ_LAYOUT_H_ */
