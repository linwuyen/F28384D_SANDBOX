/*
 * am3352_sandbox.h
 *
 * Created: 2026-06-12 (ASR5K_SANDBOX)
 * Updated: 2026-06-13 - upgraded from a bare command mailbox to the
 *          D02_2_1 packet contract: Header / CmdID / Len / Payload /
 *          CRC16 through an RX Packet Buffer, status returned via a
 *          TX Ping-Pong buffer pair.
 *
 * Software AM3352 (SPIB system master) stand-in.
 *
 * D02_2_1 architecture being modeled:
 *   AM3352 --SPI--> SPIB RX FIFO --DMA CH3--> RxPacketBuffer --poll--> parser
 *   StateUpdater --> TxPing/TxPong --DMA CH4--> SPIB TX FIFO --> AM3352
 *
 * In the sandbox the "SPI + DMA CH3/CH4" legs are replaced by direct
 * memory copies (injector writes RxPacketBuffer and raises the complete
 * flag exactly like DMA CH3 would). The packet walk - header check, CRC16
 * verify, command dispatch, ping-pong status fill - is the REAL contract
 * and survives the port; only the transport swaps in.
 * DMA CH3/CH4 stay unconfigured here (official ownership reserved).
 *
 * Packet format (16-bit words):
 *   [0] Header 0xA55A
 *   [1] CmdID  (sandbox dispatcher command id, 1..SANDBOX_CMD_MAX)
 *   [2] Len    (payload word count, 0..AM3352_PKT_PAYLOAD_MAX)
 *   [3..3+Len-1] Payload (param: [3]=high word, [4]=low word)
 *   [3+Len] CRC16 (CRC-16/MODBUS over words 0..3+Len-1, hi byte first)
 *
 * TX status packet (filled into the idle ping/pong buffer every poll):
 *   [0] Header, [1] 0x8001 (STATUS), [2] Len=6,
 *   [3] tick_hi, [4] tick_lo, [5] dds_state, [6] dac_code,
 *   [7] wave_ready, [8] last_result, [9] CRC16
 */

#ifndef SANDBOX_AM3352_SANDBOX_H_
#define SANDBOX_AM3352_SANDBOX_H_

#include <stdint.h>

#define AM3352_PKT_HEADER       0xA55AU
#define AM3352_PKT_PAYLOAD_MAX  8U
#define AM3352_PKT_MAX_WORDS    (3U + AM3352_PKT_PAYLOAD_MAX + 1U)
#define AM3352_TX_PKT_WORDS     10U
#define AM3352_CMD_STATUS       0x8001U

typedef struct {
    /* RX side (models DMA CH3 destination) */
    uint16_t au16RxPacket[AM3352_PKT_MAX_WORDS];
    volatile uint16_t u16RxComplete;   /* "DMA CH3 transfer complete" flag */

    /* TX side (models DMA CH4 ping-pong sources) */
    uint16_t au16TxPing[AM3352_TX_PKT_WORDS];
    uint16_t au16TxPong[AM3352_TX_PKT_WORDS];
    uint16_t u16TxActiveIsPong;        /* which buffer holds latest status */

    /* Diagnostics */
    uint16_t u16LastCmdId;
    uint16_t u16LastMapOk;
    uint32_t u32PktOkCount;            /* parsed + forwarded               */
    uint32_t u32HeaderErrCount;
    uint32_t u32CrcErrCount;
    uint32_t u32LenErrCount;
    uint32_t u32UnknownCmdCount;
    uint32_t u32StatusTxCount;         /* status packets prepared          */
    uint32_t u32PollCount;
} ST_AM3352_SANDBOX;

extern ST_AM3352_SANDBOX g_sAm3352Sandbox;

void Am3352Sandbox_Init(void);

/** @brief CRC-16/MODBUS (poly 0xA001 reflected, init 0xFFFF) over words
 *  (high byte then low byte per word). Shared with future real bridge. */
uint16_t Am3352Sandbox_Crc16(const uint16_t *pu16Words, uint16_t u16Count);

/** @brief Software injector: builds a full packet (header+CRC) into the
 *  RX packet buffer and raises the complete flag - exactly what the real
 *  AM3352 + DMA CH3 path would leave behind. */
void Am3352Sandbox_InjectCommand(uint16_t u16CmdId, uint32_t u32Param);

/** @brief Parse a completed RX packet, dispatch, refresh TX status. */
void Am3352Sandbox_Poll(void);

#endif /* SANDBOX_AM3352_SANDBOX_H_ */
