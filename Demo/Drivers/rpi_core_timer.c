#include <stdint.h>
#include <stdbool.h>

#include <FreeRTOS.h>
#include <Drivers/rpi_core_timer.h>
#include <Drivers/rpi_memory.h>


#define RPI_CORE_TIMER_BASE         ( 0x40000000 )

#define RPI_CORE_TIMER_IRQCNTL      ( RPI_CORE_TIMER_BASE + 0x40 )
#define RPI_CORE_TIMER_IRQ_OFFT     ( 0x4 )
#define RPI_CORE_TIMER_IRQCNTL_V    ( 0x8 )

#define RPI_CORE_IRQ_SRC            ( RPI_CORE_TIMER_BASE + 0x60 )
#define RPI_CORE_IRQ_SRC_OFFT       ( 0x4 )


uint32_t CPUID(void);

void enable_cntv(void)
{
    uint32_t cntv_ctl;
    cntv_ctl = 1;
    asm volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}

void disable_cntv(void)
{
    uint32_t cntv_ctl;
    cntv_ctl = 0;
    asm volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}

uint64_t read_cntvct(void)
{
    uint64_t val;
    asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (val));
    return (val);
}

uint64_t read_cntvoff(void)
{
    uint64_t val;
    asm volatile("mrrc p15, 4, %Q0, %R0, c14" : "=r" (val));
    return (val);
}

uint32_t read_cntv_tval(void)
{
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c14, c3, 0" : "=r"(val) );
    return val;
}

void write_cntv_tval(uint32_t val)
{
    asm volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(val) );
    return;
}

uint32_t read_cntfrq(void)
{
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
    return val;
}

uint32_t rpi_core_timer_data[CORES] = {0};

void rpi_core_timer_route_virq( uint32_t core )
{
    configASSERT( ( core >= 0 ) && ( core < CORES ) );
    MMIO_WRITE( RPI_CORE_TIMER_IRQCNTL + ( core * RPI_CORE_TIMER_IRQ_OFFT ),
                RPI_CORE_TIMER_IRQCNTL_V);
}

bool rpi_core_timer_pending( void )
{
    uint32_t core = CPUID();
    return MMIO_READ( RPI_CORE_IRQ_SRC + RPI_CORE_IRQ_SRC_OFFT * core ) == RPI_CORE_TIMER_IRQCNTL_V;
}

void rpi_core_timer_init( void )
{
    //configASSERT( rpi_core_timer_data[ CPUID() ] == 0 );
    rpi_core_timer_data[ CPUID() ] = read_cntfrq() / 16000;

    // route timer interrupts to relavent core
    rpi_core_timer_route_virq( CPUID() );
}


void rpi_core_timer_setup_interrupts( void )
{
    rpi_core_timer_clear_interrupts();
    enable_cntv();
}

void rpi_core_timer_clear_interrupts(void)
{
    uint32_t timer_val = rpi_core_timer_data[ CPUID() ];
    configASSERT( timer_val )
    write_cntv_tval( timer_val );
}
