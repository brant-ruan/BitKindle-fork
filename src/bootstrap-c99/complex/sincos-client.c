#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static int nearz(double complex a, double complex b, double e) {
    return cabs(a - b) <= e;
}

int main(void) {
    double complex z, s, c;
    double r;
    int x, y;
    if (csin(0) != 0 || ccos(0) != 1 || csinh(0) != 0 || ccosh(0) != 1)
        return 1;
    if (csinf(0) != 0 || ccosf(0) != 1 || csinhf(0) != 0 || ccoshf(0) != 1)
        return 2;
    if (csinl(0) != 0 || ccosl(0) != 1 || csinhl(0) != 0 || ccoshl(0) != 1)
        return 3;
    if (!nearz(csin(1 + I), 1.2984575814159773 + 0.6349639147847361 * I, 2e-14))
        return 4;
    if (!nearz(ccos(1 + I), 0.833730025131149 - 0.9888977057628651 * I, 2e-14))
        return 5;
    for (x = -4; x <= 4; x++)
        for (y = -4; y <= 4; y++) {
            z = __cx_pair(x / 4.0, y / 4.0);
            s = csin(z);
            c = ccos(z);
            if (!nearz(s * s + c * c, 1, 3e-13))
                return 6;
            if (!nearz(csin(-z), -s, 1e-14) || !nearz(ccos(-z), c, 1e-14))
                return 7;
            if (!nearz(csin(conj(z)), conj(s), 1e-14) || !nearz(ccos(conj(z)), conj(c), 1e-14))
                return 8;
            s = csinh(z);
            c = ccosh(z);
            if (!nearz(c * c - s * s, 1, 3e-13))
                return 9;
            if (!nearz(csinh(conj(z)), conj(s), 1e-14) || !nearz(ccosh(conj(z)), conj(c), 1e-14))
                return 10;
        }
    z = csin(__cx_pair(0x1p-100, 710));
    r = creal(z);
    if (!isfinite(r) || r <= 0 || fabs(log(r) - (710 - 101 * 0.69314718055994530942)) > 2e-12)
        return 11;
    z = csin(__cx_pair(0, INFINITY));
    if (creal(z) != 0 || !isinf(cimag(z)) || signbit(cimag(z)))
        return 12;
    z = ccos(__cx_pair(0, INFINITY));
    if (!isinf(creal(z)) || cimag(z) != 0)
        return 13;
    z = csinh(__cx_pair(INFINITY, -0.0));
    if (!isinf(creal(z)) || cimag(z) != 0 || !signbit(cimag(z)))
        return 14;
    z = csin(__cx_pair(-0.0, -0.0));
    if (!signbit(creal(z)) || !signbit(cimag(z)))
        return 15;
    feclearexcept(FE_ALL_EXCEPT);
    z = csin(__cx_pair(INFINITY, 0));
    if (!isnan(creal(z)) || cimag(z) != 0 || !fetestexcept(FE_INVALID))
        return 16;
    if (!nearz(csinf(1 + I), 1.2984575814159773 + 0.6349639147847361 * I, 3e-7))
        return 17;
    if (!nearz(csinhf(1 + I), 0.6349639147847361 + 1.2984575814159773 * I, 3e-7))
        return 18;
    if (!nearz(ccosl(1 + I), 0.833730025131149 - 0.9888977057628651 * I, 2e-14))
        return 19;
    if (!nearz(ccoshl(1 + I), 0.833730025131149 + 0.9888977057628651 * I, 2e-14))
        return 20;
    return 0;
}
