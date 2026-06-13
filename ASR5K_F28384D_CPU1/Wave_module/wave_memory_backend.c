/*
 * wave_memory_backend.c
 *
 * Created: 2026-06-11
 *
 * Wave Memory Backend implementation. All page math is in 16-bit WORDS
 * (C28x native addressing).
 */

#include "Wave_module/wave_memory_backend.h"

#if (WAVE_BACKEND_SELECT == WAVE_BACKEND_HOME_EMIF1_SRAM)
#include "Wave_module/emif1_home_sram.h"
#endif

/* Global backend status instance */
ST_WAVE_MEM g_sWaveMem;

#if (WAVE_BACKEND_SELECT == WAVE_BACKEND_GSRAM_FAKE)
/* Fake page storage in internal GSRAM (RAMGS4 via linker section). */
#ifdef __TI_COMPILER_VERSION__
#pragma DATA_SECTION(s_u16GsramFakePage, ".wave_gsram_page")
#endif
static uint16_t s_u16GsramFakePage[WAVE_PAGE_LEN_WORDS];
#endif

/* Physical base pointer of the wave memory region. */
static volatile uint16_t *s_pu16Base = (volatile uint16_t *)0;

//-----------------------------------------------------------------------------
// Interface seen by the DDS runtime (wave_memory_if.h)
//-----------------------------------------------------------------------------
const volatile uint16_t * WaveMem_GetActiveWavePtr(void)
{
    return g_sWaveMem.pu16ActiveWave;
}

uint16_t WaveMem_IsReady(void)
{
    return g_sWaveMem.u16Ready;
}

uint16_t WaveMem_GetWaveLenWords(void)
{
    return WAVE_PAGE_LEN_WORDS;
}

//-----------------------------------------------------------------------------
// Backend API
//-----------------------------------------------------------------------------
void WaveMem_Init(void)
{
    g_sWaveMem.u16Backend         = WAVE_BACKEND_SELECT;
    g_sWaveMem.u32SizeWords       = WAVE_MEM_SIZE_WORDS;
    g_sWaveMem.u16PageCount       = WAVE_MEM_PAGE_COUNT;
    g_sWaveMem.u16MemTestDone     = 0U;
    g_sWaveMem.u16MemTestPass     = 0U;
    g_sWaveMem.u32MemTestFailAddr = 0UL;
    g_sWaveMem.u16PageValidMask   = 0U;
    g_sWaveMem.u16ActivePageId    = 0U;
    g_sWaveMem.u16PageChecksum    = 0U;
    g_sWaveMem.u16Ready           = 0U;
    g_sWaveMem.pu16ActiveWave     = (const volatile uint16_t *)0;

#if (WAVE_BACKEND_SELECT == WAVE_BACKEND_GSRAM_FAKE)
    s_pu16Base = s_u16GsramFakePage;
    g_sWaveMem.u32BaseWordAddr = (uint32_t)s_u16GsramFakePage;
#else
    s_pu16Base = (volatile uint16_t *)WAVE_MEM_BASE_WORD_ADDR;
    g_sWaveMem.u32BaseWordAddr = WAVE_MEM_BASE_WORD_ADDR;
#endif

#if (WAVE_MEM_NEEDS_EMIF_INIT == 1)
    Emif1HomeSram_Init();
#endif
}

/*
 * Destructive memory test, run before any wave content is written:
 *   1. Data bus walking-1 / walking-0 at the base word.
 *   2. Address bus test: unique value at every power-of-2 word offset.
 *   3. Page 0 region: 0x5555 / 0xAAAA / low-16-of-address patterns.
 */
static uint16_t waveMemFail(volatile uint16_t *pu16Addr,
                            uint16_t u16Expect, uint16_t u16Read)
{
    g_sWaveMem.u32MemTestFailAddr = (uint32_t)pu16Addr;
    g_sWaveMem.u16MemTestExpect   = u16Expect;
    g_sWaveMem.u16MemTestRead     = u16Read;
    g_sWaveMem.u16MemTestPass     = 0U;
    g_sWaveMem.u16MemTestDone     = 1U;
    return 0U;
}

