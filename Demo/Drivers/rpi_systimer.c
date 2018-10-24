/*
 * rpi_systimer.c
 *
 *  Created on: Mar 8,2016
 *      Author: TsungMin
 */

#include <stdint.h>

#include "rpi_systimer.h"

uint64_t rpi_sys_timer_get64(void) {

    uint64_t ret = 0;       // zero a very unlikely value for this timer
    uint32_t hi, lo, temp;  // temporary variables
    uint32_t cnt = 3;       // Timeout counter, do 3 tries

    hi = RPI_SYS_TIMER->HI;
    do {
        temp = hi;
        lo = RPI_SYS_TIMER->LO;
        hi = RPI_SYS_TIMER->HI;
    } while ((temp != hi) && (cnt--)); // has the high-order word read the same twice yet?
    // note that if the first condition is true, cnt is neither tested nor decremented

    if (cnt) {
        ret = (uint64_t) hi;
        ret = (ret << (sizeof(uint32_t) * 8)) | lo;
    }
    return ret;

}

