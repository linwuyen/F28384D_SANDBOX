/*
 * meas_dds_module.c
 *
 * Created: 2026-05-18
 */

#include <SPIC_module/meas_dds_module.h>
#include "shareram.h"
#include "dds/dds_api.h"
#include "Sandbox_module/output_sink.h"

/* Global Instance aligned with Hungarian Notation */
ST_MEAS_DDS g_sMeasDds;

/* SPIC TX sequence buffer - ISR updates [2] every cycle, [0][1] fixed */
volatile uint16_t g_u16TxSequenceBuf[3];

/* SPIC RX receive buffer - ISR reads after SPI transfer completes */
volatile uint16_t g_u16SpiRxBuf[3];

void initMeasDds(void)
{
    // --- Hardware Connection Simulation (EN_SDI) ---
#ifdef MEAS_DDS_USE_SPIA_GPIO
    // Only configure GPIO4 on development board simulation mode
    GPIO_setControllerCore(MEAS_DDS_EN_SDI_GPIO, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_4_GPIO4);
    GPIO_setPadConfig(MEAS_DDS_EN_SDI_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(MEAS_DDS_EN_SDI_GPIO, GPIO_DIR_MODE_OUT);
    
    // INITIAL STATE: Gate is OPEN (High) to accept the first configuration
    GPIO_writePin(MEAS_DDS_EN_SDI_GPIO, 1);

    /* Use SPIA peripheral with GPIO16~19 for dev board probing */
    GPIO_setPinConfig(GPIO_16_SPIA_SIMO);
    GPIO_setPadConfig(16U, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(16U, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_17_SPIA_SOMI);
    GPIO_setPadConfig(17U, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(17U, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_18_SPIA_CLK);
    GPIO_setPadConfig(18U, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(18U, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_19_SPIA_STEN);
    GPIO_setPadConfig(19U, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(19U, GPIO_QUAL_ASYNC);

    // Manual SPI setup for dev board (SPIA)
    SPI_disableModule(MEAS_DDS_SPI_BASE);
    SPI_setConfig(MEAS_DDS_SPI_BASE, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA0,
                  SPI_MODE_CONTROLLER, 10000000U, 16U);
    SPI_resetRxFIFO(MEAS_DDS_SPI_BASE);
    SPI_enableFIFO(MEAS_DDS_SPI_BASE);
    SPI_setFIFOInterruptLevel(MEAS_DDS_SPI_BASE, SPI_FIFO_TX3, SPI_FIFO_RX3);

#ifdef SPI_LOOPBACK_TEST
    SPI_enableLoopback(MEAS_DDS_SPI_BASE);
#endif

    SPI_enableModule(MEAS_DDS_SPI_BASE);
#else
    // Production Mode: Parameters are fully managed by SysConfig Board_init()
    // Advanced Patch: Enable SPI FIFO and set level to 3 to support DMA transfers
    SPI_disableModule(MEAS_DDS_SPI_BASE);
    SPI_resetRxFIFO(MEAS_DDS_SPI_BASE);
    SPI_resetTxFIFO(MEAS_DDS_SPI_BASE);
    SPI_enableFIFO(MEAS_DDS_SPI_BASE);
    SPI_setFIFOInterruptLevel(MEAS_DDS_SPI_BASE, SPI_FIFO_TX3, SPI_FIFO_RX3);
    SPI_enableModule(MEAS_DDS_SPI_BASE);
#endif

    // Pre-load fixed TX words so ISR only needs to update [2] (Task 2.5)
    g_u16TxSequenceBuf[0] = LTC2353_CONFIG_TX; // 0xFC00 Configuration Word
    g_u16TxSequenceBuf[1] = 0x0000U;           // Dummy Word

    /* Clear M_Ref fields as safe default before setting init_done */
    sAccessCPU1.u16L1_Ref = 0U;
    sAccessCPU1.u16L2_Ref = 0U;
    sAccessCPU1.u16L3_Ref = 0U;
    sAccessCPU1.u16FuncID = 0U;
    sAccessCPU1.u16Data_H = 0U;
    sAccessCPU1.u16Data_L = 0U;

    // --- DMA Configuration ---
    // Initialize DMA Controller
    DMA_initController();
    DMA_setEmulationMode(DMA_EMULATION_FREE_RUN);

    // Setup DMA CH1 (RX from SPIC RX FIFO to g_u16SpiRxBuf)
    DMA_configAddresses(DMA_CH1_BASE, (const void *)g_u16SpiRxBuf, (const void *)(MEAS_DDS_SPI_BASE + SPI_O_RXBUF));
    DMA_configBurst(DMA_CH1_BASE, 3U, 0, 1);
    DMA_configTransfer(DMA_CH1_BASE, 1U, 0, 0);
    DMA_configMode(DMA_CH1_BASE, DMA_TRIGGER_SPICRX, DMA_CFG_ONESHOT_DISABLE | DMA_CFG_CONTINUOUS_ENABLE | DMA_CFG_SIZE_16BIT);
    DMA_setInterruptMode(DMA_CH1_BASE, DMA_INT_AT_END);
    DMA_enableInterrupt(DMA_CH1_BASE);
    DMA_enableTrigger(DMA_CH1_BASE);

    // Setup DMA CH2 (TX from g_u16TxSequenceBuf to SPIC TX FIFO)
    DMA_configAddresses(DMA_CH2_BASE, (const void *)(MEAS_DDS_SPI_BASE + SPI_O_TXBUF), (const void *)g_u16TxSequenceBuf);
    DMA_configBurst(DMA_CH2_BASE, 3U, 1, 0);
    DMA_configTransfer(DMA_CH2_BASE, 1U, 0, 0);
    DMA_configMode(DMA_CH2_BASE, DMA_TRIGGER_ADCA1, DMA_CFG_ONESHOT_DISABLE | DMA_CFG_CONTINUOUS_ENABLE | DMA_CFG_SIZE_16BIT);
    DMA_disableInterrupt(DMA_CH2_BASE);
    DMA_enableTrigger(DMA_CH2_BASE);

    // Register DMA CH1 interrupt in PIE
    Interrupt_register(INT_DMA_CH1, &dmaCh1ISR);
    Interrupt_enable(INT_DMA_CH1);

    // Start DMA channels
    DMA_startChannel(DMA_CH1_BASE);
    DMA_startChannel(DMA_CH2_BASE);

    /* Allow DMA to start processing */
    g_sMeasDds.u16InitDone = 1U;
}

#ifdef _FLASH
#pragma CODE_SECTION(dmaCh1ISR, ".TI.ramfunc")
#endif
__interrupt void dmaCh1ISR(void)
{
    // Clear DMA Channel 1 peripheral trigger/interrupt flag
    // (this driverlib version has no DMA_clearInterruptFlag)
    DMA_clearTriggerFlag(DMA_CH1_BASE);

    // Read M_Ref directly to GS RAM and increment heartbeat
    sAccessCPU1.u16L1_Ref = g_u16SpiRxBuf[0];
    sAccessCPU1.u16L2_Ref = g_u16SpiRxBuf[1];
    sAccessCPU1.u16L3_Ref = g_u16SpiRxBuf[2];
    sAccessCPU1.u32HeartBeat_CPU1++;

    /* Memory Barrier for IPC Dirty Data protection */
    asm(" RPT #3 || NOP");

    /* 
     * Data Extraction (LTC2353 24-bit per channel format)
     * Ch0: [g_u16SpiRxBuf[0] (Data 16b)][g_u16SpiRxBuf[1] High Byte (Status 8b)]
     */
    g_sMeasDds.i16AdcCh0Raw = (int16_t)g_u16SpiRxBuf[0];
    g_sMeasDds.u16AdcCh0Id  = (g_u16SpiRxBuf[1] >> 11U) & 0x01U;
    g_sMeasDds.u16AdcCh0Ss  = (g_u16SpiRxBuf[1] >> 8U)  & 0x07U;

    /* 
     * Ch1: [g_u16SpiRxBuf[1] Low Byte (Data 8b)][g_u16SpiRxBuf[2] High Byte (Data 8b)]
     *      [g_u16SpiRxBuf[2] Low Byte (Status 8b)]
     */
    g_sMeasDds.i16AdcCh1Raw = (int16_t)(((g_u16SpiRxBuf[1] & 0x00FFU) << 8U) |
                                       ((g_u16SpiRxBuf[2] >> 8U) & 0x00FFU));
    g_sMeasDds.u16AdcCh1Id  = (g_u16SpiRxBuf[2] >> 3U) & 0x01U;
    g_sMeasDds.u16AdcCh1Ss  = (g_u16SpiRxBuf[2] & 0x07U);

    /* Compute scaling using multiplication (Zero Division Constraint) */
    g_sMeasDds.f32AdcCh0V = (float32_t)g_sMeasDds.i16AdcCh0Raw * LTC2353_SCALE_V;
    g_sMeasDds.f32AdcCh1V = (float32_t)g_sMeasDds.i16AdcCh1Raw * LTC2353_SCALE_V;

    /* DDS runtime: step FSM, read sample via active wave pointer, then fan
     * out through the Output Sink mask (DEBUG_VAR / INTERNAL_DAC / AD5543).
     * The AD5543 sink loads g_u16TxSequenceBuf[2] for the next 100kHz cycle
     * (sent by DMA CH2) - SPIC/DMA timing is untouched. */
    DDS_Step();
    g_sMeasDds.u16DacOut = DDS_GetSample();
    OutputSink_Write(g_sMeasDds.u16DacOut);

    g_sMeasDds.u16IsrCounter++;

    // Acknowledge the PIE Group 7 interrupt
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP7);
}
