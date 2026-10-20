#include <complex.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static double complex pair(double r, double i) {
    double a[2];
    double complex z;
    unsigned char *s = (unsigned char *)a, *d = (unsigned char *)&z;
    int k;
    a[0] = r;
    a[1] = i;
    for (k = 0; k < 16; k++)
        d[k] = s[k];
    return z;
}

static double frombits(unsigned long long b) {
    double v;
    unsigned char *s = (unsigned char *)&b, *d = (unsigned char *)&v;
    int k;
    for (k = 0; k < 8; k++)
        d[k] = s[k];
    return v;
}

int main(void) {
    int modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}, k, flags;
    double complex a, b, c;
    float complex fa, fb, fc;
    double inf, q;
    for (k = 0; k < 4; k++) {
        fesetround(modes[k]);
        a = pair(1, 1);
        b = pair(3, 3);
        feclearexcept(FE_ALL_EXCEPT);
        c = a / b;
        if (creal(c) != (k == 2 ? 0x1.5555555555556p-2 : 0x1.5555555555555p-2) || cimag(c) != 0 ||
            !fetestexcept(FE_INEXACT))
            return 1;
        fa = (float complex)a;
        fb = (float complex)b;
        feclearexcept(FE_ALL_EXCEPT);
        fc = fa / fb;
        if (crealf(fc) != ((k == 0 || k == 2) ? 0x1.555556p-2f : 0x1.555554p-2f) ||
            cimagf(fc) != 0 || !fetestexcept(FE_INEXACT))
            return 2;
        a = pair(0x1.fffffffffffffp1023, 0x1.fffffffffffffp1023);
        b = pair(1, 1);
        feclearexcept(FE_ALL_EXCEPT);
        c = a / b;
        if (creal(c) != 0x1.fffffffffffffp1023 || cimag(c) != 0 || fetestexcept(FE_ALL_EXCEPT))
            return 3;
        a = pair(0x1p-1074, 0x1p-1074);
        b = pair(1, 1);
        feclearexcept(FE_ALL_EXCEPT);
        c = a / b;
        if (creal(c) != 0x1p-1074 || cimag(c) != 0 ||
            fetestexcept(FE_UNDERFLOW | FE_INVALID | FE_INEXACT | FE_OVERFLOW))
            return 4;
        b = pair(2, 2);
        feclearexcept(FE_ALL_EXCEPT);
        c = a / b;
        flags = fetestexcept(FE_ALL_EXCEPT);
        if (creal(c) != (k == 2 ? 0x1p-1074 : 0) || cimag(c) != 0 ||
            (flags & (FE_UNDERFLOW | FE_INEXACT)) != (FE_UNDERFLOW | FE_INEXACT))
            return 5;
        if (fegetround() != modes[k])
            return 6;
    }
    fesetround(FE_TONEAREST);
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    a = pair(6, 8);
    b = pair(1, 2);
    c = a / b;
    if (creal(c) != 4.4 || cimag(c) != -0.8 || !fetestexcept(FE_DIVBYZERO))
        return 7;
    inf = frombits(0x7ff0000000000000ULL);
    q = frombits(0x7ff8000000000000ULL);
    a = pair(0x1.fffffffffffffp1023, 0x1.fffffffffffffp1023);
    b = pair(inf, inf);
    feclearexcept(FE_ALL_EXCEPT);
    c = a / b;
    if (c != 0 || fetestexcept(FE_ALL_EXCEPT))
        return 8;
    a = pair(inf, q);
    b = pair(1, 1);
    feclearexcept(FE_ALL_EXCEPT);
    c = a / b;
    if (creal(c) != inf || cimag(c) != -inf || fetestexcept(FE_ALL_EXCEPT))
        return 9;
    a = pair(2, 3);
    b = pair(q, -inf);
    feclearexcept(FE_ALL_EXCEPT);
    c = a / b;
    if (c != 0 || fetestexcept(FE_ALL_EXCEPT))
        return 10;
    return 0;
}
