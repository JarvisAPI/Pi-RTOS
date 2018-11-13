#ifndef _RPI_AUX_H_
#define _RPI_AUX_H_

#include "rpi_base.h"

#define RPI_AUX_BASE                     ( PRI_BASE_ADDRESS + 0x215000 )

#define RPI_AUX_MU_IRQ                   ( 1 << 0 )

#define RPI_AUX_MU_ENABLE                ( 1 << 0 )

#define RPI_AUX_MU_LCR_8BIT_MODE         ( 3 << 0 )  /* See errata for this value */
#define RPI_AUX_MU_LCR_BREAK             ( 1 << 6 )
#define RPI_AUX_MU_LCR_DLAB_ACCESS       ( 1 << 7 )

#define RPI_AUX_MU_IER_TX_IRQ_ENABLE     ( 1 << 1 )

#define RPI_AUX_MU_IIR_MASK              ( 0x07 )
#define RPI_AUX_MU_IIR_RX_IRQ            ( 0x04 )
#define RPI_AUX_MU_IIR_TX_IRQ            ( 0x02 )

#define RPI_AUX_MU_MCR_RTS               ( 1 << 1 )

#define RPI_AUX_MU_LSR_DATA_READY        ( 1 << 0 )
#define RPI_AUX_MU_LSR_RX_OVERRUN        ( 1 << 1 )
#define RPI_AUX_MU_LSR
#define RPI_AUX_MU_LSR_TX_EMPTY          ( 1 << 5 )
#define RPI_AUX_MU_LSR_TX_IDLE           ( 1 << 6 )

#define RPI_AUX_MU_MSR_CTS               ( 1 << 5 )

#define RPI_AUX_MU_CNTL_RX_ENABLE        ( 1 << 0 )
#define RPI_AUX_MU_CNTL_TX_ENABLE        ( 1 << 1 )
#define RPI_AUX_MU_CNTL_RTS_FLOW         ( 1 << 2 )
#define RPI_AUX_MU_CNTL_CTS_FLOW         ( 1 << 3 )
#define RPI_AUX_MU_CNTL_RTS_FIFO         ( 3 << 4 )
#define RPI_AUX_MU_CNTL_RTS_ASSERT       ( 1 << 6 )
#define RPI_AUX_MU_CNTL_CTS_ASSERT       ( 1 << 7 )

#define RPI_AUX_MU_STAT_SYMBOL_AV        ( 1 << 0 )
#define RPI_AUX_MU_STAT_SPACE_AV         ( 1 << 1 )
#define RPI_AUX_MU_STAT_RX_IDLE          ( 1 << 2 )
#define RPI_AUX_MU_STAT_TX_IDLE          ( 1 << 3 )
#define RPI_AUX_MU_STAT_RX_OVERRUN       ( 1 << 4 )
#define RPI_AUX_MU_STAT_TX_FIFO_FULL     ( 1 << 5 )
#define RPI_AUX_MU_STAT_RTS              ( 1 << 6 )
#define RPI_AUX_MU_STAT_CTS              ( 1 << 7 )
#define RPI_AUX_MU_STAT_TX_EMPTY         ( 1 << 8 )
#define RPI_AUX_MU_STAT_TX_DONE          ( 1 << 9 )
#define RPI_AUX_MU_STAT_RX_FIFO_LEVEL    ( 7 << 16 )
#define RPI_AUX_MU_STAT_TX_FIFO_LEVEL    ( 7 << 24 )

typedef struct {
    uint32_t IRQ;
    uint32_t ENABLES;
    uint32_t reserved1[((0x40 - 0x04) / 4) - 1];
    uint32_t MU_IO;
    uint32_t MU_IER;
    uint32_t MU_IIR;
    uint32_t MU_LCR;
    uint32_t MU_MCR;
    uint32_t MU_LSR;
    uint32_t MU_MSR;
    uint32_t MU_SCRATCH;
    uint32_t MU_CNTL;
    uint32_t MU_STAT;
    uint32_t MU_BAUD;
} RPI_AUX_t;

#define RPI_AUX ((volatile RPI_AUX_t *) RPI_AUX_BASE)

/**
 * For now only support buadrate 115200, 8bit mode
 */
void rpi_aux_mu_init();

/**
 * Send char
 * @param c
 */
void rpi_aux_mu_putc(uint32_t c);

/**
 * Send string
 * @param str
 */
void rpi_aux_mu_string(char* str);

uint32_t rpi_aux_getc(void);

#endif /* _RPI_AUX_H_ */
