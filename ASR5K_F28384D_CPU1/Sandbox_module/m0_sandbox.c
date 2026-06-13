/*
 * m0_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 * Updated: 2026-06-12 - fake plant behavior for fans / ADC / protections.
 */

#include "Sandbox_module/m0_sandbox.h"

ST_M0_SANDBOX g_sM0Sandbox;

static uint32_t s_u32PollDivider = 0UL;
static int16_t  s_i16IlPhase     = 0;

/* Read-only registers (writes rejected, like the real device would NACK) */
static uint16_t m0RegIsReadOnly(uint16_t u16Reg)
{
    switch (u16Reg) {
    case M0_REG_WHOAMI:
    case M0_REG_GROUP_IN:
    case M0_REG_ILA_AD:
    case M0_REG_ILB_AD:
    case M0_REG_TEMP:
    case M0_REG_FAN1_CAP:
    case M0_REG_FAN2_CAP:
    case M0_REG_FAN3_CAP:
    case M0_REG_FAN4_CAP:
    case M0_REG_FANFAIL:
    case M0_REG_HEARTBEAT:
        return 1U;
    default:
        return 0U;
    }
}

void M0Sandbox_Init(void)
{
    uint16_t u16I;

    g_sM0Sandbox.u16Mode        = M0_SANDBOX_MODE_FAKE;
    g_sM0Sandbox.u16LastReg     = 0U;
    g_sM0Sandbox.u16LastData    = 0U;
    g_sM0Sandbox.u32XferCount   = 0UL;
    g_sM0Sandbox.u32NackCount   = 0UL;
    g_sM0Sandbox.u32RejectCount = 0UL;
    for (u16I = 0U; u16I < M0_SANDBOX_REG_COUNT; u16I++) {
        g_sM0Sandbox.au16Reg[u16I] = 0U;
    }
    g_sM0Sandbox.au16Reg[M0_REG_WHOAMI] = M0_SANDBOX_WHOAMI;
    g_sM0Sandbox.au16Reg[M0_REG_TEMP]   = 250U;   /* fake 25.0 C            */
    g_sM0Sandbox.au16Reg[M0_REG_IPK_POS] = 0x0FFFU; /* Ipeak limits wide open */
    g_sM0Sandbox.au16Reg[M0_REG_IPK_NEG] = 0x0FFFU;

    /* Transaction layer (D02_2_3) */
    g_sM0Xfer.u16State      = M0_XFER_IDLE;
    g_sM0Xfer.u16IsRead     = 0U;
    g_sM0Xfer.u16MemAddr    = 0U;
    g_sM0Xfer.u16Len        = 0U;
    g_sM0Xfer.u16Index      = 0U;
    g_sM0Xfer.u16MasterPec  = 0U;
    g_sM0Xfer.u16SlavePec   = 0U;
    g_sM0Xfer.u16Result     = 0U;
    g_sM0Xfer.u32OkCount    = 0UL;
    g_sM0Xfer.u32PecErrCount = 0UL;

    s_u32PollDivider = 0UL;
    s_i16IlPhase     = 0;
}

uint16_t M0Sandbox_ReadReg(uint16_t u16Reg)
{
    if (u16Reg >= M0_SANDBOX_REG_COUNT) {
        g_sM0Sandbox.u32RejectCount++;
        return 0xFFFFU;
    }
    g_sM0Sandbox.u16LastReg = u16Reg;
    g_sM0Sandbox.u32XferCount++;
    return g_sM0Sandbox.au16Reg[u16Reg];
}

uint16_t M0Sandbox_WriteReg(uint16_t u16Reg, uint16_t u16Data)
{
    if ((u16Reg >= M0_SANDBOX_REG_COUNT) || (m0RegIsReadOnly(u16Reg) != 0U)) {
        g_sM0Sandbox.u32RejectCount++;
        return 0U;
    }
    /* DAC7612 channels are 12-bit */
    if ((u16Reg == M0_REG_IPK_POS) || (u16Reg == M0_REG_IPK_NEG)) {
        u16Data &= 0x0FFFU;
    }
    g_sM0Sandbox.au16Reg[u16Reg] = u16Data;
    g_sM0Sandbox.u16LastReg  = u16Reg;
    g_sM0Sandbox.u16LastData = u16Data;
    g_sM0Sandbox.u32XferCount++;
    return 1U;
}

//-----------------------------------------------------------------------------
// D02_2_3 transaction layer
//-----------------------------------------------------------------------------
ST_M0_XFER g_sM0Xfer;

/* SMBus PEC table - CRC-8 poly 0x07, init 0x00 (verbatim from D02_2_3) */
static const uint16_t s_au8PecTable[256] = {
    0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,
    0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,
    0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,
    0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,
    0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,
    0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,
    0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,
    0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,
    0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,
    0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,
    0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,
    0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,
    0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,
    0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,
    0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
    0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3
};

