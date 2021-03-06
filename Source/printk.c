/**
 * @file printk.c
 *
 * @brief printf() implementation for KERNEL using UART
 *
 * @date   9/23/2018
 * @author william ou (ouhaoqian@hotmail.com)
 * @author zeyad tamimi (zeyadtamimi@outlook.com)
 * @author Leo Zhang (lyze96@gmail.com)
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdbool.h>
#include <Drivers/rpi_aux.h>
#include "FreeRTOS.h"

static Mutex_t mPrintLock = portMUTEX_INIT_VAL;

#if ( configUSE_MULTICORE == 1 )
    static bool bCorePrint = true;
#endif /* configUSE_MULTICORE */

const char* cpu_colors[] =
{
    "[34;1m",
    "[35;1m",
    "[36;1m",
    "[38;5;57;1m",
};

extern uint32_t CPUID(void);

/**
 * @brief helper for printing a number in a given base
 *
 * @param base 8, 10, 16
 * @param num > 0
 */
static void printnum_helper(uint8_t base, uint32_t num) {
    char digit;
    if (num > 0) {
        printnum_helper(base, num / base);
        digit = num % base;
        if (digit < 10) {
            rpi_aux_mu_putc('0' + digit);
        } else {
            rpi_aux_mu_putc('a' + (digit - 10));
        }
    }
}

/**
 * @brief prints a number
 *
 * @param base 8, 10, 16
 * @param num the number to print
 */
static void printnumk(uint8_t base, uint32_t num) {
    if (num == 0) {
        rpi_aux_mu_putc('0');
        return;
    }
    printnum_helper(base, num);
}

int printk(const char *fmt, ...) {
    va_list vargs;
    int retval = 0;
    
    va_start(vargs, fmt);

    if (portMMU_IS_ENABLED()) {
        portMUTEX_ACQUIRE( &mPrintLock );
    }
    
#if ( configUSE_MULTICORE == 1 )
    if ( bCorePrint )
    {
        rpi_aux_mu_string("\033");
        rpi_aux_mu_string(cpu_colors[CPUID()]);
        rpi_aux_mu_string("[ Core ");
        printnumk( 10, CPUID() );
        rpi_aux_mu_string(" ]\033[0m ");
    }
#endif /* configUSE_MULTICORE */
    
    while(*fmt) {
        if (*fmt == '%') {
            switch(*(fmt+1)) {
            case 'o': printnumk(8, va_arg(vargs, uint32_t)); break;
            case 'd': {
                int32_t num = va_arg(vargs, int32_t);
                if (num < 0) {
                    rpi_aux_mu_putc('-');
                    num *= -1;
                }
                printnumk(10, num);
                break;
            }
            case 'u': printnumk(10, va_arg(vargs, uint32_t)); break;
            case 'p': {
                rpi_aux_mu_putc('0');
                rpi_aux_mu_putc('x');
                printnumk(16, va_arg(vargs, uint32_t)); break;
            }
            case 'x': printnumk(16, va_arg(vargs, uint32_t)); break;
            case 'c': rpi_aux_mu_putc(va_arg(vargs, uint32_t)); break;
            case 's': {
                char *str = va_arg(vargs, char *);
                while (*str) {
                    rpi_aux_mu_putc(*str);
                    str++;
                }
                break;
            }
            case '%': rpi_aux_mu_putc('%'); break;
            default:
                retval = -1;
                break;
            }
            fmt++;
        } else {
            rpi_aux_mu_putc(*fmt);
        }
        fmt++;
    }
    
    if (portMMU_IS_ENABLED()) {
        portMUTEX_RELEASE( &mPrintLock );
    }
    
    va_end(vargs);

    return retval;
}


#if ( configUSE_MULTICORE == 1 )

void vToggleCorePrint( void )
{
    if ( portMMU_IS_ENABLED() )
        portMUTEX_ACQUIRE( &mPrintLock );

    bCorePrint = !bCorePrint;

    if ( portMMU_IS_ENABLED() )
        portMUTEX_RELEASE( &mPrintLock );
}

void vSetCorePrint( bool value )
{
    if ( portMMU_IS_ENABLED() )
        portMUTEX_ACQUIRE( &mPrintLock );

    bCorePrint = value;

    if ( portMMU_IS_ENABLED() )
        portMUTEX_RELEASE( &mPrintLock );
}

#endif

void vNPrint( char c, unsigned int num )
{
    for ( unsigned int i = 0; i < num; i++ )
        printk( "%c", c);
}

void vPrintClear( void )
{
    printk("\033[1J");
}

void vPrintSaveCursor( void )
{
    printk("\033[s");
}

void vPrintRestoreCursor( void )
{
    printk("\033[u");
}

void vPrintCursorMoveRight( uint32_t move )
{
    printk("\033[%uC", move);
}
