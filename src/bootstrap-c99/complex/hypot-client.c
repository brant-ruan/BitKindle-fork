#include <math.h>
#include <fenv.h>
#include <errno.h>

static double frombits(unsigned long long b) {
    double v;
    unsigned char *s = (unsigned char *)&b, *d = (unsigned char *)&v;
    int k;
    for (k = 0; k < 8; k++)
        d[k] = s[k];
    return v;
}

int main(void) {
    double r, a, b, inf = INFINITY, q = NAN, sn = frombits(0x7ff0000000000001ULL);
    int e, k, m, n, sc, modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    if (hypot(3, 4) != 5 || hypotf(3, 4) != 5 || hypotl(3, 4) != 5)
        return 1;
    for (e = -1074; e <= 1019; e += 13) {
        a = ldexp(3, e);
        b = ldexp(4, e);
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = hypot(a, b);
        if (r != ldexp(5, e) || errno || fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_INVALID))
            return 2;
    }
    if (hypot(-3, -4) != 5 || hypot(4, 3) != 5 || signbit(hypot(-0.0, -0.0)))
        return 3;
    feclearexcept(FE_ALL_EXCEPT);
    if (hypot(inf, q) != inf || hypot(q, -inf) != inf || fetestexcept(FE_ALL_EXCEPT))
        return 4;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(hypot(q, 2)) || fetestexcept(FE_INVALID))
        return 5;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(hypot(sn, 2)) || !fetestexcept(FE_INVALID))
        return 6;
    for (k = 0; k < 4; k++) {
        fesetround(modes[k]);
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = hypot(1, 0x1p-100);
        if (r != (k == 2 ? 0x1.0000000000001p0 : 1) || errno || !fetestexcept(FE_INEXACT) ||
            fetestexcept(FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID))
            return 7;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = hypot(0x1.fffffffffffffp1023, 0x1.fffffffffffffp1023);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffffffffffp1023 : inf) || errno != ERANGE ||
            !fetestexcept(FE_OVERFLOW))
            return 8;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = hypot(0x1p-1074, 0x1p-1074);
        if (r != (k == 2 ? 0x1p-1073 : 0x1p-1074) || errno != ERANGE ||
            (fetestexcept(FE_UNDERFLOW | FE_INEXACT)) != (FE_UNDERFLOW | FE_INEXACT))
            return 9;
        if (fegetround() != modes[k])
            return 10;
    }
    fesetround(FE_TONEAREST);
    if (hypotf(3 * 0x1p-149f, 4 * 0x1p-149f) != 5 * 0x1p-149f ||
        hypotl(3 * 0x1p-1074L, 4 * 0x1p-1074L) != 5 * 0x1p-1074L)
        return 11;
    if (hypotf(0x1.8p100f, 0x1p101f) != 0x1.4p101f || hypotl(0x1.8p900L, 0x1p901L) != 0x1.4p901L)
        return 12;
    for (m = 2; m <= 20; m++)
        for (n = 1; n < m; n++)
            for (sc = 0; sc < 3; sc++) {
                e = sc == 0 ? -1074 : sc == 1 ? 0 : 1010;
                a = ldexp(m * m - n * n, e);
                b = ldexp(2 * m * n, e);
                if (hypot(a, b) != ldexp(m * m + n * n, e) ||
                    hypotl(a, b) != ldexp(m * m + n * n, e))
                    return 13;
                e = sc == 0 ? -149 : sc == 1 ? 0 : 110;
                if (hypotf(ldexpf(m * m - n * n, e), ldexpf(2 * m * n, e)) !=
                    ldexpf(m * m + n * n, e))
                    return 14;
            }
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_OVERFLOW);
    errno = 0;
    if (hypot(3, 4) != 5 || errno || !fetestexcept(FE_OVERFLOW))
        return 15;
    return 0;
}
