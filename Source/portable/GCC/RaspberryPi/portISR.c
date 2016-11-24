#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"

/* Hardware specific definitions. */
#include <Drivers/rpi_irq.h>
#include <Drivers/rpi_timer.h>

/* Constants required to setup the tick ISR. */
#define portPRESCALE_VALUE 						 0xF9

/*
 * Setup the timer 0 to generate the tick interrupts at the required frequency.
 */
void vSetupTimerInterrupt( void )
{
uint32_t ulCompareMatch;
extern void FreeRTOS_Tick_Handler( void );
	

	/* Calculate the match value required for our wanted tick rate. */
	ulCompareMatch = configCPU_CLOCK_HZ / configTICK_RATE_HZ;

	/* Protect against divide by zero.  Using an if() statement still results
	 in a warning - hence the #if. */
#if portPRESCALE_VALUE != 0
	{
		ulCompareMatch /= ( portPRESCALE_VALUE + 1);
	}
#endif

	rpi_cpu_irq_disable();

	// Should we use system timer ??
	RPI_TIMER->CNTL = 0x003E0000;
	RPI_TIMER->LOAD = ulCompareMatch;
	RPI_TIMER->RELOAD = ulCompareMatch;
	RPI_TIMER->PREDIV = portPRESCALE_VALUE; // 250 - 1 = 249 (0xF9)
	RPI_TIMER->IRQCLR = 0;
	RPI_TIMER->CNTL = 0x003E00A2;

	rpi_irq_register_handler(
		RPI_IRQ_ID_TIMER_0, 
		(RPI_IRQ_HANDLER_t) FreeRTOS_Tick_Handler, NULL);

	rpi_irq_enable(RPI_IRQ_ID_TIMER_0);

	rpi_cpu_irq_enable();
}

void vClearTimerInterrupt( void )
{
    RPI_TIMER->IRQCLR = 1;
}

volatile uint32_t portEOIStub;
