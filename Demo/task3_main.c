#ifdef TASK3
#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>
#include <portmacro.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"
#include "Drivers/rpi_core.h"
#include "Drivers/rpi_synch.h"
#include "Drivers/rpi_timer.h"


uint32_t CPUID(void);

#define BASE 0x40000000

#define CONTROL ((volatile uint32_t *) BASE)
#define CORE_TIMER_PRESCALER ((volatile uint32_t *) BASE + 0x08)
#define CORE_TIMER_LS ((volatile uint32_t *) BASE + 0x1c)
#define CORE0_TIMER_CONTROL ((volatile uint32_t *) BASE + 0x40)
#define CORE1_TIMER_CONTROL ((volatile uint32_t *) BASE + 0x44)

volatile int x = 0;
void wakeup_for_realz(void)
{
    printk("\033[32;3m[ Core %u ] Started\r\n\033[0m\r\n", CPUID());
    rpi_cpu_irq_enable();
    rpi_irq_init();
    //rpi_irq_enable(RPI_IRQ_ID_UART);
    //rpi_irq_enable(RPI_IRQ_ID_AUX);
    rpi_aux_mu_init();
    //rpi_aux_mu_enable_interrupts();
    //rpi_timer_setup_interrupts();
    while(1);
}

int test = 0;
void __attribute__((interrupt("IRQ")))  irq_c_handler(void)
{
    //rpi_aux_getc();
    printk( "IRQ FROM CPU %u\r\n", CPUID());
    printk( "SOURCE %u\r\n", *(volatile uint32_t*)(0x4000000));
    x = 1;
    //rpi_timer_clear_interrupts();
}

void __attribute__((interrupt("SWI")))  irq_s_handler(void)
{
    printk( "SWI FROM CPU %u\r\n", CPUID());
}

/**
 *	This is the systems main entry, some call it a boot thread.
 *
 *	-- Absolutely nothing wrong with this being called main(), just it doesn't have
 *	-- the same prototype as you'd see in a linux program.
 **/
int main(void) {
    /*
    *CONTROL = (1 << 8);
    *CORE_TIMER_PRESCALER = 0xf9;
    *CORE_TIMER_LS = 0xffff;
    *CORE0_TIMER_CONTROL = 0xf;
    *CORE1_TIMER_CONTROL = 0xf;
    */
    rpi_cpu_irq_enable();
    rpi_irq_init();
    rpi_aux_mu_init();
    rpi_aux_mu_enable_interrupts();
    core_set_start_fn( wakeup_for_realz );
    core_start(1);
    while(1);
}
#endif

