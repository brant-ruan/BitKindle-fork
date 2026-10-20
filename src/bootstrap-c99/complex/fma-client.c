#include <math.h>
#include <fenv.h>
#include <errno.h>
#include "src/bootstrap-c99/complex/fma-reference.c"

int main(void) {
    int k, m[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    double r;

    union {
        double f;
        unsigned long long u;
    } n;

    for (k = 0; k < 4; k++) {
        fesetround(m[k]);
        if (fma_reference(k))
            return 11;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        if (fma(0x1.0000000000001p0, 0x1.ffffffffffffep-1, -1) != -0x1p-104)
            return 1;
        if (fmaf(0x1.000002p0f, 0x1.fffffcp-1f, -1) != -0x1p-46f)
            return 2;
        if (fmal(0x1.0000000000001p0L, 0x1.ffffffffffffep-1L, -1) != -0x1p-104L)
            return 3;
        if (fma(0x1.fffffffffffffp1023, 2, -0x1.fffffffffffffp1023) != 0x1.fffffffffffffp1023)
            return 4;
        if (errno || fetestexcept(FE_ALL_EXCEPT))
            return 5;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        r = fma(0x1p-1074, 0.5, 0x0.fffffffffffffp-1022);
        if (r != ((k == 0 || k == 2) ? 0x1p-1022 : 0x0.fffffffffffffp-1022))
            return 12;
        if (fetestexcept(FE_ALL_EXCEPT) !=
            ((k == 0 || k == 2) ? FE_INEXACT : (FE_UNDERFLOW | FE_INEXACT)))
            return 13;
        feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        if (!signbit(fma(-0.0, 1, -0.0)) || signbit(fma(0.0, 1, 0.0)))
            return 14;
        r = fma(1, 1, -1);
        if (r != 0 || !!signbit(r) != (k == 1))
            return 6;
        r = fma(0x1p-1074, 0.5, 0);
        if (r != (k == 2 ? 0x1p-1074 : 0) || errno != ERANGE ||
            fetestexcept(FE_ALL_EXCEPT) != (FE_UNDERFLOW | FE_INEXACT))
            return 7;
    }
    fesetround(FE_TONEAREST);
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    if (!isnan(fma(INFINITY, 0, 1)) || errno != EDOM || !fetestexcept(FE_INVALID))
        return 8;
    if (!isnan(fma(INFINITY, 1, -INFINITY)))
        return 9;
    if (fma(1, 2, INFINITY) != INFINITY)
        return 10;
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
    n.u = 0xfff0000000000042ULL;
    n.f = fma(1, 2, n.f);
    if (n.u != 0xfff8000000000042ULL || fetestexcept(FE_ALL_EXCEPT) != FE_INVALID)
        return 15;
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    errno = EDOM;
    if (fma(2, 3, 4) != 10 || fetestexcept(FE_ALL_EXCEPT) != FE_DIVBYZERO || errno != EDOM)
        return 16;
    return 0;
}
