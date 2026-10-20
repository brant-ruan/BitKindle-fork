#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>

static int nearz(double complex a, double complex b, double e) {
    return cabs(a - b) <= e;
}

int main(void) {
    double complex z, w;
    double p = 1.57079632679489661923;
    int x, y;
    if (casin(0) != 0 || casinf(0) != 0 || casinl(0) != 0 || casinh(0) != 0 || casinhf(0) != 0 ||
        casinhl(0) != 0)
        return 1;
    if (cacos(1) != 0 || cacosf(1) != 0 || cacosl(1) != 0 || cacosh(1) != 0 || cacoshf(1) != 0 ||
        cacoshl(1) != 0)
        return 2;
    if (!nearz(casin(1 + I), 0.6662394324925153 + 1.0612750619050357 * I, 2e-14))
        return 3;
    if (!nearz(cacos(1 + I), 0.9045568943023813 - 1.0612750619050357 * I, 2e-14))
        return 4;
    if (!nearz(casinhf(1 + I), 1.0612750619050357 + 0.6662394324925153 * I, 3e-7))
        return 5;
    if (!nearz(cacoshl(1 + I), 1.0612750619050357 + 0.9045568943023813 * I, 2e-14))
        return 6;
    for (x = -6; x <= 6; x++)
        for (y = -6; y <= 6; y++) {
            z = __cx_pair(x / 4.0, y / 4.0);
            if (!nearz(csin(casin(z)), z, 4e-12) || !nearz(ccos(cacos(z)), z, 4e-12))
                return 7;
            if (!nearz(csinh(casinh(z)), z, 4e-12) || !nearz(ccosh(cacosh(z)), z, 4e-12))
                return 8;
            if (!nearz(casin(conj(z)), conj(casin(z)), 1e-14) ||
                !nearz(cacos(conj(z)), conj(cacos(z)), 1e-14))
                return 9;
            w = cacosh(z);
            if (creal(w) < 0 || fabs(cimag(w)) > 2 * p)
                return 10;
        }
    z = casin(__cx_pair(2, -0.0));
    if (creal(z) != p || cimag(z) >= 0)
        return 11;
    z = cacos(__cx_pair(2, -0.0));
    if (creal(z) != 0 || cimag(z) <= 0)
        return 12;
    z = cacosh(__cx_pair(-2, -0.0));
    if (creal(z) <= 0 || cimag(z) != -2 * p)
        return 13;
    z = __cx_pair(0x1p-1074, -0x1p-1074);
    if (casin(z) != z || casinh(z) != z)
        return 14;
    z = cacos(__cx_pair(INFINITY, INFINITY));
    if (creal(z) != p / 2 || !isinf(cimag(z)) || !signbit(cimag(z)))
        return 15;
    z = cacos(__cx_pair(0, NAN));
    if (creal(z) != p || !isnan(cimag(z)))
        return 16;
    z = casin(__cx_pair(0x1p1000, 0x1p1000));
    if (!isfinite(creal(z)) || !isfinite(cimag(z)))
        return 17;
    z = casin(__cx_pair(0.75, 0x1p-100));
    if (cimag(z) != 0x1p-100 / sqrt(0.4375))
        return 18;
    z = cacos(__cx_pair(0x1.fffffffffffffp-1, -0x1p-200));
    if (cimag(z) <= 0)
        return 19;
    z = cacos(__cx_pair(1, 0x1p-1074));
    if (creal(z) != 0x1p-537 || cimag(z) != -0x1p-537)
        return 20;
    z = casin(__cx_pair(-1, -0x1p-1074));
    if (creal(z) != -p || cimag(z) != -0x1p-537)
        return 21;
    z = cacos(__cx_pair(2, 0x1p-100));
    if (creal(z) != 0x1p-100 / sqrt(3) || cimag(z) >= 0)
        return 22;
    z = cacosf(__cx_pairf(1, -0x1p-148f));
    if (creal(z) != 0x1p-74 || cimag(z) != 0x1p-74)
        return 23;
    z = cacosl(__cx_pairl(1, 0x1p-1074L));
    if (creal(z) != 0x1p-537 || cimag(z) != -0x1p-537)
        return 24;
    for (x = 20; x <= 1000; x += 20) {
        double tiny = ldexp(1.0, -x);
        z = cacos(__cx_pair(1, tiny));
        if (creal(z) <= 0 || cimag(z) >= 0)
            return 25;
        w = ccos(z);
        if (fabs(cimag(w) / tiny - 1) > 2e-12)
            return 26;
    }
    return 0;
}
