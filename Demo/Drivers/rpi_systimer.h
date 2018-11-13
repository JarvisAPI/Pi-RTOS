/*
 * rpi_systimer.h
 *
 *  Created on: Mar 4, 2016
 *      Author: TsungMin Huang
 *  Ref:
 *	Bare Metal Programming in C
 *  http://www.valvers.com/open-software/raspberry-pi/step01-bare-metal-programming-in-cpt1/
 */

#ifndef _RPI_SYSTIMER_H_
#define _RPI_SYSTIMER_H_

#include "rpi_base.h"

/* System timer base address */
#define RPI_SYS_TIMER_BASE            ( PRI_BASE_ADDRESS + 0x3000 )

#define RPI_SYS_TIMER_1               1
#define RPI_SYS_TIMER_3               3

typedef struct {
    uint32_t CS;
    uint32_t LO;
    uint32_t HI;
    uint32_t C0;
    uint32_t C1;
    uint32_t C2;
    uint32_t C3;
} RPI_SYS_TIMER_t;

#define RPI_SYS_TIMER ((volatile RPI_SYS_TIMER_t *) RPI_SYS_TIMER_BASE)

/**
 * return the current value of system timer
 */
uint64_t rpi_sys_timer_get64( void );

#endif /* _RPI_SYSTIMER_H_ */
