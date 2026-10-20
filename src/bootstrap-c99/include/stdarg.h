#if defined(__C1_C89__) && !defined(__C1_LIBRARY_EXTENSIONS)
#ifndef __C1_STDARG_H
#define __C1_STDARG_H
typedef char *va_list;
#define va_start(ap,last) ((void)((ap)=__va_start(last)))
#define va_arg(ap,type) (__va_arg(&(ap),type))
#define va_end(ap) ((void)((ap)=(va_list)0))
#endif
#else
#include "src/bootstrap-c99/base/include/stdarg.h"
#endif
