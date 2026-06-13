// #############################################################################
//
//  FILE:   empty_c28x_dual_sysconfig_cpu1.c
//
//  TITLE: SysConfig Empty Project
//
//  CPU1 Empty Project Example
//
//  This example is an empty project setup for Driverlib development for CPU1.
//
// #############################################################################


//
// Included Files
//
#include "common.h"
#include "shareram.h"
#include "mb_slave/ModbusSlave.h"
#include "cTimeMeas.h"
#include "HwVerification.h"
#include <SPIB_module/spi_slave.h>
#include <SPIC_module/meas_dds_module.h>
#include "Wave_module/wave_memory_backend.h"
#include "dds/dds_api.h"
#include "Sandbox_module/sandbox_manager.h"



#pragma DATA_SECTION(sAccessCPU1,"RW_CPU1");
ST_SHARERAM sAccessCPU1;
#pragma DATA_SECTION(sReadCPU2,"RD_CPU2");
ST_SHARERAM sReadCPU2;

#define ALLOW_CPU2_ACCESS_GSRAM  (MEMCFG_SECT_GS5 |MEMCFG_SECT_GS6 |MEMCFG_SECT_GS7 |MEMCFG_SECT_GS8 |MEMCFG_SECT_GS9 \
                                 |MEMCFG_SECT_GS10|MEMCFG_SECT_GS11| \
                                 MEMCFG_SECT_GS15 )
uint16_t MEP_ScaleFactor;

volatile uint32_t ePWM[9] = {
    0,
    EPWM1_BASE,
    EPWM2_BASE,
    EPWM3_BASE,
    EPWM4_BASE,
    EPWM5_BASE,
    EPWM6_BASE,
    EPWM7_BASE,
    EPWM8_BASE
};

static uint32_t s_u32Cpu1HbTs = 0U;

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    Device_init();

#ifdef _FLASH
    //
    // Send boot command to allow the CPU2 application to begin execution
    //
    Device_bootCPU2(BOOTMODE_BOOT_TO_FLASH_SECTOR0);
#endif // _STANDALONE

    //
    // Give memory access to GS13 RAM to CPU2
    //
    MemCfg_setGSRAMMasterSel(ALLOW_CPU2_ACCESS_GSRAM,
                             MEMCFG_GSRAMMASTER_CPU2);


    // The fapi_ram_LoadStart, fapi_ram_LoadSize, and fapi_ram_RunStart symbols
    // are created by the linker. Refer to the device .cmd file.
    //
    memcpy(&fapi_ram_RunStart, &fapi_ram_LoadStart, (size_t)&fapi_ram_LoadSize);

    // FLASH Initialization:
    // The "FLASH_init()" should be called after or during initialization functions like
    // Device_init() or Device_enableAllPeripherals().
    FLASH_init();

    //
    // Disable pin locks and enable internal pullups.
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Board Initialization (SysConfig)
    //
    Board_init();

    //
    // Wave Memory Backend: platform EMIF init (home board: EMIF1 CS3 async
    // SRAM), destructive memory test, then DDS init. The sine page is built
    // non-blocking by DDS_Poll() in the main loop, then validated/activated
    // before the 100kHz ISR consumes it.
    //
    WaveMem_Init();
    WaveMem_MemTest();
    DDS_Init(100000UL,   /* 100kHz sample rate                   */
             100000UL,   /* 1000.00 Hz -> set 1kHz output        */
             65535U,     /* full-scale amplitude                 */
             32768U);    /* mid-scale offset (AD5543 mid code)   */

    //
    // ASR5K Sandbox: one init for every sandbox module (output sink,
    // command dispatcher, EPWM tick observer, CV/CC abstraction, FSI/MCBSP
    // stubs, AM3352 injector, fake flash, fake M0, IPC observer).
    //
    Sandbox_InitAll();

    // IPC synchronization
    //
    IPC_sync(IPC_CPU1_L_CPU2_R, IPC_SYNC);
    
    initMeasDds();
    initHRPWMxAB(MEAS_CNV_BASE);
    initPWM();
    //
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    //
    EINT;
    ERTM;

    //
    // Loop Forever
    //
    for(;;)
    {
        // DDS: build sine page (one entry per pass), auto-start once ready
        DDS_Poll();
        {
            static uint16_t s_u16DdsStarted = 0U;
            if ((s_u16DdsStarted == 0U) && (DDS_IsInitComplete() != 0U)) {
                DDS_Start();
                s_u16DdsStarted = 1U;
            }
        }

        // All sandbox modules: command dispatch, observers, fake generators
        Sandbox_PollAll();

        // Execute all hardware verification tasks
        HwVerification_RunAllTests();

        if((U32_UPCNTS - s_u32Cpu1HbTs) >= T_500MS) {
            GPIO_togglePin(STAT_CPU1);
            s_u32Cpu1HbTs = U32_UPCNTS;
        }

        runSPIBslave();

//        // Start timing
        startTimerMeasure(&sDrv.tpMainCost);
//
        pollTimeTask();
//
//        // Stop timing
//        stopTimerMeasure(&sDrv.tpMainCost);

    }
}

//
// End of File
//