static inline uint16_t m0PecStep(uint16_t u16Crc, uint16_t u16Byte)
{
    return s_au8PecTable[(u16Crc ^ u16Byte) & 0x00FFU];
}

/* Fake-M0 byte access: virtual mem addr = byte address into the register
 * file (reg N = byte 2N high, 2N+1 low). Out of range reads 0xFF. */
static uint16_t m0FakeReadByte(uint16_t u16ByteAddr)
{
    uint16_t u16Reg = (uint16_t)(u16ByteAddr >> 1);

    if (u16Reg >= M0_SANDBOX_REG_COUNT) {
        return 0xFFU;   /* per D02_2_3: fake data, PEC still valid */
    }
    return ((u16ByteAddr & 1U) == 0U)
        ? (uint16_t)(g_sM0Sandbox.au16Reg[u16Reg] >> 8)
        : (uint16_t)(g_sM0Sandbox.au16Reg[u16Reg] & 0x00FFU);
}

static void m0FakeWriteByte(uint16_t u16ByteAddr, uint16_t u16Byte)
{
    uint16_t u16Reg = (uint16_t)(u16ByteAddr >> 1);
    uint16_t u16Val;

    if (u16Reg >= M0_SANDBOX_REG_COUNT) {
        return;          /* silently dropped, like the real device */
    }
    u16Val = g_sM0Sandbox.au16Reg[u16Reg];
    if ((u16ByteAddr & 1U) == 0U) {
        u16Val = (uint16_t)((u16Val & 0x00FFU) | (u16Byte << 8));
    } else {
        u16Val = (uint16_t)((u16Val & 0xFF00U) | (u16Byte & 0x00FFU));
    }
    (void)M0Sandbox_WriteReg(u16Reg, u16Val);   /* RO rules still apply */
}

static uint16_t m0XferStart(uint16_t u16MemAddr, uint16_t u16Len,
                            uint16_t u16IsRead)
{
    uint16_t u16AddrH;
    uint16_t u16AddrL;

    if ((g_sM0Xfer.u16State != M0_XFER_IDLE) &&
        (g_sM0Xfer.u16State != M0_XFER_DONE)) {
        return 0U;
    }
    if ((u16Len == 0U) || (u16Len > M0_XFER_DATA_MAX) ||
        (u16MemAddr > 0x03FFU)) {
        return 0U;
    }

    /* Virtual address: ADDR_H = mem[9:2], ADDR_L = mem[1:0]<<6 | len */
    u16AddrH = (uint16_t)((u16MemAddr >> 2) & 0x00FFU);
    u16AddrL = (uint16_t)(((u16MemAddr & 0x0003U) << 6) | (u16Len & 0x3FU));

    g_sM0Xfer.u16IsRead  = u16IsRead;
    g_sM0Xfer.u16MemAddr = u16MemAddr;
    g_sM0Xfer.u16Len     = u16Len;
    g_sM0Xfer.u16Index   = 0U;

    /* PEC scope starts at Slave_W and covers ADDR_H/L (+ Slave_R on read);
     * both sides run it - master computes, fake slave verifies. */
    g_sM0Xfer.u16MasterPec = m0PecStep(0x00U, (M0_I2C_SLAVE_ADDR7 << 1));
    g_sM0Xfer.u16MasterPec = m0PecStep(g_sM0Xfer.u16MasterPec, u16AddrH);
    g_sM0Xfer.u16MasterPec = m0PecStep(g_sM0Xfer.u16MasterPec, u16AddrL);
    if (u16IsRead != 0U) {
        g_sM0Xfer.u16MasterPec = m0PecStep(g_sM0Xfer.u16MasterPec,
                                  (uint16_t)((M0_I2C_SLAVE_ADDR7 << 1) | 1U));
    }
    g_sM0Xfer.u16SlavePec = g_sM0Xfer.u16MasterPec;   /* same bus bytes */

    g_sM0Xfer.u16State = M0_XFER_DATA;
    return 1U;
}

uint16_t M0Sandbox_RequestWrite(uint16_t u16MemAddr, const uint16_t *pu8Data,
                                uint16_t u16Len)
{
    uint16_t u16I;

    if (m0XferStart(u16MemAddr, u16Len, 0U) == 0U) {
        return 0U;
    }
    for (u16I = 0U; u16I < u16Len; u16I++) {
        g_sM0Xfer.au8Data[u16I] = (uint16_t)(pu8Data[u16I] & 0x00FFU);
    }
    return 1U;
}

