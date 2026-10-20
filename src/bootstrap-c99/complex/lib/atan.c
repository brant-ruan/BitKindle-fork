#include "src/bootstrap-c99/complex/lib/internal.h"
#include "src/bootstrap-c99/include/math.h"
#define CX_ATAN(R,S,TINY) \
R complex catanh##S(R complex z){ \
    R x=creal##S(z),y=cimag##S(z),r,i,d,p=(R)1.57079632679489661923132169164L;R complex a,b; \
    if(isinf(x)||isinf(y))return __cx_pair##S(copysign##S(0,x),isnan(y)?y:copysign##S(p,y)); \
    if(isnan(x)||isnan(y))return __cx_pair##S(x==0?x:(R)NAN,(R)NAN); \
    if(fabs##S(x)>0x1p30||fabs##S(y)>0x1p30){ \
        if(fabs##S(x)>=fabs##S(y)){ \
            d=y/x;r=(1/x)/(1+d*d);i=(d/x)/(1+d*d); \
        }else{ \
            d=x/y;r=(d/y)/(1+d*d);i=(1/y)/(1+d*d); \
        } \
        return __cx_pair##S(r,copysign##S(p,y)-i); \
    } \
    if(fabs##S(x)<TINY&&fabs##S(y)<TINY)return z; \
    if(fabs##S(x)<=0.5&&fabs##S(y)<=0.5){ \
        d=(1-x)*(1-x)+y*y;r=log1p##S(4*x/d)/4; \
        i=atan2##S(2*y,1-x*x-y*y)/2;return __cx_pair##S(r,i); \
    } \
    a=clog##S(__cx_pair##S(1+x,y));b=clog##S(__cx_pair##S(1-x,-y)); \
    r=(creal##S(a)-creal##S(b))/2;i=(cimag##S(a)-cimag##S(b))/2; \
    if(r==0)r=copysign##S(0,x);if(i==0)i=copysign##S(0,y); \
    return __cx_pair##S(r,i); \
} \
R complex catan##S(R complex z){ \
    R complex v=catanh##S(__cx_pair##S(-cimag##S(z),creal##S(z))); \
    return __cx_pair##S(cimag##S(v),-creal##S(v)); \
}
CX_ATAN(float, f, 0x1p-13f)
CX_ATAN(double, , 0x1p-28)
CX_ATAN(long double, l, 0x1p-28L)
