/*
 *  File Name: linkEeprom.h
 *
 *  Created on: 12/30/2025
 *  Author: POWER2-54FD92
 */

typedef struct {
    uint16_t size;
    void *ptr;
} EE_REG;
typedef EE_REG * HAL_EEREG;

#define _TABLE_ADVPARAMS    5
