#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static int nearz(double complex a, double complex b, double e) {
    return cabs(a - b) <= e;
}

int main(void) {
    double complex z, w;
    double (*real_fn)(double complex) = creal;
    double complex (*power)(double complex, double complex) = cpow;
    int x, y;
    if (power(2, 3) != 8 || cpowf(4, 0.5f) != 2 || cpowl(2, -3) != 0.125L)
        return 1;
    if (cpow(2 + 3 * I, 0) != 1 || cpow(0, 0) != 1)
        return 2;
    if (!nearz(cpow(I, 2), -1, 1e-14))
        return 3;
    z = cpow(__cx_pair(-1, 0.0), 0.5);
    w = cpow(__cx_pair(-1, -0.0), 0.5);
    if (!nearz(z, I, 1e-14) || !nearz(w, -I, 1e-14))
        return 4;
    z = cpow(__cx_pair(4, -0.0), 0.5);
    if (real_fn(z) != 2 || cimag(z) != 0 || !signbit(cimag(z)))
        return 5;
    z = cpow(__cx_pair(4, -0.0), -0.5);
    if (creal(z) != 0.5 || cimag(z) != 0 || signbit(cimag(z)))
        return 6;
    if (cpow(0, 3) != 0)
        return 7;
    feclearexcept(FE_ALL_EXCEPT);
    z = cpow(0, -2);
    if (!isinf(creal(z)) || cimag(z) != 0 || !fetestexcept(FE_DIVBYZERO))
        return 8;
    for (x = -4; x <= 4; x++)
        for (y = -4; y <= 4; y++) {
            if (!x && !y)
                continue;
            z = __cx_pair(x, y);
            if (!nearz(cpow(z, 2), z * z, 3e-12))
                return 9;
            if (!nearz(cpow(z, 0.5), csqrt(z), 2e-12))
                return 10;
            w = __cx_pair(0.75, -0.25);
            if (!nearz(cpow(conj(z), conj(w)), conj(cpow(z, w)), 2e-12))
                return 11;
        }
    if (!nearz(cpow(2 + 3 * I, 1 - I), 9.20434248861135 - 2.844019508370223 * I, 2e-11))
        return 12;
    return 0;
}