uint16_t WaveMem_MemTest(void)
{
    uint16_t u16Bit;
    uint16_t u16Pattern;
    uint16_t u16Read;
    uint32_t u32Offset;
    uint32_t u32Index;

    /* 1. Data bus walk at base word */
    for (u16Bit = 0U; u16Bit < 16U; u16Bit++) {
        u16Pattern = (uint16_t)(1U << u16Bit);          /* walking 1 */
        s_pu16Base[0] = u16Pattern;
        u16Read = s_pu16Base[0];
        if (u16Read != u16Pattern) {
            return waveMemFail(&s_pu16Base[0], u16Pattern, u16Read);
        }
        u16Pattern = (uint16_t)~u16Pattern;             /* walking 0 */
        s_pu16Base[0] = u16Pattern;
        u16Read = s_pu16Base[0];
        if (u16Read != u16Pattern) {
            return waveMemFail(&s_pu16Base[0], u16Pattern, u16Read);
        }
    }

    /* 2. Address bus test across the full region (power-of-2 word offsets).
     *    Catches stuck/shorted address lines (A0..A18 on the home board). */
    s_pu16Base[0] = 0xA5A5U;
    for (u32Offset = 1UL; u32Offset < WAVE_MEM_SIZE_WORDS; u32Offset <<= 1) {
        s_pu16Base[u32Offset] = (uint16_t)(0x5A00U | (u32Offset & 0xFFU));
    }
    u16Read = s_pu16Base[0];
    if (u16Read != 0xA5A5U) {
        return waveMemFail(&s_pu16Base[0], 0xA5A5U, u16Read);
    }
    for (u32Offset = 1UL; u32Offset < WAVE_MEM_SIZE_WORDS; u32Offset <<= 1) {
        u16Pattern = (uint16_t)(0x5A00U | (u32Offset & 0xFFU));
        u16Read = s_pu16Base[u32Offset];
        if (u16Read != u16Pattern) {
            return waveMemFail(&s_pu16Base[u32Offset], u16Pattern, u16Read);
        }
    }

    /* 3. Page 0 region patterns */
    for (u32Index = 0UL; u32Index < WAVE_PAGE_LEN_WORDS; u32Index++) {
        s_pu16Base[u32Index] = 0x5555U;
    }
    for (u32Index = 0UL; u32Index < WAVE_PAGE_LEN_WORDS; u32Index++) {
        u16Read = s_pu16Base[u32Index];
        if (u16Read != 0x5555U) {
            return waveMemFail(&s_pu16Base[u32Index], 0x5555U, u16Read);
        }
    }
    for (u32Index = 0UL; u32Index < WAVE_PAGE_LEN_WORDS; u32Index++) {
        s_pu16Base[u32Index] = 0xAAAAU;
    }
    for (u32Index = 0UL; u32Index < WAVE_PAGE_LEN_WORDS; u32Index++) {
        u16Read = s_pu16Base[u32Index];
        if (u16Read != 0xAAAAU) {
            return waveMemFail(&s_pu16Base[u32Index], 0xAAAAU, u16Read);
        }
    }
    for (u32Index = 0UL; u32Index < WAVE_PAGE_LEN_WORDS; u32Index++) {
        s_pu16Base[u32Index] = (uint16_t)u32Index;
    }
    for (u32Index = 0UL; u32Index < WAVE_PAGE_LEN_WORDS; u32Index++) {
        u16Read = s_pu16Base[u32Index];
        if (u16Read != (uint16_t)u32Index) {
            return waveMemFail(&s_pu16Base[u32Index],
                               (uint16_t)u32Index, u16Read);
        }
    }

    g_sWaveMem.u16MemTestPass = 1U;
    g_sWaveMem.u16MemTestDone = 1U;
    return 1U;
}

volatile uint16_t * WaveMem_GetPageWritePtr(uint16_t u16PageId)
{
    if (u16PageId >= g_sWaveMem.u16PageCount) {
        return (volatile uint16_t *)0;
    }
    return &s_pu16Base[(uint32_t)u16PageId * WAVE_PAGE_LEN_WORDS];
}

uint16_t WaveMem_ValidatePage(uint16_t u16PageId)
{
    const volatile uint16_t *pu16Page;
    uint16_t u16Checksum = 0U;
    uint16_t u16Min = 0xFFFFU;
    uint16_t u16Max = 0U;
    uint16_t u16Value;
    uint32_t u32Index;

    pu16Page = WaveMem_GetPageWritePtr(u16PageId);
    if (pu16Page == (const volatile uint16_t *)0) {
        return 0U;
    }

    for (u32Index = 0UL; u32Index < WAVE_PAGE_LEN_WORDS; u32Index++) {
        u16Value = pu16Page[u32Index];
        u16Checksum += u16Value;                 /* additive, wraps on 16 bit */
        if (u16Value < u16Min) { u16Min = u16Value; }
        if (u16Value > u16Max) { u16Max = u16Value; }
    }

    /* Reject flat content: a real wave page must have amplitude variation. */
    if (u16Min == u16Max) {
        return 0U;
    }

    g_sWaveMem.u16PageChecksum = u16Checksum;
    if (u16PageId < WAVE_PAGE_TRACK_MAX) {
        g_sWaveMem.u16PageValidMask |= (uint16_t)(1U << u16PageId);
    }
    return 1U;
}

uint16_t WaveMem_ActivatePage(uint16_t u16PageId)
{
    if (u16PageId >= g_sWaveMem.u16PageCount) {
        return 0U;
    }
    if (u16PageId < WAVE_PAGE_TRACK_MAX) {
        if ((g_sWaveMem.u16PageValidMask & (uint16_t)(1U << u16PageId)) == 0U) {
            return 0U;   /* page not validated */
        }
    }

    /* Publish pointer first (single 32-bit write, atomic on C28x),
     * then raise ready so the 100kHz ISR never sees a half state. */
    g_sWaveMem.pu16ActiveWave =
        &s_pu16Base[(uint32_t)u16PageId * WAVE_PAGE_LEN_WORDS];
    g_sWaveMem.u16ActivePageId = u16PageId;
    g_sWaveMem.u16Ready = 1U;
    return 1U;
}
