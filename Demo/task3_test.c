#ifdef TASK3_TEST
#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>
#include <portmacro.h>

#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_core.h"
#include "Drivers/rpi_memory.h"
#include "Drivers/rpi_core_timer.h"

uint32_t CPUID(void);

// Memory-Mapped I/O output

volatile int x = 0;
void __attribute__((interrupt("IRQ")))  irq_c_handler(void)
{
    printk("CPUID: %u\r\n", CPUID());
    if (rpi_core_timer_pending())
        rpi_core_timer_clear_interrupts();    // clear cntv interrupt and set next 1sec timer.
    x++;
    if ( x == 10 )
        portYIELD();
}


void __attribute__((interrupt("SWI"))) irq_s_handler(void)
{
    printk( "SWI FROM CPU %u\r\n", CPUID());
}

void wakeup_for_realz(void)
{
    printk("\033[32;1m[ Core %u ] Started\033[0m\r\n", CPUID());
    rpi_cpu_irq_enable();
    rpi_core_timer_init();


    while(1);
}

int main(void)
{
    rpi_irq_init();
    rpi_cpu_irq_enable();
    rpi_core_timer_init();

    //rpi_core_timer_setup_interrupts();
    core_set_start_fn( wakeup_for_realz );
    core_start(1);
    rpi_core_timer_setup_interrupts();
    while(x<10);

    while (1) {
    }
}
#endif
