/*
 * am3352_sandbox.c
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 * Updated: 2026-06-13 - D02_2_1 packet layer (CRC16 + ping-pong status).
 */

#include "Sandbox_module/am3352_sandbox.h"
#include "Sandbox_module/sandbox_cmd.h"
#include "Sandbox_module/output_sink.h"
#include "Wave_module/wave_memory_backend.h"
#include "dds/dds_api.h"
#include "shareram.h"

ST_AM3352_SANDBOX g_sAm3352Sandbox;

void Am3352Sandbox_Init(void)
{
    uint16_t u16I;

    for (u16I = 0U; u16I < AM3352_PKT_MAX_WORDS; u16I++) {
        g_sAm3352Sandbox.au16RxPacket[u16I] = 0U;
    }
    for (u16I = 0U; u16I < AM3352_TX_PKT_WORDS; u16I++) {
        g_sAm3352Sandbox.au16TxPing[u16I] = 0U;
        g_sAm3352Sandbox.au16TxPong[u16I] = 0U;
    }
    g_sAm3352Sandbox.u16RxComplete      = 0U;
    g_sAm3352Sandbox.u16TxActiveIsPong  = 0U;
    g_sAm3352Sandbox.u16LastCmdId       = 0U;
    g_sAm3352Sandbox.u16LastMapOk       = 0U;
    g_sAm3352Sandbox.u32PktOkCount      = 0UL;
    g_sAm3352Sandbox.u32HeaderErrCount  = 0UL;
    g_sAm3352Sandbox.u32CrcErrCount     = 0UL;
    g_sAm3352Sandbox.u32LenErrCount     = 0UL;
    g_sAm3352Sandbox.u32UnknownCmdCount = 0UL;
    g_sAm3352Sandbox.u32StatusTxCount   = 0UL;
    g_sAm3352Sandbox.u32PollCount       = 0UL;
}

/* CRC-16/MODBUS, bytewise, words fed high byte first */
uint16_t Am3352Sandbox_Crc16(const uint16_t *pu16Words, uint16_t u16Count)
{
    uint16_t u16Crc = 0xFFFFU;
    uint16_t u16I;
    uint16_t u16Bit;
    uint16_t u16ByteIdx;

    for (u16I = 0U; u16I < u16Count; u16I++) {
        for (u16ByteIdx = 0U; u16ByteIdx < 2U; u16ByteIdx++) {
            uint16_t u16Byte = (u16ByteIdx == 0U)
                ? (uint16_t)(pu16Words[u16I] >> 8)
                : (uint16_t)(pu16Words[u16I] & 0x00FFU);
            u16Crc ^= u16Byte;
            for (u16Bit = 0U; u16Bit < 8U; u16Bit++) {
                if ((u16Crc & 0x0001U) != 0U) {
                    u16Crc = (uint16_t)((u16Crc >> 1) ^ 0xA001U);
                } else {
                    u16Crc >>= 1;
                }
            }
        }
    }
    return u16Crc;
}

void Am3352Sandbox_InjectCommand(uint16_t u16CmdId, uint32_t u32Param)
{
    uint16_t *p = g_sAm3352Sandbox.au16RxPacket;

    p[0] = AM3352_PKT_HEADER;
    p[1] = u16CmdId;
    p[2] = 2U;                                   /* payload = param hi/lo */
    p[3] = (uint16_t)(u32Param >> 16);
    p[4] = (uint16_t)(u32Param & 0xFFFFU);
    p[5] = Am3352Sandbox_Crc16(p, 5U);

    g_sAm3352Sandbox.u16RxComplete = 1U;         /* "DMA CH3 done" flag    */
}

/* Fill the idle ping/pong buffer with the current status packet, then
 * flip the active pointer - the D02_2_1 TX contract. */
static void am3352FillStatusTx(void)
{
    uint16_t *p = (g_sAm3352Sandbox.u16TxActiveIsPong != 0U)
                ? g_sAm3352Sandbox.au16TxPing      /* fill the idle one    */
                : g_sAm3352Sandbox.au16TxPong;

    p[0] = AM3352_PKT_HEADER;
    p[1] = AM3352_CMD_STATUS;
    p[2] = 6U;
    p[3] = (uint16_t)(sAccessCPU1.u32HeartBeat_CPU1 >> 16);
    p[4] = (uint16_t)(sAccessCPU1.u32HeartBeat_CPU1 & 0xFFFFU);
    p[5] = (uint16_t)sDDS.fgState;
    p[6] = g_sOutputSink.u16LastCode;
    p[7] = g_sWaveMem.u16Ready;
    p[8] = g_sSandboxCmd.u16LastResult;
    p[9] = Am3352Sandbox_Crc16(p, 9U);

    g_sAm3352Sandbox.u16TxActiveIsPong =
        (g_sAm3352Sandbox.u16TxActiveIsPong != 0U) ? 0U : 1U;
    g_sAm3352Sandbox.u32StatusTxCount++;
}

void Am3352Sandbox_Poll(void)
{
    const uint16_t *p = g_sAm3352Sandbox.au16RxPacket;
    uint16_t u16Len;
    uint16_t u16CmdId;
    uint32_t u32Param;

    g_sAm3352Sandbox.u32PollCount++;

    if (g_sAm3352Sandbox.u16RxComplete == 0U) {
        return;
    }
    g_sAm3352Sandbox.u16RxComplete = 0U;

    /* 1. Header */
    if (p[0] != AM3352_PKT_HEADER) {
        g_sAm3352Sandbox.u32HeaderErrCount++;
        return;
    }
    /* 2. Length */
    u16Len = p[2];
    if (u16Len > AM3352_PKT_PAYLOAD_MAX) {
        g_sAm3352Sandbox.u32LenErrCount++;
        return;
    }
    /* 3. CRC16 over header..payload */
    if (p[3U + u16Len] != Am3352Sandbox_Crc16(p, (uint16_t)(3U + u16Len))) {
        g_sAm3352Sandbox.u32CrcErrCount++;
        return;
    }

    /* 4. Dispatch: packet CmdID == sandbox dispatcher id (1:1 today; the
     * real bridge maps the official register protocol onto the same
     * SandboxCmd_Inject entry). Param = payload[0]<<16 | payload[1]. */
    u16CmdId = p[1];
    u32Param = (u16Len >= 2U)
        ? (((uint32_t)p[3] << 16) | (uint32_t)p[4])
        : ((u16Len == 1U) ? (uint32_t)p[3] : 0UL);

    g_sAm3352Sandbox.u16LastCmdId = u16CmdId;
    if ((u16CmdId >= 1U) && (u16CmdId <= SANDBOX_CMD_MAX)) {
        SandboxCmd_Inject(u16CmdId, u32Param);
        g_sAm3352Sandbox.u16LastMapOk = 1U;
        g_sAm3352Sandbox.u32PktOkCount++;
    } else {
        g_sAm3352Sandbox.u16LastMapOk = 0U;
        g_sAm3352Sandbox.u32UnknownCmdCount++;
    }

    /* 5. Refresh the TX status packet (ping-pong) */
    am3352FillStatusTx();
}
