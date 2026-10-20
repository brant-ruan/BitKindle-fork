#include <fenv.h>
#pragma STDC FENV_ACCESS ON
#pragma STDC FP_CONTRACT OFF

double contraction(double a, double b, double c) {
    return a * b + c;
}

int conversions(void) {
    static double translated = 1.0 + 0x1p-53;
    double d = 0x1.000001p0, out;
    float f;
    long long si = 9007199254740993LL;
    unsigned long long ui = 18446744073709551615ULL;
    int i;
    if (fesetround(FE_UPWARD))
        return 20;
    feclearexcept(FE_ALL_EXCEPT);
    f = (float)d;
    if (f != 0x1.000002p0f || !(fetestexcept(FE_INEXACT)))
        return 21;
    out = (double)si;
    if (out != 9007199254740994.0)
        return 22;
    out = (double)ui;
    if (out != 0x1p64)
        return 23;
    d = -1.75;
    i = (int)d;
    if (i != -1)
        return 24;
    if (fesetround(FE_DOWNWARD))
        return 25;
    d = 0x1.000001p0;
    f = (float)d;
    if (f != 1.0f)
        return 26;
    out = (double)si;
    if (out != 9007199254740992.0)
        return 27;
    out = (double)ui;
    if (out != 0x1.fffffffffffffp63)
        return 28;
    d = 1.75;
    i = (int)d;
    if (i != 1)
        return 29;
    /* Static initialization is translated before execution, using default rounding. */
    if (translated != 1.0)
        return 30;
    if (fesetround(FE_TONEAREST))
        return 31;
    return 0;
}

int main(void) {
    double a, b, r, up, down;
    float af, bf, rf;
    fenv_t saved;
    int status;
    if (feholdexcept(&saved))
        return 1;
    status = conversions();
    if (status)
        return status;
    a = 1.0;
    b = 0x1p-53;
    if (fesetround(FE_UPWARD))
        return 2;
    up = a + b;
    if (up != 0x1.0000000000001p0)
        return 3;
    if (!(fetestexcept(FE_INEXACT)))
        return 4;
    if (fesetround(FE_DOWNWARD))
        return 5;
    down = a + b;
    if (down != 1.0)
        return 6;
    if (fesetround(FE_UPWARD))
        return 7;
    af = 1.0f;
    bf = 0x1p-24f;
    rf = af + bf;
    if (rf != 0x1.000002p0f)
        return 8;
    if (fesetround(FE_TONEAREST))
        return 9;
    feclearexcept(FE_ALL_EXCEPT);
    a = 1.0;
    b = 0.0;
    a / b;
    if (fetestexcept(FE_ALL_EXCEPT) != FE_DIVBYZERO)
        return 10;
    feclearexcept(FE_ALL_EXCEPT);
    r = contraction(0x1.0000000000001p0, 0x1.ffffffffffffep-1, -1.0);
    if (r != 0.0)
        return 11;
    if (!(fetestexcept(FE_INEXACT)))
        return 12;
    {
#pragma STDC FP_CONTRACT ON
        double nested = 0x1.0000000000001p0 * 0x1.ffffffffffffep-1 - 1.0;
        if (nested != 0.0)
            return 13;
    }
    {
        _Pragma("STDC FP_CONTRACT DEFAULT")
            _Pragma("STDC FENV_ACCESS ON") if (fesetround(FE_UPWARD)) return 14;
        r = 1.0 + 0x1p-53;
        if (r != 0x1.0000000000001p0)
            return 15;
    }
    if (fesetenv(&saved))
        return 16;
    return 0;
}
