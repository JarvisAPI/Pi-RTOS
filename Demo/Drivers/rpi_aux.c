/*
 * rpi_aux.c
 *
 *  Created on: Mar 4, 2016
 *      Author: TsungMin Huang
 */

#include <stdint.h>

//#include "lib/memio.h"

#include "rpi_gpio.h"
#include "rpi_aux.h"

/* System clock frequency in MHz for the baud rate calculation */
#define _RPI_SYS_FREQ		250000000

// FIXME: Using GPIO library
#define _GPIO_FSEL1         ( PRI_BASE_ADDRESS + 0x00200004 )
#define _GPIO_PUD           ( PRI_BASE_ADDRESS + 0x00200094 )
#define _GPIO_PUD_CLK0      ( PRI_BASE_ADDRESS + 0x00200098 )

extern void PUT32 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );

/**
 * Dummy function for delay
 * @param n
 */
void dummy_aux(uint32_t n) {
    for (; n != 0; --n) {
        // dummy no op
    }
}

void rpi_aux_mu_init() {
    volatile uint32_t reg_val;

    /* Enable aux uart */
    RPI_AUX->ENABLES = RPI_AUX_MU_ENABLE;

    RPI_AUX->MU_IER = 0;

    /* Disable flow control */
    RPI_AUX->MU_CNTL = 0;

    RPI_AUX->MU_LCR = RPI_AUX_MU_LCR_8BIT_MODE;

    RPI_AUX->MU_MCR = 0;

    /* Diable all interrupts from mu and clear the fifos */
    RPI_AUX->MU_IER = (1 << 0) | (1 << 2) | (1 << 3);
    RPI_AUX->MU_IIR = 0xC6; // ??

    //#define bits 8
    //RPI_AUX->MU_BAUD = ( _RPI_SYS_FREQ / ( 8 * bits )) - 1;
    RPI_AUX->MU_BAUD = 270;

    /* Setup GPIO 14 and 15 as alternative function 5 which is
     UART 1 TXD/RXD. These need to be set before enabling the UART */
    reg_val =GET32(_GPIO_FSEL1);
    reg_val &= ~(7 << 12); // GPIO 14
    reg_val |= 2 << 12;      // ALT5
    reg_val &= ~(7 << 15); // GPIO 15
    reg_val |= 2 << 15;      // ALT5
    PUT32(_GPIO_FSEL1, reg_val);
    PUT32(_GPIO_PUD, 0);
    dummy_aux(150);
    PUT32(_GPIO_PUD_CLK0, (1 << 14) | (1 << 15));
    dummy_aux(150);
    PUT32(_GPIO_PUD_CLK0, 0);

    RPI_AUX->MU_CNTL = RPI_AUX_MU_CNTL_TX_ENABLE | RPI_AUX_MU_CNTL_RX_ENABLE;
}

#define MU_IER_RCV_INTERRUPT_ENABLE (1 << 0)
#define MU_IER_RCV_LINE_STATUS_INTERRUPT_ENABLE (1 << 2)
#define MU_IER_MODEM_STATUS_INTERRUPT_ENABLE (1 << 3)

void rpi_aux_mu_enable_interrupts(void)
{
    RPI_AUX->MU_IER = ( MU_IER_RCV_INTERRUPT_ENABLE | MU_IER_RCV_LINE_STATUS_INTERRUPT_ENABLE |
                        MU_IER_MODEM_STATUS_INTERRUPT_ENABLE );
}

void rpi_aux_mu_putc(uint32_t c) {

    if(c == 0x0A) {
        rpi_aux_mu_putc(0x0D);
    }

//    while ((RPI_AUX->MU_LSR & RPI_AUX_MU_LSR_TX_EMPTY) == 0) {
//
//    }
    while(1)
    {
        if(RPI_AUX->MU_LSR & RPI_AUX_MU_LSR_TX_EMPTY) {
            break;
        }
    }

    RPI_AUX->MU_IO = c;
}

void rpi_aux_mu_string(const char* str) {
    while (*str != 0) {
        rpi_aux_mu_putc((uint32_t) * str);
        str++;
    }
}

uint32_t rpi_aux_getc(void) {
    while( (RPI_AUX->MU_LSR & RPI_AUX_MU_LSR_DATA_READY) == 0) {

    }

    return (RPI_AUX->MU_IO & 0xFF);
}

uint32_t rpi_aux_getc_nonblocking(char *ch) {
    int read = (RPI_AUX->MU_LSR & RPI_AUX_MU_LSR_DATA_READY) != 0;
    
    *ch = (RPI_AUX->MU_IO & 0xFF);
    return read;
}
