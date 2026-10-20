#include <math.h>
#include <errno.h>
#include <fenv.h>
#include "src/bootstrap-c99/complex/real-inverse-reference.c"

static int close(double a, double b, double tol) {
    return fabs(a - b) <= tol * (1 + fabs(b));
}

static double frombits(unsigned long long b) {
    double v;
    unsigned char *s = (unsigned char *)&b, *d = (unsigned char *)&v;
    int k;
    for (k = 0; k < 8; k++)
        d[k] = s[k];
    return v;
}

int main(void) {
    double x, y, inf = INFINITY;
    int i, mode[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    if (!close(asinh(1), 0.88137358701954302523, 3e-16) ||
        !close(acosh(2), 1.3169578969248167086, 3e-16) ||
        !close(atanh(0.5), 0.5493061443340548457, 3e-16))
        return 1;
    if (!close(asinhf(1), 0.88137358701954302523, 2e-7) ||
        !close(acoshf(2), 1.3169578969248167086, 2e-7) ||
        !close(atanhf(0.5), 0.5493061443340548457, 2e-7))
        return 2;
    if (!close(asinhl(1), 0.88137358701954302523, 3e-16) ||
        !close(acoshl(2), 1.3169578969248167086, 3e-16) ||
        !close(atanhl(0.5), 0.5493061443340548457, 3e-16))
        return 3;
    for (i = -100; i <= 100; i++) {
        x = i / 10.0;
        y = asinh(x);
        if (!close(sinh(y), x, 2e-14))
            return 4;
        x = i / 101.0;
        y = atanh(x);
        if (!close(tanh(y), x, 2e-14))
            return 5;
    }
    for (i = 0; i <= 100; i++) {
        x = 1 + i / 10.0;
        if (!close(cosh(acosh(x)), x, 2e-14))
            return 6;
    }
    if (!signbit(asinh(-0.0)) || !signbit(atanh(-0.0)) || signbit(acosh(1)))
        return 7;
    if (asinh(inf) != inf || asinh(-inf) != -inf || acosh(inf) != inf)
        return 8;
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(acosh(0.5)) || errno != EDOM || !fetestexcept(FE_INVALID))
        return 9;
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(atanh(2)) || errno != EDOM || !fetestexcept(FE_INVALID))
        return 10;
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    if (atanh(-1) != -inf || errno != ERANGE || !fetestexcept(FE_DIVBYZERO))
        return 11;
    feclearexcept(FE_ALL_EXCEPT);
    y = asinh(0x1.fffffffffffffp1023);
    if (!isfinite(y) || fetestexcept(FE_OVERFLOW | FE_INVALID))
        return 12;
    if (!close(y, 710.4758600739439, 3e-16))
        return 13;
    for (i = 0; i < 4; i++) {
        fesetround(mode[i]);
        x = 0x1p-100;
        feclearexcept(FE_ALL_EXCEPT);
        y = asinh(x);
        if (y != ((i == 1 || i == 3) ? 0x1.fffffffffffffp-101 : x) || !fetestexcept(FE_INEXACT))
            return 14;
        y = atanh(x);
        if (y != (i == 2 ? 0x1.0000000000001p-100 : x))
            return 15;
        x = -0x1p-100;
        y = asinh(x);
        if (y != ((i == 2 || i == 3) ? -0x1.fffffffffffffp-101 : x))
            return 16;
        y = atanh(x);
        if (y != (i == 1 ? -0x1.0000000000001p-100 : x) || fegetround() != mode[i])
            return 17;
    }
    fesetround(FE_TONEAREST);
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    y = asinh(0x1p-1074);
    if (y != 0x1p-1074 || errno != ERANGE ||
        (fetestexcept(FE_UNDERFLOW | FE_INEXACT)) != (FE_UNDERFLOW | FE_INEXACT))
        return 18;
    feclearexcept(FE_ALL_EXCEPT);
    x = frombits(0x7ff0000000000001ULL);
    y = atanh(x);
    if (!isnan(y) || !fetestexcept(FE_INVALID))
        return 19;
    if (real_inverse_reference())
        return 20;
    return 0;
}
