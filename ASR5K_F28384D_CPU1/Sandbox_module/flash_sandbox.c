/*
 * flash_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 */

#include "Sandbox_module/flash_sandbox.h"

ST_FLASH_SANDBOX g_sFlashSandbox;

static uint32_t s_u32PollDivider = 0UL;
static uint16_t s_u16SelfTestStep = 0U;

void FlashSandbox_Init(void)
{
    uint16_t u16I;

    g_sFlashSandbox.u16Mode        = FLASH_SANDBOX_MODE_FAKE;
    g_sFlashSandbox.u32JedecId     = FLASH_SANDBOX_FAKE_ID;
    g_sFlashSandbox.u16LastAddr    = 0U;
    g_sFlashSandbox.u16LastData    = 0U;
    g_sFlashSandbox.u32ReadCount   = 0UL;
    g_sFlashSandbox.u32WriteCount  = 0UL;
    g_sFlashSandbox.u32EraseCount  = 0UL;
    g_sFlashSandbox.u32RejectCount = 0UL;
    for (u16I = 0U; u16I < FLASH_SANDBOX_SIZE_WORDS; u16I++) {
        g_sFlashSandbox.au16Mem[u16I] = 0xFFFFU;   /* erased state */
    }
    s_u32PollDivider  = 0UL;
    s_u16SelfTestStep = 0U;
}

uint32_t FlashSandbox_ReadId(void)
{
    /* REAL mode placeholder would issue 0x9F over SPIA instead */
    return g_sFlashSandbox.u32JedecId;
}

uint16_t FlashSandbox_Read(uint16_t u16Addr)
{
    if (u16Addr >= FLASH_SANDBOX_SIZE_WORDS) {
        g_sFlashSandbox.u32RejectCount++;
        return 0xFFFFU;
    }
    g_sFlashSandbox.u16LastAddr = u16Addr;
    g_sFlashSandbox.u32ReadCount++;
    return g_sFlashSandbox.au16Mem[u16Addr];
}

uint16_t FlashSandbox_Write(uint16_t u16Addr, uint16_t u16Data)
{
    if ((u16Addr >= FLASH_SANDBOX_SIZE_WORDS) ||
        (g_sFlashSandbox.u16Mode != FLASH_SANDBOX_MODE_FAKE)) {
        g_sFlashSandbox.u32RejectCount++;
        return 0U;
    }
    /* NOR semantics: programming only clears bits */
    g_sFlashSandbox.au16Mem[u16Addr] &= u16Data;
    g_sFlashSandbox.u16LastAddr = u16Addr;
    g_sFlashSandbox.u16LastData = u16Data;
    g_sFlashSandbox.u32WriteCount++;
    return 1U;
}

void FlashSandbox_EraseSector(void)
{
    uint16_t u16I;

    for (u16I = 0U; u16I < FLASH_SANDBOX_SIZE_WORDS; u16I++) {
        g_sFlashSandbox.au16Mem[u16I] = 0xFFFFU;
    }
    g_sFlashSandbox.u32EraseCount++;
}

/*
 * Slow self-test so the watch window shows the backend living:
 * erase -> write pattern -> read-verify, one step every 4096 polls.
 */
void FlashSandbox_Poll(void)
{
    s_u32PollDivider++;
    if ((s_u32PollDivider & 0x0FFFUL) != 0UL) {
        return;
    }

    switch (s_u16SelfTestStep) {
    case 0U:
        FlashSandbox_EraseSector();
        s_u16SelfTestStep = 1U;
        break;
    case 1U:
        (void)FlashSandbox_Write(0U, 0x55AAU);
        s_u16SelfTestStep = 2U;
        break;
    case 2U:
        if (FlashSandbox_Read(0U) != 0x55AAU) {
            g_sFlashSandbox.u32RejectCount++;
        }
        s_u16SelfTestStep = 0U;
        break;
    default:
        s_u16SelfTestStep = 0U;
        break;
    }
}
