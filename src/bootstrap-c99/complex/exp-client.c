#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static int near(double a, double b, double e) {
    return fabs(a - b) <= e;
}

int main(void) {
    double complex z, w;
    double r;
    int x, y;
    if (cexp(0) != 1 || cexpf(0) != 1 || cexpl(0) != 1)
        return 1;
    z = cexp(__cx_pair(0, -0.0));
    if (creal(z) != 1 || cimag(z) != 0 || !signbit(cimag(z)))
        return 2;
    z = cexp(I);
    if (!near(creal(z), 0.5403023058681397174, 1e-14) ||
        !near(cimag(z), 0.8414709848078965067, 1e-14))
        return 3;
    if (!near(crealf(cexpf(I)), 0.5403023058681397174, 2e-7) ||
        !near(cimagl(cexpl(I)), 0.8414709848078965067, 1e-14))
        return 4;
    z = cexp(__cx_pair(710, 1.5707963267948966));
    if (!isfinite(creal(z)) || creal(z) <= 0 || !isinf(cimag(z)))
        return 5;
    z = cexp(__cx_pair(INFINITY, -0.0));
    if (!isinf(creal(z)) || cimag(z) != 0 || !signbit(cimag(z)))
        return 6;
    z = cexp(__cx_pair(-INFINITY, -1));
    if (creal(z) != 0 || cimag(z) != 0 || !signbit(cimag(z)))
        return 7;
    z = cexp(__cx_pair(-INFINITY, NAN));
    if (z != 0)
        return 8;
    z = cexp(__cx_pair(INFINITY, NAN));
    if (!isinf(creal(z)) || !isnan(cimag(z)))
        return 9;
    z = cexp(__cx_pair(NAN, -0.0));
    if (!isnan(creal(z)) || cimag(z) != 0 || !signbit(cimag(z)))
        return 10;
    feclearexcept(FE_ALL_EXCEPT);
    z = cexp(__cx_pair(1, INFINITY));
    if (!isnan(creal(z)) || !isnan(cimag(z)) || !fetestexcept(FE_INVALID))
        return 11;
    for (x = -4; x <= 4; x++)
        for (y = -6; y <= 6; y++) {
            z = __cx_pair(x / 2.0, y / 4.0);
            w = cexp(z);
            if (cexp(conj(z)) != conj(w))
                return 12;
            w = clog(w) - z;
            if (fabs(creal(w)) > 2e-13 || fabs(cimag(w)) > 2e-13)
                return 13;
        }
    z = cexp(__cx_pair(710, 0x1p-100));
    r = cimag(z);
    if (!isfinite(r) || r <= 0 || !near(log(r), 710 - 100 * 0.69314718055994530942, 2e-12))
        return 14;
    z = cexp(__cx_pair(1400, 0x1p-1074));
    r = cimag(z);
    if (!isfinite(r) || r <= 0 || !near(log(r), 1400 - 1074 * 0.69314718055994530942, 2e-12))
        return 15;
    r = cimagf(cexpf(__cx_pairf(100, 0x1p-100f)));
    if (!isfinite(r) || r <= 0 || !near(log(r), 100 - 100 * 0.69314718055994530942, 2e-6))
        return 16;
    z = cexp(__cx_pair(INFINITY, INFINITY));
    if (!isinf(creal(z)) || !isnan(cimag(z)))
        return 17;
    return 0;
}
