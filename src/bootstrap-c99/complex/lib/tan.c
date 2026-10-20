#include "src/bootstrap-c99/complex/lib/internal.h"
#include "src/bootstrap-c99/include/math.h"
#include <fenv.h>
/* Avoid doubled arguments and hyperbolic overflow. Near the real axis use
   cos(x)^2+sinh(y)^2, which avoids cancellation in cos(2x)+cosh(2y). */
#define CX_TAN(R,S) \
R complex ctan##S(R complex z){ \
    R x=creal##S(z),y=cimag##S(z),a=fabs##S(y),s,c,t,d,q,r,i; \
    if(y==0)return __cx_pair##S(tan##S(x),y); \
    if(isinf(y)){ \
        if(!isfinite(x))return __cx_pair##S(0,copysign##S(1,y)); \
        s=sin##S(x);c=cos##S(x);return __cx_pair##S(copysign##S(0,s*c),copysign##S(1,y)); \
    } \
    if(!isfinite(x)||isnan(y)){ \
        if(isinf(x))feraiseexcept(FE_INVALID);q=(R)NAN; \
        return __cx_pair##S(x==0?x:q,q); \
    } \
    s=sin##S(x);c=cos##S(x); \
    if(a>1){ \
        t=exp##S(-a);q=t*t;d=1+2*(c*c-s*s)*q+q*q; \
        r=4*s*c*q/d;i=copysign##S((1-q*q)/d,y); \
    }else{ \
        t=sinh##S(y);d=c*c+t*t;r=s*c/d;i=t*cosh##S(y)/d; \
    } \
    return __cx_pair##S(r,i); \
} \
R complex ctanh##S(R complex z){ \
    R complex v=ctan##S(__cx_pair##S(-cimag##S(z),creal##S(z))); \
    return __cx_pair##S(cimag##S(v),-creal##S(v)); \
}
CX_TAN(float, f)
CX_TAN(double, )
CX_TAN(long double, l)
