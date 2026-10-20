#include <fenv.h>
#pragma STDC FENV_ACCESS ON
static double seen;

static int change(double value) {
    seen = value;
    return fesetround(FE_UPWARD);
}

static int verify(double value) {
    return value != 0x1.0000000000001p0;
}

int main(void) {
    fenv_t saved;
    double x = 0x1p-53, one = 1.0, zero = 0.0, three = 3.0, r;
    float f;
    if (fegetenv(&saved))
        return 1;
    if (fesetround(FE_TONEAREST))
        return 2;
    if (change(x + 1.0) || seen != 1.0 || verify(x + 1.0))
        return 3;
    if (fesetround(FE_DOWNWARD))
        return 4;
    r = one + x;
    if (r != 1.0)
        return 5;
    if (fesetround(FE_TOWARDZERO))
        return 6;
    r = one + x;
    if (r != 1.0)
        return 7;
    if (fesetround(FE_UPWARD))
        return 8;
    r = one + x;
    if (r != 0x1.0000000000001p0)
        return 9;
    f = (float)0x1.000001p0;
    if (f != 0x1.000002p0f)
        return 10;
    if (fesetround(FE_TONEAREST) || feclearexcept(FE_ALL_EXCEPT))
        return 11;
    r = one / three;
    if (!(fetestexcept(FE_INEXACT) & FE_INEXACT))
        return 12;
    if (feclearexcept(FE_ALL_EXCEPT))
        return 13;
    r = one / zero;
    if (!(fetestexcept(FE_DIVBYZERO) & FE_DIVBYZERO))
        return 14;
    if (feclearexcept(FE_ALL_EXCEPT))
        return 15;
    if (0 && (one / zero))
        return 16;
    if (sizeof(one / zero) != sizeof(double))
        return 17;
    if (fetestexcept(FE_ALL_EXCEPT))
        return 18;
    {
        _Pragma("STDC FENV_ACCESS ON") if (fesetround(FE_DOWNWARD)) return 19;
        r = one + x;
        if (r != 1.0)
            return 20;
    }
    if (fegetround() != FE_DOWNWARD)
        return 21;
    if (fesetenv(&saved))
        return 22;
    return 0;
}
