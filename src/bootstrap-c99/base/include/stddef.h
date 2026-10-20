#ifndef __C1_STDDEF_H
#define __C1_STDDEF_H
typedef unsigned long long size_t;
typedef long long ptrdiff_t;
typedef unsigned short wchar_t;
#define NULL ((void *)0)
#define offsetof(type, member) __offsetof(type, member)
#endif
