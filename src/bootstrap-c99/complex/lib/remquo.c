#include "src/bootstrap-c99/include/math.h"
#include <errno.h>
#include <fenv.h>

/* Exact integer significand division; retain seven quotient magnitude bits.
   No finite floating arithmetic and no dependence on the current rounding mode. */
static unsigned long long __rq_bits(unsigned long long x, unsigned long long y, int type,
                                    int *quo) {
    int p = type == 13 ? 23 : 52, bias = type == 13 ? 127 : 1023, ex, ey, diff, bit, negative;
    unsigned int q = 0;
    unsigned long long sign = type == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL;
    unsigned long long ax = x & (sign - 1), ay = y & (sign - 1), leading = 1ULL << p, mx, my, r,
                       rs = x & sign;
    *quo = 0;
    negative = ((x ^ y) & sign) != 0;
    if (ax > infinity || ay > infinity) {
        if ((ax > infinity && !(ax & quiet)) || (ay > infinity && !(ay & quiet)))
            feraiseexcept(FE_INVALID);
        return (ax > infinity ? x : y) | quiet;
    }
    if (!ay || ax == infinity) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
    if (!ax || ay == infinity)
        return x;
    ex = (int)(ax >> p) - bias;
    mx = (ax & (leading - 1)) | leading;
    if (!(ax >> p)) {
        ex = 1 - bias;
        mx = ax;
        while (mx < leading) {
            mx <<= 1;
            ex--;
        }
    }
    ey = (int)(ay >> p) - bias;
    my = (ay & (leading - 1)) | leading;
    if (!(ay >> p)) {
        ey = 1 - bias;
        my = ay;
        while (my < leading) {
            my <<= 1;
            ey--;
        }
    }
    if (ex < ey) {
        if (ey - ex > 1 || mx <= my)
            return x;
        r = (my << 1) - mx;
        rs ^= sign;
        q = 1;
    } else {
        r = mx;
        for (diff = ex - ey; diff >= 0; diff--) {
            bit = r >= my;
            if (bit)
                r -= my;
            q = ((q << 1) + (unsigned int)bit) & 127U;
            if (diff)
                r <<= 1;
        }
        ex = ey;
        if ((r << 1) > my || ((r << 1) == my && (q & 1))) {
            r = my - r;
            rs ^= sign;
            q = (q + 1) & 127U;
        }
    }
    *quo = negative ? -(int)q : (int)q;
    if (!r)
        return x & sign;
    while (r < leading) {
        r <<= 1;
        ex--;
    }
    if (ex + bias > 0)
        return rs | ((unsigned long long)(ex + bias) << p) | (r & (leading - 1));
    return rs | (r >> (1 - bias - ex));
}

#define REMQUO(T,U,S,K) \
T remquo##S(T x,T y,int *q){union{T f;U u;}a,b;a.f=x;b.f=y;a.u=(U)__rq_bits(a.u,b.u,K,q);return a.f;} \
T remainder##S(T x,T y){int q;return remquo##S(x,y,&q);}
REMQUO(float, unsigned int, f, 13)
REMQUO(double, unsigned long long, , 14)
REMQUO(long double, unsigned long long, l, 15)
#undef REMQUO
