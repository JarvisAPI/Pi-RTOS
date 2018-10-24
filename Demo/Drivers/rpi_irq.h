#ifndef _RPI_IRQ_H
#define _RPI_IRQ_H

#include "rpi_base.h"

#define RPI_IRQ_BASE                 ( PRI_BASE_ADDRESS + 0xB200 )

/* RPI IRQ type ID */
/* 0 ~ 31 : Enable & Disable & Pending_1  */
#define RPI_IRQ_ID_SYS_TIMER1         1
#define RPI_IRQ_ID_SYS_TIMER3         3
#define RPI_IRQ_ID_AUX               29

/* 32 ~ 63 : Enable & Disable & Pending 2 */
#define RPI_IRQ_ID_SPI_SLAVE         43
#define RPI_IRQ_ID_PWA0              45
#define RPI_IRQ_ID_PWA1              46
#define RPI_IRQ_ID_SMI               48
#define RPI_IRQ_ID_GPIO_0            49
#define RPI_IRQ_ID_GPIO_1            50
#define RPI_IRQ_ID_GPIO_2            51
#define RPI_IRQ_ID_GPIO_3            52
#define RPI_IRQ_ID_I2C               53
#define RPI_IRQ_ID_SPI               54
#define RPI_IRQ_ID_PCM               55
#define RPI_IRQ_ID_UART              57
#define RPI_GPU_IRQ_TOTAL            ( 2 * 32 )

/* 64 ~ 71: Enable & Disable & Pending basic */
#define RPI_IRQ_ID_TIMER_0           ( RPI_GPU_IRQ_TOTAL + 0 )
#define RPI_IRQ_ID_MAILBOX_0         ( RPI_GPU_IRQ_TOTAL + 1 )
#define RPI_IRQ_ID_DOORBELL_0        ( RPI_GPU_IRQ_TOTAL + 2 )
#define RPI_IRQ_ID_DOORBELL_1        ( RPI_GPU_IRQ_TOTAL + 3 )
#define RPI_IRQ_ID_GPU0_HALTED       ( RPI_GPU_IRQ_TOTAL + 4 )
#define RPI_IRQ_ID_GPU1_HALTED       ( RPI_GPU_IRQ_TOTAL + 5 )
#define RPI_IRQ_ID_ILLEGAL_ACCESS_1  ( RPI_GPU_IRQ_TOTAL + 6 )
#define RPI_IRQ_ID_ILLEGAL_ACCESS_2  ( RPI_GPU_IRQ_TOTAL + 7 )

#define RPI_TOTAL_IRQ                 ( RPI_GPU_IRQ_TOTAL + 8 )

#define RPI_IRQ_ID_MASK(IRQ_ID)       ( 1 << ( ( IRQ_ID )  % 32 ) )
#define RPI_IRQ_ID_BANK(IRQ_ID)       ( ( IRQ_ID ) / 32 )

#define RPI_IRQ_MASK                  0xFFFFFFFF

typedef struct {
    volatile uint32_t BASIC_PENDING;
    volatile uint32_t PENDING_1;
    volatile uint32_t PENDING_2;
    volatile uint32_t FIQ_CNTL;
    volatile uint32_t ENABLE_1;
    volatile uint32_t ENABLE_2;
    volatile uint32_t ENABLE_BASIC;
    volatile uint32_t DISABLE_1;
    volatile uint32_t DISABLE_2;
    volatile uint32_t DISABLE_BASIC;
} RPI_IRQ_t;

#define RPI_IRQ ((volatile RPI_IRQ_t *) (RPI_IRQ_BASE))

/* RPI IRQ handler function */
typedef void (*RPI_IRQ_HANDLER_t)(int nIRQ, void *pParam);

/* RPI irq handler function table */
typedef struct {
    RPI_IRQ_HANDLER_t pHandler;  // Handler function
    void *pParam;                // Handler function param
} RPI_IRQ_TABLE_t;

/**
 * Init RPI IRQ chip
 */
void rpi_irq_init();

/**
 * Register IRQ handler function
 * @param nIRQ: IRQ type
 * @param pHandler: handler function
 * @param pParam: param for handler function
 * @return
 */
int rpi_irq_register_handler(uint32_t nIRQ, RPI_IRQ_HANDLER_t pHandler,
        void *pParam);

/**
 * Enable IRQ chip service
 * @param nIRQ
 * @return -1: enable failed - nIRQ : IRQ enabled
 */
int rpi_irq_enable(uint32_t nIRQ);

/**
 * Disable IRQ chip service
 * @param nIRQ
 * @return -1: disable failed - nIRQ : IRQ disabled
 */
int rpi_irq_disable(uint32_t nIRQ);

/**
 * Enable CPU interrupt service
 */
void rpi_cpu_irq_enable();

/**
 * Disable CPU interrupt service
 */
void rpi_cpu_irq_disable();

///**
// *
// * @param irq
// * @return
// */
//uint32_t rpi_irq_set_mask(uint32_t irq);
//
///**
// *
// * @param irq
// * @return
// */
//uint32_t rpi_irq_clear_mask(uint32_t irq);

#endif /* _RPI_IRQ_H */
