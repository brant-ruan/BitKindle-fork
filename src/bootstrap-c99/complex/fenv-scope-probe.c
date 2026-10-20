#include <fenv.h>
#pragma STDC FENV_ACCESS DEFAULT

static double ordinary(double x) {
    return 1.0 + x;
}

#pragma STDC FENV_ACCESS OFF

static double ordinary_off(double x) {
    return 1.0 + x;
}

#pragma STDC FENV_ACCESS ON

static double upward(double x) {
    fesetround(FE_UPWARD);
    return 1.0 + x;
}

static double translated = 1.0 + 0x1p-53;

int main(void) {
    fenv_t saved;
    double x = 0x1p-53, r, one = 1.0, zero = 0.0;
    if (fegetenv(&saved) || fesetround(FE_TONEAREST))
        return 1;
    if (ordinary(x) != 1.0 || ordinary_off(x) != 1.0)
        return 2;
    if (upward(x) != 0x1.0000000000001p0)
        return 3;
    if (fegetround() != FE_UPWARD || translated != 1.0)
        return 4;
    if (fesetround(FE_TONEAREST))
        return 5;
    {
#pragma STDC FENV_ACCESS OFF
        double a = ordinary_off(x);
        if (a != 1.0)
            return 6;
        {
            _Pragma("STDC FENV_ACCESS ON") if (fesetround(FE_DOWNWARD) ||
                                               feclearexcept(FE_ALL_EXCEPT)) return 7;
            r = one + zero;
            if (r != 1.0 || fetestexcept(FE_ALL_EXCEPT))
                return 8;
            r = one / zero;
            if (!(fetestexcept(FE_DIVBYZERO) & FE_DIVBYZERO))
                return 9;
            if (fesetround(FE_TONEAREST))
                return 10;
        }
        if (ordinary(x) != 1.0)
            return 11;
    }
    if (fesetround(FE_UPWARD))
        return 12;
    r = one + x;
    if (r != 0x1.0000000000001p0)
        return 13;
    {
#pragma STDC FENV_ACCESS ON
        if (fesetround(FE_DOWNWARD))
            return 14;
    }
    if (fegetround() != FE_DOWNWARD)
        return 15;
    if (fesetenv(&saved))
        return 16;
    return 0;
}
