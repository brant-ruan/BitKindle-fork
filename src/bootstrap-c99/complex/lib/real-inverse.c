#include "src/bootstrap-c99/include/math.h"
#include <errno.h>
#include <fenv.h>

static unsigned long long __ri_tiny(unsigned long long b, int type, int tangent) {
    unsigned long long sign = type == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
    unsigned long long a = b & (sign - 1), minimum = type == 13 ? 0x800000ULL : 0x10000000000000ULL;
    int negative = (b & sign) != 0, mode = fegetround();
    if (tangent) {
        if ((!negative && mode == FE_UPWARD) || (negative && mode == FE_DOWNWARD))
            a++;
    } else if (mode == FE_TOWARDZERO || (!negative && mode == FE_DOWNWARD) ||
               (negative && mode == FE_UPWARD))
        a--;
    if (a < minimum) {
        errno = ERANGE;
        feraiseexcept(FE_UNDERFLOW);
    }
    feraiseexcept(FE_INEXACT);
    return a | (b & sign);
}

#define RI(T,U,S,K,SIGN,INF,QUIET,TINY,LIMIT,LN2) \
static T __ri_eval##S(T x,int op){ \
 union{T f;U u;}v;U a;T y,t,r;int mode,flip; \
 v.f=x;a=v.u&((U)(SIGN)-1); \
 if(a>(U)(INF)){if(!(a&(U)(QUIET)))feraiseexcept(FE_INVALID);v.u|=(U)(QUIET);return v.f;} \
 if(op==1&&x<1){errno=EDOM;feraiseexcept(FE_INVALID);v.u=(U)(INF)|(U)(QUIET);return v.f;} \
 if(op==2&&fabs##S(x)>1){errno=EDOM;feraiseexcept(FE_INVALID);v.u=(U)(INF)|(U)(QUIET);return v.f;} \
 if(op==2&&fabs##S(x)==1){errno=ERANGE;feraiseexcept(FE_DIVBYZERO);v.u=(v.u&(U)(SIGN))|(U)(INF);return v.f;} \
 if(a==(U)(INF))return x; \
 if(op==1&&x==1)return 0; \
 if(op!=1&&!a)return x; \
 if(op!=1&&a<(U)(TINY)){v.u=(U)__ri_tiny(v.u,K,op==2);return v.f;} \
 y=fabs##S(x);mode=fegetround();flip=op!=1&&(v.u&(U)(SIGN))&&(mode==FE_UPWARD||mode==FE_DOWNWARD); \
 if(flip)fesetround(mode==FE_UPWARD?FE_DOWNWARD:FE_UPWARD); \
 if(op==2)r=(T)0.5*log1p##S(((T)2*y)/((T)1-y)); \
 else if(y>(T)(LIMIT))r=log##S(y)+(T)(LN2); \
 else if(op==1){t=x-(T)1;r=log1p##S(t+sqrt##S(t)*sqrt##S(x+(T)1));} \
 else r=log1p##S(y+y*y/((T)1+sqrt##S((T)1+y*y))); \
 if(flip)fesetround(mode); \
 return op==1?r:copysign##S(r,x); \
} \
T asinh##S(T x){return __ri_eval##S(x,0);} \
T acosh##S(T x){return __ri_eval##S(x,1);} \
T atanh##S(T x){return __ri_eval##S(x,2);}
RI(float, unsigned int, f, 13, 0x80000000U, 0x7f800000U, 0x400000U, 0x39000000U, 0x1p12f,
   0x1.62e43p-1f)
RI(double, unsigned long long, , 14, 0x8000000000000000ULL, 0x7ff0000000000000ULL,
   0x8000000000000ULL, 0x3e30000000000000ULL, 0x1p28, 0x1.62e42fefa39efp-1)
RI(long double, unsigned long long, l, 15, 0x8000000000000000ULL, 0x7ff0000000000000ULL,
   0x8000000000000ULL, 0x3e30000000000000ULL, 0x1p28L, 0x1.62e42fefa39efp-1L)
#undef RI
