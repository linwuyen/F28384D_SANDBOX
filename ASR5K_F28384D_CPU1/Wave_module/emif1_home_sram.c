/*
 * emif1_home_sram.c
 *
 * Created: 2026-06-11
 *
 * Home board EMIF1 CS3 async SRAM init (IS61LV51216, 512K x 16).
 * See emif1_home_sram.h for the pin map and porting notes.
 */

#include "Wave_module/emif1_home_sram.h"
#include "driverlib.h"
#include "device.h"

/*
 * Async timing for IS61LV51216 at EMIF1CLK = 100MHz (10ns per cycle).
 * Register fields encode (cycles - 1); values below are deliberately
 * conservative for first bring-up (~110ns per access) and can be tightened
 * toward the 10ns-class part once the scope confirms clean strobes.
 */
#define HOME_SRAM_R_SETUP    2U  /* 3 cycles */
#define HOME_SRAM_R_STROBE   5U  /* 6 cycles */
#define HOME_SRAM_R_HOLD     1U  /* 2 cycles */
#define HOME_SRAM_W_SETUP    2U  /* 3 cycles */
#define HOME_SRAM_W_STROBE   5U  /* 6 cycles */
#define HOME_SRAM_W_HOLD     1U  /* 2 cycles */
#define HOME_SRAM_TURNARND   2U  /* 3 cycles */

/*
 * Pins the U9 schematic could NOT resolve (plain net labels, multiple mux
 * options). Defaults below are assumptions - see emif1_home_sram_pin_audit.md.
 * Override the PINCFG/GPIO pair together if bring-up shows otherwise.
 *
 *   CS3n : GPIO19 / GPIO29 / GPIO35(only if A0 sits on GPIO38)
 *   OEn  : GPIO32 / GPIO37 (A2 is on GPIO40 on this board, so 37 is free)
 *
 * Failure signatures: CS3n wrong -> mem test all garbage from step 1;
 * OEn wrong -> writes ok but every read floats (often reads 0xFFFF).
 */
#ifndef HOME_SRAM_CS3N_PINCFG
#define HOME_SRAM_CS3N_PINCFG   GPIO_29_EMIF1_CS3N
#define HOME_SRAM_CS3N_GPIO     29U
#endif

#ifndef HOME_SRAM_OEN_PINCFG
#define HOME_SRAM_OEN_PINCFG    GPIO_32_EMIF1_OEN
#define HOME_SRAM_OEN_GPIO      32U
#endif

