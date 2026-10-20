#include "src/bootstrap-c99/complex/lib/internal.h"
#include "src/bootstrap-c99/include/math.h"
/* Scale a bounded trigonometric factor by cosh/sinh without materializing
   an overflowing hyperbolic result. */
#define CX_SINCOS(R,S) \
static R __cx_hyper_part##S(R q,R y,int odd){ \
    R a=fabs##S(y),e;if(odd&&signbit(y))q=-q; \
    if(q==0)return q; \
    if(isinf(a))return copysign##S(a,q); \
    if(a<=20)return q*(odd?sinh##S(a):cosh##S(a)); \
    e=exp##S(a/4);return (((q*(e/2))*e)*e)*e; \
} \
R complex csin##S(R complex z){ \
    R x=creal##S(z),y=cimag##S(z),s=sin##S(x),c; \
    if(isinf(y)&&!isfinite(x))return __cx_pair##S(s,y); \
    if(y==0&&!isfinite(x))return __cx_pair##S(s,y); \
    c=cos##S(x);return __cx_pair##S(__cx_hyper_part##S(s,y,0),__cx_hyper_part##S(c,y,1)); \
} \
R complex ccos##S(R complex z){ \
    R x=creal##S(z),y=cimag##S(z),c=cos##S(x),s; \
    if(isinf(y)&&!isfinite(x))return __cx_pair##S(fabs##S(y),c); \
    if(y==0&&!isfinite(x))return __cx_pair##S(c,y); \
    s=sin##S(x);return __cx_pair##S(__cx_hyper_part##S(c,y,0),-__cx_hyper_part##S(s,y,1)); \
} \
R complex csinh##S(R complex z){ \
    R complex v=csin##S(__cx_pair##S(-cimag##S(z),creal##S(z))); \
    return __cx_pair##S(cimag##S(v),-creal##S(v)); \
} \
R complex ccosh##S(R complex z){ \
    return ccos##S(__cx_pair##S(-cimag##S(z),creal##S(z))); \
}
CX_SINCOS(float, f)
CX_SINCOS(double, )
CX_SINCOS(long double, l)
