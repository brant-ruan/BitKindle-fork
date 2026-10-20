#if defined(__C1_C89__) && !defined(__C1_LIBRARY_EXTENSIONS)
#ifndef __C1_CTYPE_H
#define __C1_CTYPE_H
int isalnum(int __c1_c);
int isalpha(int __c1_c);
int iscntrl(int __c1_c);
int isdigit(int __c1_c);
int isgraph(int __c1_c);
int islower(int __c1_c);
int isprint(int __c1_c);
int ispunct(int __c1_c);
int isspace(int __c1_c);
int isupper(int __c1_c);
int isxdigit(int __c1_c);
int tolower(int __c1_c);
int toupper(int __c1_c);
#endif
#else
#include "src/bootstrap-c99/include/ctype-full.h"
#endif
