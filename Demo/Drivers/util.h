#ifndef _UTIL_H_
#define _UTIL_H_

#include "stdarg.h"

/**
 * @return 0 on success or -1 on failure
 */
int printk(const char *format, ...);

#endif /* _UTIL_H_ */