uint16_t M0Sandbox_RequestRead(uint16_t u16MemAddr, uint16_t u16Len)
{
    return m0XferStart(u16MemAddr, u16Len, 1U);
}

/* One byte per call: the non-blocking polling FSM from D02_2_3. In REAL
 * mode this body becomes I2CA register handling; the API is unchanged. */
void M0Sandbox_XferPoll(void)
{
    uint16_t u16Byte;

    switch (g_sM0Xfer.u16State) {
    case M0_XFER_DATA:
        if (g_sM0Xfer.u16IsRead != 0U) {
            /* Slave drives the bus: fake M0 sources the byte */
            u16Byte = m0FakeReadByte(
                (uint16_t)(g_sM0Xfer.u16MemAddr + g_sM0Xfer.u16Index));
            g_sM0Xfer.au8Data[g_sM0Xfer.u16Index] = u16Byte;
            g_sM0Xfer.u16SlavePec  = m0PecStep(g_sM0Xfer.u16SlavePec, u16Byte);
            g_sM0Xfer.u16MasterPec = m0PecStep(g_sM0Xfer.u16MasterPec, u16Byte);
        } else {
            /* Master drives: fake M0 absorbs the byte */
            u16Byte = g_sM0Xfer.au8Data[g_sM0Xfer.u16Index];
            g_sM0Xfer.u16MasterPec = m0PecStep(g_sM0Xfer.u16MasterPec, u16Byte);
            g_sM0Xfer.u16SlavePec  = m0PecStep(g_sM0Xfer.u16SlavePec, u16Byte);
            m0FakeWriteByte(
                (uint16_t)(g_sM0Xfer.u16MemAddr + g_sM0Xfer.u16Index), u16Byte);
        }
        g_sM0Xfer.u16Index++;
        if (g_sM0Xfer.u16Index >= g_sM0Xfer.u16Len) {
            g_sM0Xfer.u16State = M0_XFER_PEC;
        }
        g_sM0Sandbox.u32XferCount++;
        break;

    case M0_XFER_PEC:
        /* Wire carries the sender's PEC; receiver verifies. In FAKE mode
         * both ran over identical bytes, so a mismatch means the model
         * itself broke - counted, expected 0. */
        if (g_sM0Xfer.u16MasterPec == g_sM0Xfer.u16SlavePec) {
            g_sM0Xfer.u16Result = 1U;
            g_sM0Xfer.u32OkCount++;
        } else {
            g_sM0Xfer.u16Result = 0U;
            g_sM0Xfer.u32PecErrCount++;
            g_sM0Sandbox.u32NackCount++;
        }
        g_sM0Xfer.u16State = M0_XFER_DONE;
        break;

    case M0_XFER_DONE:
        g_sM0Xfer.u16State = M0_XFER_IDLE;
        break;

    case M0_XFER_IDLE:
    default:
        break;
    }
}

/*
 * Fake plant (every 4096 polls) + protocol self-exercise:
 *   - fan tach feedback tracks the commanded duty (no FANFAIL)
 *   - ILA/ILB wander as a slow triangle so the watch shows live current
 *   - protections stay clear (fault injection via debugger on GROUP_IN)
 *   - heartbeat increments
 *   - WHOAMI is re-read through the FULL virtual-address + PEC path so
 *     the D02_2_3 protocol machinery is continuously proven
 */
void M0Sandbox_Poll(void)
{
    uint16_t u16Fan;

    M0Sandbox_XferPoll();   /* step the byte FSM every pass */

    s_u32PollDivider++;
    if ((s_u32PollDivider & 0x0FFFUL) != 0UL) {
        return;
    }
    if (g_sM0Sandbox.u16Mode != M0_SANDBOX_MODE_FAKE) {
        return;   /* REAL mode: plant is the real M0 */
    }

    for (u16Fan = 0U; u16Fan < 4U; u16Fan++) {
        g_sM0Sandbox.au16Reg[M0_REG_FAN1_CAP + u16Fan] =
            g_sM0Sandbox.au16Reg[M0_REG_FAN1_PWM + u16Fan];
    }

    s_i16IlPhase += 64;
    if (s_i16IlPhase > 4000) {
        s_i16IlPhase = -4000;
    }
    g_sM0Sandbox.au16Reg[M0_REG_ILA_AD] = (uint16_t)(2048 + (s_i16IlPhase >> 1));
    g_sM0Sandbox.au16Reg[M0_REG_ILB_AD] = (uint16_t)(2048 - (s_i16IlPhase >> 1));

    g_sM0Sandbox.au16Reg[M0_REG_HEARTBEAT]++;

    /* Protocol self-exercise: WHOAMI readback (reg0 = byte addr 0, 2 bytes) */
    (void)M0Sandbox_RequestRead(0U, 2U);
}