void Emif1HomeSram_Init(void)
{
    EMIF_AsyncTimingParams sTiming;

    /* EMIF1 owned by CPU1, EMIF1CLK = SYSCLK / 2 = 100MHz */
    EMIF_selectController(EMIF1CONFIG_BASE, EMIF_CONTROLLER_CPU1_G);
    SysCtl_setEMIF1ClockDivider(SYSCTL_EMIF1CLK_DIV_2);

    /* Home-board pin overrides (applied after Board_init). The U9 schematic
     * net labels (ESC / ENET second functions) prove this board uses the
     * ALTERNATE mux pins for A1-A4 and D0-D4/D7, different from the product
     * SysConfig picks - see emif1_home_sram_pin_audit.md section 6.
     * SRAM A0 hangs on EM1BA1 (GPIO21, set by Board_init); SRAM uses
     * EM1A13-A17 only - A18/A19 are not wired on this board. */

    /* Control */
    GPIO_setPinConfig(HOME_SRAM_CS3N_PINCFG);
    GPIO_setPinConfig(HOME_SRAM_OEN_PINCFG);

    /* Address lines on alternate pins (schematic-decoded, differ from syscfg) */
    GPIO_setPinConfig(GPIO_39_EMIF1_A1);   /* syscfg: GPIO36 */
    GPIO_setPinConfig(GPIO_40_EMIF1_A2);   /* syscfg: GPIO37 */
    GPIO_setPinConfig(GPIO_41_EMIF1_A3);   /* syscfg: GPIO38 */
    GPIO_setPinConfig(GPIO_44_EMIF1_A4);   /* syscfg: GPIO39 */

    /* Upper address lines (device-unique pins) */
    GPIO_setPinConfig(GPIO_86_EMIF1_A13);
    GPIO_setPinConfig(GPIO_87_EMIF1_A14);
    GPIO_setPinConfig(GPIO_88_EMIF1_A15);
    GPIO_setPinConfig(GPIO_89_EMIF1_A16);
    GPIO_setPinConfig(GPIO_90_EMIF1_A17);

    /* Data lines on alternate pins (schematic-decoded, differ from syscfg) */
    GPIO_setPinConfig(GPIO_85_EMIF1_D0);   /* syscfg: GPIO55 */
    GPIO_setPinConfig(GPIO_83_EMIF1_D1);   /* syscfg: GPIO56 */
    GPIO_setPinConfig(GPIO_82_EMIF1_D2);   /* syscfg: GPIO57 */
    GPIO_setPinConfig(GPIO_81_EMIF1_D3);   /* syscfg: GPIO58 */
    GPIO_setPinConfig(GPIO_80_EMIF1_D4);   /* syscfg: GPIO59 */
    GPIO_setPinConfig(GPIO_77_EMIF1_D7);   /* syscfg: GPIO62 */

    /* Release the stale product-board picks. Mandatory for the data pins:
     * leaving two balls muxed to the same EMIF1_Dx makes the read path
     * undefined. Address releases just stop driving stray dev-board balls. */
    GPIO_setPinConfig(GPIO_36_GPIO36);     /* was EMIF1_A1 */
#if (HOME_SRAM_OEN_GPIO != 37U)
    GPIO_setPinConfig(GPIO_37_GPIO37);     /* was EMIF1_A2 */
#endif
    GPIO_setPinConfig(GPIO_38_GPIO38);     /* was EMIF1_A3 */
    GPIO_setPinConfig(GPIO_55_GPIO55);     /* was EMIF1_D0 */
    GPIO_setPinConfig(GPIO_56_GPIO56);     /* was EMIF1_D1 */
    GPIO_setPinConfig(GPIO_57_GPIO57);     /* was EMIF1_D2 */
    GPIO_setPinConfig(GPIO_58_GPIO58);     /* was EMIF1_D3 */
    GPIO_setPinConfig(GPIO_59_GPIO59);     /* was EMIF1_D4 */
    GPIO_setPinConfig(GPIO_62_GPIO62);     /* was EMIF1_D7 */

    GPIO_setQualificationMode(HOME_SRAM_CS3N_GPIO, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(HOME_SRAM_OEN_GPIO, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(39U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(40U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(41U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(44U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(86U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(87U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(88U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(89U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(90U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(85U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(83U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(82U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(81U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(80U, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(77U, GPIO_QUAL_ASYNC);

    /* CS3: normal async mode, 16-bit bus, no extended wait (EM1WAIT unused) */
    EMIF_setAsyncMode(EMIF1_BASE, EMIF_ASYNC_CS3_OFFSET,
                      EMIF_ASYNC_NORMAL_MODE);
    EMIF_disableAsyncExtendedWait(EMIF1_BASE, EMIF_ASYNC_CS3_OFFSET);
    EMIF_setAsyncDataBusWidth(EMIF1_BASE, EMIF_ASYNC_CS3_OFFSET,
                              EMIF_ASYNC_DATA_WIDTH_16);

    sTiming.rSetup   = HOME_SRAM_R_SETUP;
    sTiming.rStrobe  = HOME_SRAM_R_STROBE;
    sTiming.rHold    = HOME_SRAM_R_HOLD;
    sTiming.wSetup   = HOME_SRAM_W_SETUP;
    sTiming.wStrobe  = HOME_SRAM_W_STROBE;
    sTiming.wHold    = HOME_SRAM_W_HOLD;
    sTiming.turnArnd = HOME_SRAM_TURNARND;
    EMIF_setAsyncTimingParams(EMIF1_BASE, EMIF_ASYNC_CS3_OFFSET, &sTiming);
}
