#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

int main(void) {
    double complex z, w;
    double r, i;
    int x, y;
    if (csqrt(3 + 4 * I) != 2 + I || csqrtf(3 - 4 * I) != 2 - I || csqrtl(-3 + 4 * I) != 1 + 2 * I)
        return 1;
    z = csqrt(__cx_pair(-4, -0.0));
    if (creal(z) != 0 || signbit(creal(z)) || cimag(z) != -2)
        return 2;
    z = csqrt(__cx_pair(-0.0, -0.0));
    if (creal(z) != 0 || signbit(creal(z)) || cimag(z) != 0 || !signbit(cimag(z)))
        return 3;
    z = csqrt(__cx_pair(0x1p-1074, 0));
    if (creal(z) != 0x1p-537 || cimag(z) != 0)
        return 4;
    z = csqrt(__cx_pair(0x1.fffffffffffffp1023, 0x1.fffffffffffffp1023));
    if (!isfinite(creal(z)) || !isfinite(cimag(z)) || creal(z) <= 0 || cimag(z) <= 0)
        return 5;
    if (csqrt(__cx_pair(3 * 0x1p900, 4 * 0x1p900)) != __cx_pair(2 * 0x1p450, 0x1p450))
        return 6;
    z = csqrt(__cx_pair(NAN, -INFINITY));
    if (!isinf(creal(z)) || signbit(creal(z)) || !isinf(cimag(z)) || !signbit(cimag(z)))
        return 7;
    z = csqrt(__cx_pair(-INFINITY, -2));
    if (creal(z) != 0 || !isinf(cimag(z)) || !signbit(cimag(z)))
        return 8;
    z = csqrt(__cx_pair(INFINITY, -2));
    if (!isinf(creal(z)) || cimag(z) != 0 || !signbit(cimag(z)))
        return 9;
    z = csqrt(__cx_pair(INFINITY, NAN));
    if (!isinf(creal(z)) || !isnan(cimag(z)))
        return 10;
    z = csqrt(__cx_pair(-INFINITY, NAN));
    if (!isnan(creal(z)) || !isinf(cimag(z)))
        return 11;
    z = csqrt(__cx_pair(NAN, 2));
    if (!isnan(creal(z)) || !isnan(cimag(z)))
        return 12;
    for (x = -8; x <= 8; x++)
        for (y = -8; y <= 8; y++) {
            z = __cx_pair(x, y);
            w = csqrt(z);
            if (creal(w) < 0 || csqrt(conj(z)) != conj(w))
                return 13;
            w = w * w - z;
            r = fabs(creal(w));
            i = fabs(cimag(w));
            if (r > 1e-13 || i > 1e-13)
                return 14;
        }
    feclearexcept(FE_ALL_EXCEPT);
    z = csqrt(__cx_pair(3 * 0x1p900, 4 * 0x1p900));
    if (fetestexcept(FE_OVERFLOW | FE_UNDERFLOW))
        return 15;
    if (csqrtf(__cx_pairf(0x1p-148f, 0)) != 0x1p-74f)
        return 16;
    if (csqrtl(__cx_pairl(0x1p-1074L, 0)) != 0x1p-537L)
        return 17;
    z = csqrt(__cx_pair(4, -0.0));
    if (creal(z) != 2 || cimag(z) != 0 || !signbit(cimag(z)))
        return 18;
    z = csqrt(__cx_pair(0, NAN));
    if (!isnan(creal(z)) || !isnan(cimag(z)))
        return 19;
    return 0;
}
