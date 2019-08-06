#include <stdint.h>
#include <FreeRTOS.h>
#include "Drivers/rpi_core.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"
#include "Drivers/rpi_mmu.h"
#include "Drivers/rpi_core_timer.h"
#include <printk.h>

extern void core_wakeup(void);
extern void PUT32 ( unsigned int, unsigned int );

core_start_fn_t core_start_fn[NUM_CORES] = { NULL, NULL, NULL, NULL};

extern uint32_t CPUID(void);
void core_init(void)
{
    // Core initialization
#if( configUSE_GLOBAL_EDF == 1 )
    // Do not print anything before mmu is enabled, or else the prints
    // will be messed up
    mmu_init();
    core_start_fn[CPUID()]();
#else /* configUSE_GLOBAL_EDF */
    rpi_cpu_irq_disable();
    rpi_core_timer_init();
    mmu_init();
    printk("Core Initialized\r\n");

    core_start_fn[CPUID()]();
#endif /* configUSE_GLOBAL_EDF */

    // We should never return from the start function
    printk("Returned from core start function, panic...\r\n");
    configASSERT(pdFALSE);
}

void core_set_start_fn( uint32_t cpuid, core_start_fn_t fn )
{
    core_start_fn[cpuid] = fn;
}


void core_start( uint8_t core_num )
{
    if ( core_start_fn == NULL ) {
        printk("Cannot start core unless core start function is set! Ignoring...\r\n");
        return;
    }
    PUT32( ( 0x4000008C + 0x10 * core_num ), (unsigned int) core_wakeup );
}

void core_enable_mailbox_interrupt( uint32_t mailbox_num ) {
    uint32_t val = MMIO_READ(REG_BASE + MAILBOX_INT_OFFSET + (MAILBOX_OFFSET * CPUID()));
    val |= (1 << mailbox_num);
    MMIO_WRITE(REG_BASE + MAILBOX_INT_OFFSET + (MAILBOX_OFFSET * CPUID()), val);
}

void core_clear_mailbox_interrupt( uint32_t mailbox_num ) {
    MMIO_WRITE(REG_BASE + MAILBOX_RDCLR_OFFSET + (CORE_OFFSET * CPUID()) + (MAILBOX_OFFSET * mailbox_num), 0xffffffff);
}

void core_send_interrupt( uint32_t cpuid, uint32_t mailbox_num, uint32_t val )
{
    // Send interrupt to core with cpuid.
    MMIO_WRITE(REG_BASE + MAILBOX_SET_OFFSET + (CORE_OFFSET * cpuid) + (MAILBOX_OFFSET * mailbox_num), val);    
}

uint32_t core_get_mailbox_interrupt_source(void) {
    uint32_t val = MMIO_READ(REG_BASE + INT_SOURCE_OFFSET + (MAILBOX_OFFSET * CPUID()));
    return (val >> 4) & 0xF;
}

uint32_t core_read_mailbox( uint32_t mailbox_num )
{
    return MMIO_READ(REG_BASE + MAILBOX_RDCLR_OFFSET + (CORE_OFFSET * CPUID()) + (MAILBOX_OFFSET * mailbox_num));
}
   
void smp_init() {
    printk("Starting core 1\r\n");
    core_start( 1 );

    printk("Starting core 2\r\n");    
    core_start( 2 );

    printk("Starting core 3\r\n");    
    core_start( 3 );
}
