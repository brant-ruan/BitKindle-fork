#include <math.h>
#include <fenv.h>
#include <errno.h>
#include "src/bootstrap-c99/complex/cbrt-reference.c"

static double frombits(unsigned long long b) {
    double v;
    unsigned char *s = (unsigned char *)&b, *d = (unsigned char *)&v;
    int k;
    for (k = 0; k < 8; k++)
        d[k] = s[k];
    return v;
}

int main(void) {
    int i, k, e, modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    double x, r;
    for (k = 0; k < 4; k++) {
        fesetround(modes[k]);
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        for (i = -100; i <= 100; i++) {
            if (cbrt(i * i * i) != i || cbrtf(i * i * i) != i || cbrtl(i * i * i) != i)
                return 1;
        }
        if (errno || fetestexcept(FE_ALL_EXCEPT))
            return 2;
        if (cbrt_reference(k))
            return 12;
        for (e = -358; e <= 341; e += 7) {
            x = ldexp(1, 3 * e);
            r = cbrt(x);
            if (r != ldexp(1, e))
                return 3;
        }
        feclearexcept(FE_ALL_EXCEPT);
        r = cbrt(2);
        if (r != ((k == 0 || k == 2) ? 0x1.428a2f98d728bp0 : 0x1.428a2f98d728ap0) ||
            !fetestexcept(FE_INEXACT))
            return 4;
        r = cbrt(-2);
        if (r != ((k == 0 || k == 1) ? -0x1.428a2f98d728bp0 : -0x1.428a2f98d728ap0))
            return 5;
        if (fegetround() != modes[k])
            return 6;
    }
    fesetround(FE_TONEAREST);
    if (!signbit(cbrt(-0.0)) || signbit(cbrt(0.0)) || cbrt(INFINITY) != INFINITY ||
        cbrt(-INFINITY) != -INFINITY)
        return 7;
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    r = cbrt(0x1p-1074);
    if (r != 0x1p-358 || errno || fetestexcept(FE_ALL_EXCEPT))
        return 8;
    r = cbrt(0x1.fffffffffffffp1023);
    if (!isfinite(r) || r < 0x1p341 || r > 0x1p342 ||
        fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_INVALID))
        return 9;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(cbrt(NAN)) || fetestexcept(FE_INVALID))
        return 10;
    feclearexcept(FE_ALL_EXCEPT);
    if (!isnan(cbrt(frombits(0x7ff0000000000001ULL))) || !fetestexcept(FE_INVALID))
        return 11;
    return 0;
}
