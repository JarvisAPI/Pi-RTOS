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

/**
 * @brief A kernel printf() function for debugging the kernel.
 *   Note: The format arguments must be 64 bit values except for
 *   char which is 32 bit.
 *
 * @param format the format string
 * @return 0 on success or -1 on failure
 */
int printk(const char *format, ...);

#endif /* _PRINTK_H_ */
