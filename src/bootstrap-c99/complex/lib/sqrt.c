#include "src/bootstrap-c99/complex/lib/internal.h"
#include "src/bootstrap-c99/include/math.h"
/* Normalize with an even power of two. Recover the smaller component from
   the original imaginary input, avoiding subtraction cancellation. */
#define CX_SQRT(R,S) \
R complex csqrt##S(R complex z){ \
    R x=creal##S(z),y=cimag##S(z),a,b,m,t,q;int e; \
    if(isinf(y))return __cx_pair##S(fabs##S(y),y); \
    if(isinf(x)){ \
        if(x>0)return __cx_pair##S(x,isnan(y)?y:copysign##S(0,y)); \
        return __cx_pair##S(isnan(y)?y:0,copysign##S(-x,y)); \
    } \
    if(isnan(x)||isnan(y)){q=x+y;return __cx_pair##S(q,q);} \
    if(y==0){ \
        if(x<0)return __cx_pair##S(0,copysign##S(sqrt##S(-x),y)); \
        return __cx_pair##S(sqrt##S(fabs##S(x)),y); \
    } \
    m=fabs##S(x);if(fabs##S(y)>m)m=fabs##S(y);frexp##S(m,&e); \
    if(e%2)e--;a=ldexp##S(x,-e);b=ldexp##S(y,-e); \
    t=sqrt##S((fabs##S(a)+cabs##S(__cx_pair##S(a,b)))/2);t=ldexp##S(t,e/2); \
    if(x>=0)return __cx_pair##S(t,y/(2*t)); \
    return __cx_pair##S(fabs##S(y)/(2*t),copysign##S(t,y)); \
}
CX_SQRT(float, f)
CX_SQRT(double, )
CX_SQRT(long double, l)
