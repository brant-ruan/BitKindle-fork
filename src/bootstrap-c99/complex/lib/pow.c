#include "src/bootstrap-c99/complex/lib/internal.h"
#include "src/bootstrap-c99/include/math.h"
/* Principal power. Purely real exponents avoid an artificial zero times an
   infinite log component; the nonnegative real axis uses the real pow path. */
#define CX_POW(R,S) \
R complex cpow##S(R complex z,R complex w){ \
    R x=creal##S(z),y=cimag##S(z),u=creal##S(w),v=cimag##S(w); \
    if(u==0&&v==0)return __cx_pair##S(1,0); \
    if(v==0){ \
        if(y==0&&!signbit(x)&&!isnan(x)&&isfinite(u)) \
            return __cx_pair##S(pow##S(x,u),u<0?-y:y); \
        return cexp##S(u*clog##S(z)); \
    } \
    return cexp##S(w*clog##S(z)); \
}
CX_POW(float, f)
CX_POW(double, )
CX_POW(long double, l)
