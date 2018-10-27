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
#include <stdarg.h>
#include <Drivers/rpi_aux.h>

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
static void printnumk(uint8_t base, uint64_t num) {
    if (num == 0) {
        rpi_aux_mu_putc('0');
        return;
    }
    printnum_helper(base, num);
}

int printk(const char *fmt, ...) {
    va_list vargs;

    va_start(vargs, fmt);
    while(*fmt) {
        if (*fmt == '%') {
            switch(*(fmt+1)) {
            case 'o': printnumk(8, va_arg(vargs, uint64_t)); break;
            case 'd': {
                int64_t num = va_arg(vargs, int64_t);
                if (num < 0) {
                    rpi_aux_mu_putc('-');
                    num *= -1;
                }
                printnumk(10, num);
                break;
            }
            case 'u': printnumk(10, va_arg(vargs, uint64_t)); break;
            case 'p': {
                rpi_aux_mu_putc('0');
                rpi_aux_mu_putc('x');
                printnumk(16, va_arg(vargs, uint32_t)); break;
            }
            case 'x': printnumk(16, va_arg(vargs, uint64_t)); break;
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
                va_end(vargs);                
                return -1;
            }
            fmt++;
        } else {
            rpi_aux_mu_putc(*fmt);
        }
        fmt++;
    }
    va_end(vargs);

    return 0;
}
