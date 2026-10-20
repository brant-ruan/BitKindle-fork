#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static int near(double a, double b, double e) {
    return fabs(a - b) <= e;
}

int main(void) {
    double pi = 3.14159265358979323846, ln2 = 0.69314718055994530942;
    double complex z, w;
    int x, y;
    if (clog(1) != 0 || clogf(1) != 0 || clogl(1) != 0)
        return 1;
    z = clog(__cx_pair(-1, -0.0));
    if (creal(z) != 0 || !near(cimag(z), -pi, 1e-14))
        return 2;
    z = clog(I);
    if (creal(z) != 0 || !near(cimag(z), pi / 2, 1e-14))
        return 3;
    z = clog(__cx_pair(0x1p1023, 0x1p1023));
    if (!isfinite(creal(z)) || !near(creal(z), 1023.5 * ln2, 2e-12))
        return 4;
    z = clog(__cx_pair(0x1p-1074, 0));
    if (!near(creal(z), -1074 * ln2, 2e-12))
        return 5;
    z = clog(__cx_pair(1, 0x1p-30));
    if (!(creal(z) > 0) || !near(creal(z), 0x1p-61, 0x1p-110))
        return 6;
    feclearexcept(FE_ALL_EXCEPT);
    z = clog(__cx_pair(-0.0, -0.0));
    if (!isinf(creal(z)) || !signbit(creal(z)) || !near(cimag(z), -pi, 1e-14) ||
        !fetestexcept(FE_DIVBYZERO))
        return 7;
    z = clog(__cx_pair(INFINITY, NAN));
    if (!isinf(creal(z)) || !isnan(cimag(z)))
        return 8;
    z = clog(__cx_pair(NAN, INFINITY));
    if (!isinf(creal(z)) || !isnan(cimag(z)))
        return 9;
    z = clog(__cx_pair(-INFINITY, INFINITY));
    if (!isinf(creal(z)) || !near(cimag(z), 3 * pi / 4, 1e-14))
        return 10;
    if (!near(crealf(clogf(3 + 4 * I)), 1.6094379124341003, 3e-7) ||
        !near(creall(clogl(3 + 4 * I)), 1.6094379124341003, 1e-14))
        return 11;
    for (x = -6; x <= 6; x++)
        for (y = -6; y <= 6; y++) {
            if (!x && !y)
                continue;
            z = __cx_pair(x, y);
            w = clog(z);
            if (clog(conj(z)) != conj(w))
                return 12;
            if (!near(exp(creal(w)), cabs(z), 2e-12))
                return 13;
        }
    feclearexcept(FE_ALL_EXCEPT);
    z = clog(__cx_pair(0x1.fffffffffffffp1023, 0x1.fffffffffffffp1023));
    if (!isfinite(creal(z)) || fetestexcept(FE_OVERFLOW))
        return 14;
    return 0;
}
