#include <math.h>
#include <fenv.h>
#include <errno.h>
#include "src/bootstrap-c99/complex/erf-reference.c"

int main(void) {
    int i, k, m[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    double x, r;

    union {
        double f;
        unsigned long long u;
    } n;

    if (erf(1) != 0.84270079294971486934 || erfc(1) != 0.15729920705028513066)
        return 1;
    if (fabs(erfc(2) / 0.004677734981047265838 - 1) > 1e-15)
        return 2;
    if (fabs(erfc(10) / 2.088487583762544757e-45 - 1) > 1e-15)
        return 3;
    for (i = 0; i < 224; i++) {
        x = i / 8.0;
        if (fabs(erf(x) + erfc(x) - 1) > 2e-16)
            return 4;
        if (erf(-x) != -erf(x))
            return 5;
    }
    for (k = 0; k < 4; k++) {
        fesetround(m[k]);
        i = erf_reference(k);
        if (i)
            return 1000 + k * 1000 + i;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        r = erfc(28);
        if (r != (k == 2 ? 0x1p-1074 : 0) || errno != ERANGE ||
            fetestexcept(FE_ALL_EXCEPT) != (FE_UNDERFLOW | FE_INEXACT))
            return 6;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        if (erf(100) != ((k == 1 || k == 3) ? 0x1.fffffffffffffp-1 : 1) ||
            erfc(-100) != ((k == 1 || k == 3) ? 0x1.fffffffffffffp0 : 2))
            return 7;
        if (errno || fetestexcept(FE_ALL_EXCEPT) != FE_INEXACT)
            return 8;
    }
    fesetround(FE_TONEAREST);
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    if (erfc(INFINITY) != 0 || erfc(-INFINITY) != 2 || erf(INFINITY) != 1 || erf(-INFINITY) != -1 ||
        !signbit(erf(-0.0)) || erfc(-0.0) != 1)
        return 9;
    if (errno || fetestexcept(FE_ALL_EXCEPT))
        return 10;
    n.u = 0xfff0000000001234ULL;
    n.f = erfc(n.f);
    if (n.u != 0xfff8000000001234ULL || fetestexcept(FE_ALL_EXCEPT) != FE_INVALID)
        return 11;
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    if (erf(0x1p-1074) != 0x1p-1074 || errno != ERANGE ||
        fetestexcept(FE_ALL_EXCEPT) != (FE_UNDERFLOW | FE_INEXACT))
        return 12;
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    if (erff(0x1p-149f) != 0x1p-149f || errno != ERANGE ||
        fetestexcept(FE_ALL_EXCEPT) != (FE_UNDERFLOW | FE_INEXACT))
        return 13;
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    errno = EDOM;
    r = erfc(1);
    if (errno != EDOM || fetestexcept(FE_ALL_EXCEPT) != (FE_DIVBYZERO | FE_INEXACT))
        return 14;
    return 0;
}
