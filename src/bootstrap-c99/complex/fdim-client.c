#include <math.h>
#include <fenv.h>
#include <errno.h>

static double frombits(unsigned long long b) {
    double v;
    unsigned char *s = (unsigned char *)&b, *d = (unsigned char *)&v;
    int k;
    for (k = 0; k < 8; k++)
        d[k] = s[k];
    return v;
}

#define EXTRA(T,U,S,SIGN,INF,Q) \
static int extra##S(void){union{T f;U u;}a,b,r; \
 a.u=(U)(SIGN)|(U)(INF)|(U)(Q)|123;b.u=(U)(INF)|321; \
 feclearexcept(FE_ALL_EXCEPT);r.f=fdim##S(a.f,b.f); \
 if(r.u!=a.u||!fetestexcept(FE_INVALID))return 1; \
 feclearexcept(FE_ALL_EXCEPT);r.f=fdim##S((T)1,a.f); \
 if(r.u!=a.u||fetestexcept(FE_ALL_EXCEPT))return 2; \
 a.u=2;b.u=1;errno=0;feclearexcept(FE_ALL_EXCEPT);r.f=fdim##S(a.f,b.f); \
 if(r.u!=1||errno||fetestexcept(FE_UNDERFLOW|FE_INEXACT|FE_INVALID))return 3; \
 a.u=(U)(SIGN);b.u=0;r.f=fdim##S(a.f,b.f);if(r.u!=0)return 4; \
 a.u=(U)(INF);r.f=fdim##S(a.f,a.f);if(r.u!=0)return 5;return 0;}
EXTRA(float, unsigned int, f, 0x80000000U, 0x7f800000U, 0x400000U)
EXTRA(double, unsigned long long, , 0x8000000000000000ULL, 0x7ff0000000000000ULL,
      0x8000000000000ULL)
EXTRA(long double, unsigned long long, l, 0x8000000000000000ULL, 0x7ff0000000000000ULL,
      0x8000000000000ULL)
#undef EXTRA

int main(void) {
    double inf = INFINITY, q = frombits(0x7ff8000000000001ULL),
           sn = frombits(0x7ff0000000000001ULL), r;
    int i, j, k, modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    for (i = -20; i <= 20; i++)
        for (j = -20; j <= 20; j++)
            if (fdim(i, j) != (i > j ? i - j : 0) || fdimf(i, j) != (i > j ? i - j : 0) ||
                fdiml(i, j) != (i > j ? i - j : 0))
                return 1;
    if (signbit(fdim(-0.0, 0)) || signbit(fdim(0, -0.0)) || signbit(fdim(-1, 1)))
        return 2;
    feclearexcept(FE_ALL_EXCEPT);
    if (fdim(inf, inf) != 0 || fdim(-inf, -inf) != 0 || fdim(inf, 1) != inf ||
        fdim(1, -inf) != inf || fdim(1, inf) != 0 || fetestexcept(FE_ALL_EXCEPT))
        return 3;
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(fdim(q, 1)) || !isnan(fdim(1, q)) || errno || fetestexcept(FE_INVALID))
        return 4;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(fdim(q, sn)) || !fetestexcept(FE_INVALID))
        return 5;
    for (k = 0; k < 4; k++) {
        fesetround(modes[k]);
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = fdim(0x1.fffffffffffffp1023, -0x1.fffffffffffffp1023);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffffffffffp1023 : inf) || errno != ERANGE ||
            (fetestexcept(FE_OVERFLOW | FE_INEXACT)) != (FE_OVERFLOW | FE_INEXACT))
            return 6;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = fdimf(0x1.fffffep127f, -0x1.fffffep127f);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffep127 : inf) || errno != ERANGE ||
            !fetestexcept(FE_OVERFLOW))
            return 7;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = fdiml(0x1.fffffffffffffp1023L, -0x1.fffffffffffffp1023L);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffffffffffp1023 : inf) || errno != ERANGE ||
            !fetestexcept(FE_OVERFLOW))
            return 8;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = fdim(1, -0x1p-53);
        if (r != (k == 2 ? 0x1.0000000000001p0 : 1) || errno || !fetestexcept(FE_INEXACT))
            return 9;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = fdim(0x1p-1073, 0x1p-1074);
        if (r != 0x1p-1074 || errno || fetestexcept(FE_UNDERFLOW | FE_INEXACT | FE_INVALID))
            return 10;
        if (fegetround() != modes[k])
            return 11;
    }
    fesetround(FE_TONEAREST);
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_OVERFLOW);
    errno = 0;
    r = fdim(3, 1);
    if (r != 2 || errno || !fetestexcept(FE_OVERFLOW))
        return 12;
    if (extraf() || extra() || extral())
        return 13;
    return 0;
}
