#ifndef __C1_WORK_HYPOT_CORE_H
#define __C1_WORK_HYPOT_CORE_H
#include "src/bootstrap-c99/include/math.h"
#include <errno.h>
#include <fenv.h>
/* Shared by real hypot and complex magnitude. Normalize before squaring;
   a widely separated smaller component only affects directed rounding. */
#define HYPOT_CORE(T,U,S,SIGN,INF,QUIET,GAP) \
static T __cx_hypot##S(T x,T y){ \
 union{T f;U u;}v,w;U ax,ay;T a,b,s,t,r;int ea,eb,raised;fenv_t saved; \
 v.f=x;w.f=y;ax=v.u&((U)(SIGN)-1);ay=w.u&((U)(SIGN)-1); \
 if((ax>(U)(INF)&&!(ax&(U)(QUIET)))||(ay>(U)(INF)&&!(ay&(U)(QUIET))))feraiseexcept(FE_INVALID); \
 if(ax==(U)(INF)||ay==(U)(INF)){v.u=(U)(INF);return v.f;} \
 if(ax>(U)(INF)||ay>(U)(INF)){if(ax<=(U)(INF))v.u=w.u;v.u|=(U)(QUIET);return v.f;} \
 v.u=ax;w.u=ay;a=v.f;b=w.f; \
 if(a<b){t=a;a=b;b=t;}if(b==0)return a; \
 feholdexcept(&saved);s=frexp##S(a,&ea);frexp##S(b,&eb); \
 if(ea-eb>GAP)r=a+b; \
 else{t=ldexp##S(b,-ea);r=ldexp##S(sqrt##S(s*s+t*t),ea);} \
 raised=fetestexcept(FE_ALL_EXCEPT);if(raised&(FE_OVERFLOW|FE_UNDERFLOW))errno=ERANGE; \
 feupdateenv(&saved);return r; \
}
HYPOT_CORE(float, unsigned int, f, 0x80000000U, 0x7f800000U, 0x400000U, 30)
HYPOT_CORE(double, unsigned long long, , 0x8000000000000000ULL, 0x7ff0000000000000ULL,
           0x8000000000000ULL, 60)
HYPOT_CORE(long double, unsigned long long, l, 0x8000000000000000ULL, 0x7ff0000000000000ULL,
           0x8000000000000ULL, 60)
#undef HYPOT_CORE
#endif
