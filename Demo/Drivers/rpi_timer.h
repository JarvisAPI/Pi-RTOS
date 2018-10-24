#ifndef _RPI_TIMER_H_
#define _RPI_TIMER_H_

#include "rpi_base.h"

/* ARM timer base address */
#define RPI_TIMER_BASE          ( PRI_BASE_ADDRESS + 0xB400 )

/** @brief 0 : 16-bit counters - 1 : 23-bit counter */
#define RPI_TIMER_23BIT         ( 1 << 1 )

#define RPI_TIMER_PRESCALE_1    ( 0 << 2 )
#define RPI_TIMER_PRESCALE_16   ( 1 << 2 )
#define RPI_TIMER_PRESCALE_256  ( 2 << 2 )

/** @brief 0 : ARM timer interrupt disabled - 1 : ARM timer interrupt enabled */
#define RPI_TIMER_INT_ENABLE    ( 1 << 5 )
#define RPI_TIMER_INT_DISABLE   ( 0 << 5 )

/** @brief 0 : ARM timer disabled - 1 : ARM timer enabled */
#define RPI_TIMER_ENABLE        ( 1 << 7 )
#define RPI_TIMER_DISABLE       ( 0 << 7 )

typedef struct {
    uint32_t LOAD;
    uint32_t VALUE;
    uint32_t CNTL;
    uint32_t IRQCLR;
    uint32_t RAWIRQ;
    uint32_t IRQMASKED;
    uint32_t RELOAD;
    uint32_t PREDIV;
    uint32_t FREECNT;
} RPI_TIMER_t;

#define RPI_TIMER ((volatile RPI_TIMER_t *) RPI_TIMER_BASE)

#endif /* _RPI_ARM_TIMER_H_ */
