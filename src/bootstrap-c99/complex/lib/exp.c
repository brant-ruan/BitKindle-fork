#include "src/bootstrap-c99/complex/lib/internal.h"
#include "src/bootstrap-c99/include/math.h"
#include <fenv.h>
#define CX_EXP(R,S,LIMIT) \
static R __cx_exp_part##S(R x,R q){ \
    R e;if(q==0)return q; \
    if(x<=LIMIT)return exp##S(x)*q; \
    e=exp##S(x/4);return (((q*e)*e)*e)*e; \
} \
R complex cexp##S(R complex z){ \
    R x=creal##S(z),y=cimag##S(z),c,s,q; \
    if(y==0)return __cx_pair##S(exp##S(x),y); \
    if(isinf(y)){ \
        if(isinf(x)&&x<0)return __cx_pair##S(0,0); \
        feraiseexcept(FE_INVALID);q=(R)NAN;return __cx_pair##S(isinf(x)?x:q,q); \
    } \
    if(isnan(y)){ \
        if(isinf(x))return x<0?__cx_pair##S(0,0):__cx_pair##S(x,y); \
        return __cx_pair##S(y,y); \
    } \
    if(isnan(x))return __cx_pair##S(x,x); \
    c=cos##S(y);s=sin##S(y); \
    if(isinf(x)){ \
        if(x<0)return __cx_pair##S(copysign##S(0,c),copysign##S(0,s)); \
        return __cx_pair##S(c==0?c:copysign##S(x,c),s==0?s:copysign##S(x,s)); \
    } \
    return __cx_pair##S(__cx_exp_part##S(x,c),__cx_exp_part##S(x,s)); \
}
CX_EXP(float, f, 80)
CX_EXP(double, , 700)
CX_EXP(long double, l, 700)
