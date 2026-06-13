/*
 * flash_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 *
 * SPIA external flash (W25Q64) fake backend.
 *
 * Product use: W25Q64 on SPIA carries boot / OTA / maintenance data only
 * (runtime never touches it; DMA CH5/CH6 are reserved for this path and are
 * NOT used here). The fake backend emulates a small sector with NOR flash
 * semantics so the access layer can be exercised without the chip:
 *
 *   - erase sets all words to 0xFFFF
 *   - write can only clear bits (new = old & data), like real NOR
 *   - read returns the array content
 *
 *   FLASH_SANDBOX_MODE_FAKE - RAM-backed emulation (default)
 *   FLASH_SANDBOX_MODE_REAL - placeholder: routes to the existing SPIA
 *                             W25Q command path (HwVerification_FLASH_*),
 *                             wired when a real chip is mounted
 */

#ifndef SANDBOX_FLASH_SANDBOX_H_
#define SANDBOX_FLASH_SANDBOX_H_

#include <stdint.h>

#define FLASH_SANDBOX_MODE_FAKE   0U
#define FLASH_SANDBOX_MODE_REAL   1U   /* placeholder */

#define FLASH_SANDBOX_SIZE_WORDS  64U  /* emulated sector size */
#define FLASH_SANDBOX_FAKE_ID     0x00EF4017UL  /* W25Q64 JEDEC id */

typedef struct {
    uint16_t u16Mode;          /* FLASH_SANDBOX_MODE_*                     */
    uint32_t u32JedecId;       /* fake id, matches real W25Q64             */
    uint16_t u16LastAddr;
    uint16_t u16LastData;
    uint32_t u32ReadCount;
    uint32_t u32WriteCount;
    uint32_t u32EraseCount;
    uint32_t u32RejectCount;   /* out-of-range / mode-not-ready accesses   */
    uint16_t au16Mem[FLASH_SANDBOX_SIZE_WORDS];  /* emulated sector        */
} ST_FLASH_SANDBOX;

extern ST_FLASH_SANDBOX g_sFlashSandbox;

void     FlashSandbox_Init(void);
uint32_t FlashSandbox_ReadId(void);
uint16_t FlashSandbox_Read(uint16_t u16Addr);
uint16_t FlashSandbox_Write(uint16_t u16Addr, uint16_t u16Data); /* 1=ok */
void     FlashSandbox_EraseSector(void);
void     FlashSandbox_Poll(void);   /* self-test pattern (slow)            */

#endif /* SANDBOX_FLASH_SANDBOX_H_ */
