#include <math.h>
#include <errno.h>
#include <fenv.h>
#include "src/bootstrap-c99/complex/remquo-reference.c"
#define EDGE(T,U,S,SIGN,INF,Q) \
static int edge##S(void){union{T f;U u;}a,b,r;int q=99; \
 a.u=(U)(SIGN)|(U)(INF)|123;b.u=(U)(INF)|(U)(Q)|321; \
 feclearexcept(FE_ALL_EXCEPT);r.f=remquo##S(a.f,b.f,&q); \
 if(r.u!=(a.u|(U)(Q))||q||!fetestexcept(FE_INVALID))return 1; \
 a.f=1;b.u=(U)(SIGN);errno=0;feclearexcept(FE_ALL_EXCEPT);r.f=remquo##S(a.f,b.f,&q); \
 if(!isnan(r.f)||q||errno!=EDOM||!fetestexcept(FE_INVALID))return 2; \
 a.u=(U)(SIGN);b.f=2;feclearexcept(FE_ALL_EXCEPT);r.f=remquo##S(a.f,b.f,&q); \
 if(r.u!=(U)(SIGN)||q||fetestexcept(FE_ALL_EXCEPT))return 3;return 0;}
EDGE(float, unsigned int, f, 0x80000000U, 0x7f800000U, 0x400000U)
EDGE(double, unsigned long long, , 0x8000000000000000ULL, 0x7ff0000000000000ULL, 0x8000000000000ULL)
EDGE(long double, unsigned long long, l, 0x8000000000000000ULL, 0x7ff0000000000000ULL,
     0x8000000000000ULL)
#undef EDGE

int main(void) {
    int x, y, n, r, q, k, e, modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    double a, b, v;
    for (k = 0; k < 4; k++) {
        fesetround(modes[k]);
        feclearexcept(FE_ALL_EXCEPT);
        for (x = -100; x <= 100; x++)
            for (y = -17; y <= 17; y++)
                if (y) {
                    n = x / y;
                    r = x - n * y;
                    if (2 * (r < 0 ? -r : r) > (y < 0 ? -y : y) ||
                        (2 * (r < 0 ? -r : r) == (y < 0 ? -y : y) && (n & 1)))
                        n += ((x < 0) != (y < 0)) ? -1 : 1;
                    r = x - n * y;
                    if (remquo(x, y, &q) != r || q != n || remainder(x, y) != r)
                        return 1;
                    if (remquof(x, y, &q) != r || q != n || remainderf(x, y) != r)
                        return 2;
                    if (remquol(x, y, &q) != r || q != n || remainderl(x, y) != r)
                        return 3;
                }
        if (fetestexcept(FE_ALL_EXCEPT) || fegetround() != modes[k])
            return 4;
        if (remquo_reference())
            return 15;
        if (remquo(1, 3 * 0x1p-1074, &q) != 0x1p-1074 || q != 85)
            return 5;
        if (remquo(-1, 3 * 0x1p-1074, &q) != -0x1p-1074 || q != -85)
            return 6;
        if (remquo(0x1.fffffffffffffp1023, 3 * 0x1p-1074, &q) != -0x1p-1074 || q != 43)
            return 7;
        if (remquo(10001, 3, &q) != -1 || q != 6)
            return 8;
        for (e = -1074; e <= 1019; e += 13) {
            a = ldexp(7, e);
            b = ldexp(2, e);
            feclearexcept(FE_ALL_EXCEPT);
            v = remquo(a, b, &q);
            if (v != -ldexp(1, e) || q != 4 || fetestexcept(FE_ALL_EXCEPT))
                return 9;
        }
    }
    fesetround(FE_TONEAREST);
    if (!signbit(remainder(-4, 2)) || signbit(remainder(4, -2)))
        return 10;
    if (remainder(3, INFINITY) != 3 || remquo(-3, INFINITY, &q) != -3 || q != 0)
        return 11;
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(remainder(1, 0)) || errno != EDOM || !fetestexcept(FE_INVALID))
        return 12;
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(remquo(INFINITY, 2, &q)) || errno != EDOM || !fetestexcept(FE_INVALID) || q != 0)
        return 13;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(remainder(NAN, 2)) || fetestexcept(FE_INVALID))
        return 14;
    if (edgef() || edge() || edgel())
        return 16;
    return 0;
}
