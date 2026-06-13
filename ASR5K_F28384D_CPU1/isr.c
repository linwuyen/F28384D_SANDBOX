/*
 * isr.c
 *
 *  Created on: Mar 18, 2024
 *      Author: cody_chen
 */

#include "common.h"
#include "shareram.h"
#include "c28protection.h"
#include "cTimeMeas.h"

#include "isr_common.h"
#include  <SPIC_module/meas_dds_module.h>

ST_DRV sDrv = {
        .fgStatus = _CSTAT_INIT_DRV_PARAM,

};



void initDerivedHwConfig(void)
{

}

void initHwConfigAndDrvParams(void)
{
    initHwConfig(&sHwConfig);
    initDerivedHwConfig();
}


#ifdef _FLASH
#pragma SET_CODE_SECTION(".TI.ramfunc")
#endif //_FLASH

__interrupt void INT_ADCA_AIN1_1_ISR (void)
{
    // Clear the interrupt flag
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);

    // Acknowledge the interrupt
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}


#ifdef _FLASH
#pragma SET_CODE_SECTION()
#endif //_FLASH
