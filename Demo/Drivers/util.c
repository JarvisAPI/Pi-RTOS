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
#include "rpi_aux.h"
#include "util.h"

#define uart_put_byte rpi_aux_mu_putc

unsigned int Divide (unsigned int nDividend, unsigned int nDivisor, unsigned int *pRest)
{
	if (nDivisor == 0)
	{
        // Divide by zero should throw exception, but there is no such thing as exception
        // Return -1 for largest unsigned number, behavior is undefined at this point
        return -1;
	}
	
	unsigned long long ullDivisor = nDivisor;

	unsigned int nCount = 1;
	while (nDividend > ullDivisor)
	{
		ullDivisor <<= 1;
		nCount++;
	}

	unsigned int nQuotient = 0;
	while (nCount--)
	{
		nQuotient <<= 1;

		if (nDividend >= ullDivisor)
		{
			nQuotient |= 1;
			nDividend -= ullDivisor;
		}

		ullDivisor >>= 1;
	}
	
	if (pRest != 0)
	{
		*pRest = nDividend;
	}
	
	return nQuotient;
}

/**
 * @brief helper for printing a number in a given base
 *
 * @param base 8, 10, 16
 * @param num > 0
 */
static void printnum_helper(uint8_t base, uint64_t num) {
    char digit;
    if (num > 0) {
        printnum_helper(base, num / base);
        digit = num % base;
        if (digit < 10) {
            uart_put_byte('0' + digit);
        } else {
            uart_put_byte('a' + (digit - 10));
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
        uart_put_byte('0');
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
            case 'o': printnumk(8, va_arg(vargs, uint32_t)); break;
            case 'd': {
                int64_t num = va_arg(vargs, int32_t);
                if (num < 0) {
                    uart_put_byte('-');
                    num *= -1;
                }
                printnumk(10, num);
                break;
            }
            case 'u': printnumk(10, va_arg(vargs, uint32_t)); break;
            case 'p': {
                uart_put_byte('0');
                uart_put_byte('x');
                printnumk(16, va_arg(vargs, uint32_t)); break;
            }
            case 'x': printnumk(16, va_arg(vargs, uint32_t)); break;
            case 'c': uart_put_byte(va_arg(vargs, uint32_t)); break;
            case 's': {
                char *str = va_arg(vargs, char *);
                while (*str) {
                    uart_put_byte(*str);
                    str++;
                }
                break;
            }
            case '%': uart_put_byte('%'); break;
            default:
                va_end(vargs);                
                return -1;
            }
            fmt++;
        } else {
            uart_put_byte(*fmt);
        }
        fmt++;
    }
    va_end(vargs);
    return 0;
}
