#include <math.h>
#include <fenv.h>
#include <errno.h>
#include "src/bootstrap-c99/complex/gamma-reference.c"

int main(void) {
    int i, k, m[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    double x, r;

    union {
        double f;
        unsigned long long u;
    } nan;

    if (fabs(tgamma(0.5) / 1.7724538509055160273 - 1) > 2e-15)
        return 1;
    if (fabs(tgamma(-0.5) / (-3.5449077018110320546) - 1) > 2e-15)
        return 2;
    if (fabs(lgamma(0.5) - 0.57236494292470008707) > 2e-15)
        return 3;
    if (fabs(lgamma(100) - 359.13420536957539878) > 1e-12)
        return 4;
    for (i = 1; i < 120; i++) {
        x = i / 8.0;
        if (fabs(tgamma(x + 1) / (x * tgamma(x)) - 1) > 2e-14)
            return 5;
    }
    for (k = 0; k < 4; k++) {
        fesetround(m[k]);
        i = gamma_reference(k);
        if (i)
            return 10000 + k * 2000 + i;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        if (tgamma(5) != 24 || tgammaf(5) != 24 || tgammal(5) != 24 || lgamma(1) != 0 ||
            lgamma(2) != 0 || errno || fetestexcept(FE_ALL_EXCEPT))
            return 6;
        r = tgamma(0x1p-100);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffffffffffp99 : 0x1p100))
            return 11;
        r = tgamma(-0x1p-100);
        if (r != (k == 1 ? -0x1.0000000000001p100 : -0x1p100))
            return 12;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        r = tgamma(-180.5);
        if (r != (k == 1 ? -0x1p-1074 : -0.0) || !signbit(r) || errno != ERANGE ||
            fetestexcept(FE_ALL_EXCEPT) != (FE_UNDERFLOW | FE_INEXACT))
            return 13;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        r = lgamma(0x1.fffffffffffffp1023);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffffffffffp1023 : INFINITY) || errno != ERANGE ||
            fetestexcept(FE_ALL_EXCEPT) != (FE_OVERFLOW | FE_INEXACT))
            return 14;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        r = tgamma(172);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffffffffffp1023 : INFINITY) || errno != ERANGE ||
            fetestexcept(FE_ALL_EXCEPT) != (FE_OVERFLOW | FE_INEXACT))
            return 7;
    }
    fesetround(FE_TONEAREST);
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    if (tgamma(-0.0) != -INFINITY || errno != ERANGE || fetestexcept(FE_ALL_EXCEPT) != FE_DIVBYZERO)
        return 8;
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    if (!isnan(tgamma(-2)) || errno != EDOM || fetestexcept(FE_ALL_EXCEPT) != FE_INVALID)
        return 9;
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    if (lgamma(-2) != INFINITY || errno != ERANGE || fetestexcept(FE_ALL_EXCEPT) != FE_DIVBYZERO)
        return 10;
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    if (tgamma(INFINITY) != INFINITY || lgamma(-INFINITY) != INFINITY || errno ||
        fetestexcept(FE_ALL_EXCEPT))
        return 15;
    nan.u = 0xfff0000000000042ULL;
    nan.f = lgamma(nan.f);
    if (nan.u != 0xfff8000000000042ULL || fetestexcept(FE_ALL_EXCEPT) != FE_INVALID)
        return 16;
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    errno = EDOM;
    r = lgamma(3);
    if (errno != EDOM || fetestexcept(FE_ALL_EXCEPT) != (FE_DIVBYZERO | FE_INEXACT))
        return 17;
    return 0;
}
