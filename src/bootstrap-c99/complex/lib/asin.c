#include "src/bootstrap-c99/complex/lib/internal.h"
#include "src/bootstrap-c99/include/math.h"
#define CX_ASIN(R,S,TINY) \
static R __cx_acosh_real##S(R a){ \
    if(a>0x1p30)return log##S(a)+(R)0.69314718055994530941723212146L; \
    return log1p##S((a-1)+sqrt##S(a-1)*sqrt##S(a+1)); \
} \
static R complex __cx_asin##S(R complex z,int cosine){ \
    R x=creal##S(z),y=cimag##S(z),ax=fabs##S(x),ay=fabs##S(y),u,v,a,b,r,i; \
    R p=(R)1.57079632679489661923132169164L; \
    if(isinf(x)||isinf(y)){ \
        if(isnan(x)||isnan(y))r=(R)NAN; \
        else if(isinf(x)&&isinf(y))r=cosine?(x<0?3*p/2:p/2):copysign##S(p/2,x); \
        else if(isinf(x))r=cosine?(x<0?2*p:0):copysign##S(p,x); \
        else r=cosine?p:copysign##S(0,x); \
        i=copysign##S((R)INFINITY,y);return __cx_pair##S(r,cosine?-i:i); \
    } \
    if(isnan(x)||isnan(y))return __cx_pair##S(x==0?(cosine?p:x):(R)NAN,(R)NAN); \
    if(y==0){ \
        if(ax<=1){r=cosine?acos##S(x):asin##S(x);i=y;} \
        else{r=cosine?(x<0?2*p:0):copysign##S(p,x);i=copysign##S(__cx_acosh_real##S(ax),y);} \
        return __cx_pair##S(r,cosine?-i:i); \
    } \
    if(ax>0x1p30||ay>0x1p30){ \
        r=cosine?atan2##S(ay,x):atan2##S(x,ay); \
        i=copysign##S(creal##S(clog##S(z))+(R)0.69314718055994530941723212146L,y); \
    }else if(ax<1&&ay<TINY*(1-ax)){ \
        r=cosine?acos##S(x):asin##S(x);i=y/sqrt##S((1-x)*(1+x)); \
    }else if(ax==1&&ay<TINY*TINY){ \
        b=sqrt##S(ay);r=cosine?(x<0?2*p-b:b):copysign##S(p-b,x); \
        i=copysign##S(b,y); \
    }else if(ax==1&&ay<0.25){ \
        u=cabs##S(__cx_pair##S(2,ay));a=(ay+ay*(ay/(u+2)))/2; \
        b=sqrt##S(a)*sqrt##S(a+2);i=copysign##S(log1p##S(a+b),y); \
        b=atan##S(b);r=cosine?(x<0?2*p-b:b):copysign##S(p-b,x); \
    }else if(ax>1&&ay<TINY*(ax-1)){ \
        b=ay/sqrt##S((ax-1)*(ax+1)); \
        r=cosine?(x<0?2*p-b:b):copysign##S(p-b,x); \
        i=copysign##S(__cx_acosh_real##S(ax),y); \
    }else{ \
        u=cabs##S(__cx_pair##S(x+1,y));v=cabs##S(__cx_pair##S(x-1,y)); \
        a=u/2+v/2;if(a<1)a=1;b=x/a;if(b>1)b=1;if(b< -1)b=-1; \
        r=cosine?acos##S(b):asin##S(b);i=copysign##S(__cx_acosh_real##S(a),y); \
    } \
    return __cx_pair##S(r,cosine?-i:i); \
} \
R complex casin##S(R complex z){return __cx_asin##S(z,0);} \
R complex cacos##S(R complex z){return __cx_asin##S(z,1);} \
R complex casinh##S(R complex z){ \
    R complex v=casin##S(__cx_pair##S(-cimag##S(z),creal##S(z))); \
    return __cx_pair##S(cimag##S(v),-creal##S(v)); \
} \
R complex cacosh##S(R complex z){ \
    R complex v=cacos##S(z);R r=creal##S(v),i=cimag##S(v); \
    return signbit(cimag##S(z))?__cx_pair##S(i,-r):__cx_pair##S(-i,r); \
}
CX_ASIN(float, f, 0x1p-13f)
CX_ASIN(double, , 0x1p-28)
CX_ASIN(long double, l, 0x1p-28L)
