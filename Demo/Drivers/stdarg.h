#ifndef _STDARG_H_
#define _STDARG_H_

#ifdef __GNUC__
  typedef __builtin_va_list va_list;
  #define va_start(v,l) __builtin_va_start(v,l)
  #define va_end(v)     __builtin_va_end(v)
  #define va_arg(v,l)   __builtin_va_arg(v,l)
#else
  #error "Compiler is not GNU and cannot use __builtin_va_list for stdarg.h"
#endif

#endif /* _STDARG_H_ */
