#include "src/bootstrap-c99/complex/lib/internal.h"
#include "src/bootstrap-c99/include/math.h"
/* Compute log magnitude without materializing a possibly overflowing modulus.
   Around the unit circle, log1p retains the small real logarithm. */
#define CX_LOG(R,S) \
R complex clog##S(R complex z){ \
    R a=fabs##S(creal##S(z)),b=fabs##S(cimag##S(z)),t,r,angle=carg##S(z); \
    if(isinf(a)||isinf(b))r=isinf(a)?a:b; \
    else if(isnan(a)||isnan(b))r=a+b; \
    else{ \
        if(a<b){t=a;a=b;b=t;} \
        if(b==0)r=log##S(a); \
        else if(a>0.5&&a<1.5&&b<0.5)r=log1p##S((a-1)*(a+1)+b*b)/2; \
        else{t=b/a;r=log##S(a)+log1p##S(t*t)/2;} \
    } \
    return __cx_pair##S(r,angle); \
}
CX_LOG(float, f)
CX_LOG(double, )
CX_LOG(long double, l)
