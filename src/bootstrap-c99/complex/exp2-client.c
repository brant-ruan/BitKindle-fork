#include <math.h>
#include <errno.h>
#include <fenv.h>
#include "src/bootstrap-c99/complex/exp2-reference.c"

int main(void) {
    int e, k, modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    double r, expected;
    float f;
    for (k = 0; k < 4; k++) {
        fesetround(modes[k]);
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        for (e = -1074; e <= 1023; e++) {
            expected = ldexp(1, e);
            if (exp2(e) != expected || exp2l(e) != expected)
                return 1;
        }
        for (e = -149; e <= 127; e++)
            if (exp2f(e) != ldexpf(1, e))
                return 2;
        if (errno || fetestexcept(FE_ALL_EXCEPT))
            return 3;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = exp2(1024);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffffffffffp1023 : INFINITY) || errno != ERANGE ||
            !fetestexcept(FE_OVERFLOW))
            return 4;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        r = exp2(-1075);
        if (r != (k == 2 ? 0x1p-1074 : 0) || errno != ERANGE ||
            (fetestexcept(FE_UNDERFLOW | FE_INEXACT)) != (FE_UNDERFLOW | FE_INEXACT))
            return 5;
        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        f = exp2f(-150);
        if (f != (k == 2 ? 0x1p-149f : 0) || errno != ERANGE || !fetestexcept(FE_UNDERFLOW))
            return 6;
        r = exp2(0x1p-100);
        if (r != (k == 2 ? 0x1.0000000000001p0 : 1))
            return 7;
        r = exp2(-0x1p-100);
        if (r != ((k == 1 || k == 3) ? 0x1.fffffffffffffp-1 : 1) || fegetround() != modes[k])
            return 8;
    }
    fesetround(FE_TONEAREST);
    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);
    if (exp2(-INFINITY) != 0 || exp2(INFINITY) != INFINITY || exp2(-0.0) != 1 || errno ||
        fetestexcept(FE_ALL_EXCEPT))
        return 9;
    if (!isnan(exp2(NAN)))
        return 10;
    for (e = -100; e <= 100; e++) {
        r = exp2(e / 8.0);
        if (fabs(log2(r) - e / 8.0) > 2e-14)
            return 11;
    }
    if (exp2(0.5) != sqrt(2) || exp2f(0.5f) != sqrtf(2) || exp2l(0.5L) != sqrtl(2))
        return 12;
    if (exp2_reference())
        return 13;
    return 0;
}
