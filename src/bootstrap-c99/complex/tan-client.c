#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static int nearz(double complex a, double complex b, double e) {
    return cabs(a - b) <= e;
}

int main(void) {
    double complex z, w;
    int x, y;
    if (ctan(0) != 0 || ctanf(0) != 0 || ctanl(0) != 0 || ctanh(0) != 0 || ctanhf(0) != 0 ||
        ctanhl(0) != 0)
        return 1;
    if (!nearz(ctan(1 + I), 0.2717525853195117 + 1.0839233273386945 * I, 2e-14))
        return 2;
    if (!nearz(ctanh(1 + I), 1.0839233273386945 + 0.2717525853195117 * I, 2e-14))
        return 3;
    if (!nearz(ctanf(1 + I), 0.2717525853195117 + 1.0839233273386945 * I, 4e-7))
        return 4;
    if (!nearz(ctanhl(1 + I), 1.0839233273386945 + 0.2717525853195117 * I, 2e-14))
        return 5;
    for (x = -4; x <= 4; x++)
        for (y = -8; y <= 8; y++) {
            z = __cx_pair(x / 4.0, y / 4.0);
            w = ctan(z);
            if (!nearz(w, csin(z) / ccos(z), 3e-13))
                return 6;
            if (!nearz(ctan(-z), -w, 1e-14) || !nearz(ctan(conj(z)), conj(w), 1e-14))
                return 7;
            if (!nearz(ctanh(z), csinh(z) / ccosh(z), 3e-12))
                return 8;
        }
    if (ctan(__cx_pair(1, 1000)) != I || ctan(__cx_pair(1, -1000)) != -I)
        return 9;
    if (ctanh(__cx_pair(1000, 1)) != 1 || ctanh(__cx_pair(-1000, 1)) != -1)
        return 10;
    z = ctan(__cx_pair(1, -0.0));
    if (cimag(z) != 0 || !signbit(cimag(z)))
        return 11;
    z = ctan(__cx_pair(INFINITY, -INFINITY));
    if (creal(z) != 0 || cimag(z) != -1)
        return 12;
    z = ctanh(__cx_pair(INFINITY, NAN));
    if (creal(z) != 1 || cimag(z) != 0)
        return 13;
    feclearexcept(FE_ALL_EXCEPT);
    z = ctan(__cx_pair(INFINITY, 1));
    if (!isnan(creal(z)) || !isnan(cimag(z)) || !fetestexcept(FE_INVALID))
        return 14;
    z = ctan(__cx_pair(1.5707963267948966, 0x1p-30));
    if (!isfinite(creal(z)) || !isfinite(cimag(z)) || cimag(z) <= 0)
        return 15;
    if (ctanf(__cx_pairf(1, 100)) != I || ctanhf(__cx_pairf(100, 1)) != 1)
        return 16;
    if (ctanl(__cx_pairl(1, -1000)) != -I || ctanhl(__cx_pairl(-1000, 1)) != -1)
        return 17;
    z = ctanh(__cx_pair(-0.0, -0.0));
    if (!signbit(creal(z)) || !signbit(cimag(z)))
        return 18;
    return 0;
}
