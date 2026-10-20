#include "src/bootstrap-c99/include/math.h"
#include <errno.h>
#include <fenv.h>
#define FDIM(T,U,S,SIGN,INF,QUIET) \
T fdim##S(T x,T y){ \
 union{T f;U u;}a,b;U ax,ay;T r;int raised;fenv_t saved; \
 a.f=x;b.f=y;ax=a.u&((U)(SIGN)-1);ay=b.u&((U)(SIGN)-1); \
 if(ax>(U)(INF)||ay>(U)(INF)){ \
  if((ax>(U)(INF)&&!(ax&(U)(QUIET)))||(ay>(U)(INF)&&!(ay&(U)(QUIET))))feraiseexcept(FE_INVALID); \
  if(ax<=(U)(INF))a.u=b.u;a.u|=(U)(QUIET);return a.f; \
 } \
 if(x<=y)return 0; \
 if(ax==(U)(INF)||ay==(U)(INF)){a.u=(U)(INF);return a.f;} \
 feholdexcept(&saved);r=x-y;raised=fetestexcept(FE_ALL_EXCEPT); \
 if(raised&(FE_OVERFLOW|FE_UNDERFLOW))errno=ERANGE; \
 feupdateenv(&saved);return r; \
}
FDIM(float, unsigned int, f, 0x80000000U, 0x7f800000U, 0x400000U)
FDIM(double, unsigned long long, , 0x8000000000000000ULL, 0x7ff0000000000000ULL, 0x8000000000000ULL)
FDIM(long double, unsigned long long, l, 0x8000000000000000ULL, 0x7ff0000000000000ULL,
     0x8000000000000ULL)
#undef FDIM
