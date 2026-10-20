#ifndef __C1_COMPLEX_INTERNAL_H
#define __C1_COMPLEX_INTERNAL_H
#include "src/bootstrap-c99/include/complex.h"
/* Assemble components without arithmetic that could alter NaNs or zero signs. */
#define CX_PAIR(R,S) \
static R complex __cx_pair##S(R r,R i){ \
    R a[2];R complex z;unsigned char *d=(unsigned char*)&z,*s=(unsigned char*)a;int n; \
    a[0]=r;a[1]=i;for(n=0;n<sizeof z;n++)d[n]=s[n];return z; \
}
CX_PAIR(float, f)
CX_PAIR(double, )
CX_PAIR(long double, l)
#undef CX_PAIR
#endif
