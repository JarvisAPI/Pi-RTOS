/**
 * @file   printk.h
 *
 * @brief  kernel debugging routine
 *
 * @date   9/9/2018
 * @author William Ou (ouhaoqian@hotmail.com)
 * @author Zeyad Tamimi (ZeyadTamimi@Outlook.com)
 * @author Leo Zhang (lyze96@gmail.com)
 */

#ifndef _ZPRINTK_H_
#define _ZPRINTK_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define COLOR_RED "[31;1m"
#define COLOR_GREEN "[32;1m"
#define COLOR_YELLOW "[33;1m"
#define COLOR_END "\033[m"

/**
 * @brief A kernel printf() function for debugging the kernel.
 *   Note: The format arguments must be 64 bit values except for
 *   char which is 32 bit.
 *
 * @param format the format string
 * @return 0 on success or -1 on failure
 */
int printk(const char *format, ...);

void vToggleCorePrint( void );
void vSetCorePrint( bool value );
void vNPrint( char c, unsigned int num );
void vPrintClear( void );
void vPrintSaveCursor( void );
void vPrintRestoreCursor( void );
void vPrintCursorMoveRight( uint32_t move );
#endif /* _PRINTK_H_ */
