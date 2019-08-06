#ifndef DRIVERS_RPI_CORE_TIMER_H
#define DRIVERS_RPI_CORE_TIMER_H
#include <stdint.h>
#include <stdbool.h>

void rpi_core_timer_init( void );
void rpi_core_timer_setup_interrupts( void );
void rpi_core_timer_clear_interrupts( void );
bool rpi_core_timer_pending( void );

#endif // DRIVERS_RPI_CORE_TIMER_H

