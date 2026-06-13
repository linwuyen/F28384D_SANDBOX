/*
 * emif1_home_sram.h
 *
 * Created: 2026-06-11
 *
 * Home board EMIF1 async SRAM (External SRAM Wave Source) initialization.
 *
 * Target device : IS61LV51216 (512K x 16 async SRAM, NOT SDRAM)
 * Chip select   : EMIF1 CS3n, word address window 0x00300000 - 0x0037FFFF
 *
 * This module exists ONLY for the WAVE_BACKEND_HOME_EMIF1_SRAM platform.
 * It validates the external-memory runtime wave source path. It does NOT
 * validate SDRAM refresh or SDRAM timing - the product board IS42S16160J
 * SDRAM bring-up remains a separate future task.
 *
 * [HOME BOARD WIRING - per dev board manual + U9 schematic decode]
 *   SRAM /CE -> EM1CS3n, /WE -> EM1WEn, /OE -> EM1OEn
 *   SRAM /UB,/LB -> GND (always enabled, DQM unused)
 *   SRAM D0-D15 -> EM1D0-D15 (sequential)
 *   SRAM A0 -> EM1BA1 (standard C2000 16-bit async behavior: word address
 *              LSB is driven on EM1BA1; GPIO21 set by Board_init, INFERRED)
 *   SRAM A1-A18 -> EM1A0-A13, A15-A17, A14 (scrambled but one-to-one;
 *              bijective mapping needs no software de-scramble and keeps
 *              the power-of-2 address bus memory test valid)
 *   => EMIF signals used: BA1, A0-A17 (A18/A19 unused)
 *
 * GPIO mapping decoded from the U9 schematic second-function net labels
 * (ESC_* / ENET_* functions pin down the exact ball) - full evidence trail
 * in emif1_home_sram_pin_audit.md. KEY FINDING: this board uses ALTERNATE
 * mux pins for A1-A4 and D0-D4/D7, different from the product SysConfig:
 *   A1=GPIO39, A2=GPIO40, A3=GPIO41, A4=GPIO44
 *   D0=GPIO85, D1=GPIO83, D2=GPIO82, D3=GPIO81, D4=GPIO80, D7=GPIO77
 * Matching syscfg (no action): A5-A12=GPIO45-52, A13-A17=GPIO86-90,
 * D5/D6=GPIO79/78, D8-D15=GPIO76-69, WEn=GPIO31.
 * Still unresolved (plain net labels): CS3n (19/29, default 29) and
 * OEn (32/37, default 32) - both overridable via HOME_SRAM_*_PINCFG.
 * The stale syscfg picks (GPIO36/37/38, GPIO55-59, GPIO62) are released
 * back to plain GPIO; for data pins this is mandatory because double-muxed
 * EMIF data inputs make the read path undefined.
 *
 * These overrides are applied AFTER Board_init() and only in the HOME
 * backend build, so pinmux.syscfg (product configuration) stays untouched.
 * After the override the product SDRAM window (0x80000000) is non-functional
 * in this build - do not trigger HwVerification SDRAM tests on the home board.
 */

#ifndef EMIF1_HOME_SRAM_H_
#define EMIF1_HOME_SRAM_H_

/** @brief Configure EMIF1 CS3 for the home board async SRAM.
 *         Call after Board_init(), before any access to 0x00300000. */
void Emif1HomeSram_Init(void);

#endif /* EMIF1_HOME_SRAM_H_ */
