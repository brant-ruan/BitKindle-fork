#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static int nearz(double complex a, double complex b, double e) {
    return cabs(a - b) <= e;
}

int main(void) {
    double complex z, w;
    double halfpi = 1.57079632679489661923;
    int x, y;
    if (catan(0) != 0 || catanf(0) != 0 || catanl(0) != 0 || catanh(0) != 0 || catanhf(0) != 0 ||
        catanhl(0) != 0)
        return 1;
    if (!nearz(catan(1 + I), 1.0172219678978514 + 0.4023594781085251 * I, 2e-14))
        return 2;
    if (!nearz(catanh(1 + I), 0.4023594781085251 + 1.0172219678978514 * I, 2e-14))
        return 3;
    z = catanh(__cx_pair(-0.0, 2));
    if (creal(z) != 0 || !signbit(creal(z)))
        return 14;
    z = catan(__cx_pair(2, -0.0));
    if (cimag(z) != 0 || !signbit(cimag(z)))
        return 15;
    if (!nearz(catanf(1 + I), 1.0172219678978514 + 0.4023594781085251 * I, 3e-7))
        return 4;
    if (!nearz(catanhl(1 + I), 0.4023594781085251 + 1.0172219678978514 * I, 2e-14))
        return 5;
    for (x = -5; x <= 5; x++)
        for (y = -5; y <= 5; y++) {
            z = __cx_pair(x / 8.0, y / 8.0);
            w = catanh(z);
            if (!nearz(ctanh(w), z, 2e-12) || !nearz(catanh(conj(z)), conj(w), 1e-14))
                return 6;
            if (!nearz(catanh(-z), -w, 1e-14))
                return 7;
            if (!nearz(ctan(catan(z)), z, 2e-12))
                return 8;
        }
    z = catanh(__cx_pair(2, 0.0));
    w = catanh(__cx_pair(2, -0.0));
    if (!nearz(z, __cx_pair(log(3) / 2, halfpi), 2e-14) || !nearz(w, conj(z), 1e-14))
        return 9;
    feclearexcept(FE_ALL_EXCEPT);
    z = catanh(__cx_pair(1, 0));
    if (!isinf(creal(z)) || signbit(creal(z)) || cimag(z) != 0 || !fetestexcept(FE_DIVBYZERO))
        return 10;
    z = __cx_pair(0x1p-1074, -0x1p-1074);
    if (catanh(z) != z || catan(z) != z)
        return 11;
    z = catanh(__cx_pair(INFINITY, -2));
    if (creal(z) != 0 || cimag(z) != -halfpi)
        return 12;
    z = catanh(__cx_pair(-0.0, NAN));
    if (creal(z) != 0 || !signbit(creal(z)) || !isnan(cimag(z)))
        return 13;
    z = catanh(__cx_pair(0x1p1000, 0));
    if (creal(z) != 0x1p-1000 || cimag(z) != halfpi)
        return 16;
    z = catanh(__cx_pair(0x1p1000, 0x1p1000));
    if (creal(z) != 0x1p-1001 || cimag(z) != halfpi)
        return 17;
    z = catanh(__cx_pair(0x1p999, -0x1p1000));
    if (creal(z) <= 0 || cimag(z) != -halfpi)
        return 18;
    z = catanhf(__cx_pairf(0x1p100f, 0));
    if (creal(z) != 0x1p-100)
        return 19;
    z = catanhl(__cx_pairl(-0x1p1000L, -0.0L));
    if (creal(z) != -0x1p-1000 || cimag(z) != -halfpi)
        return 20;
    for (x = 40; x <= 1000; x += 20) {
        double scale = ldexp(1.0, x), small = ldexp(1.0, -x - 1);
        z = catanh(__cx_pair(scale, scale));
        if (creal(z) != small)
            return 21;
        z = catan(__cx_pair(scale, scale));
        if (cimag(z) != small)
            return 22;
    }
    z = catanh(__cx_pair(0x1.fffffffffffffp1023, 0));
    if (creal(z) != 1.0 / 0x1.fffffffffffffp1023)
        return 23;
    return 0;
}
