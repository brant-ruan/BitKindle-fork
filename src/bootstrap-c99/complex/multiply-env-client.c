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

static unsigned long long bits(double d) {
    unsigned long long v;
    unsigned char *s = (unsigned char *)&d, *p = (unsigned char *)&v;
    int k;
    for (k = 0; k < 8; k++)
        p[k] = s[k];
    return v;
}

int main(void) {
    int modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}, k, flags;
    unsigned int oldcw, cw;
    unsigned long long stack;
    double complex a, b, c;
    float complex fa, fb, fc;
    for (k = 0; k < 4; k++) {
        if (fesetround(modes[k]))
            return 1;
        a = pair(1, 0x1p-53);
        b = pair(1, -1);
        feclearexcept(FE_ALL_EXCEPT);
        c = a * b;
        if (creal(c) != (k == 2 ? 0x1.0000000000001p0 : 1) || !(fetestexcept(FE_INEXACT)))
            return 2;
        fa = (float complex)pair(1, 0x1p-24);
        fb = (float complex)b;
        feclearexcept(FE_ALL_EXCEPT);
        fc = fa * fb;
        if (crealf(fc) != (k == 2 ? 0x1.000002p0f : 1) || !(fetestexcept(FE_INEXACT)))
            return 3;
        a = pair(1, 1);
        c = a * a;
        if (bits(creal(c)) != (k == 1 ? 0x8000000000000000ULL : 0))
            return 4;
        a = pair(0x1p-537, 0x1p-537);
        b = pair(0x1p-538, 0x1p-538);
        feclearexcept(FE_ALL_EXCEPT);
        c = a * b;
        if (cimag(c) != 0x1p-1074 ||
            fetestexcept(FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID | FE_INEXACT))
            return 5;
        a = pair(0x1p-537, 0);
        b = pair(0, 0x1p-538);
        feclearexcept(FE_ALL_EXCEPT);
        c = a * b;
        flags = fetestexcept(FE_ALL_EXCEPT);
        if (cimag(c) != (k == 2 ? 0x1p-1074 : 0) ||
            (flags & (FE_UNDERFLOW | FE_INEXACT)) != (FE_UNDERFLOW | FE_INEXACT))
            return 6;
        a = pair(0x1.fffffffffffffp1023, 0x1.fffffffffffffp1023);
        feclearexcept(FE_ALL_EXCEPT);
        c = a * a;
        flags = fetestexcept(FE_ALL_EXCEPT);
        if (creal(c) != 0 || (flags & FE_INVALID) || !(flags & FE_OVERFLOW) ||
            !(flags & FE_INEXACT))
            return 7;
        if ((k == 1 || k == 3) ? cimag(c) != 0x1.fffffffffffffp1023
                               : bits(cimag(c)) != 0x7ff0000000000000ULL)
            return 8;
        if (fegetround() != modes[k])
            return 9;
    }
    fesetround(FE_TONEAREST);
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    a = pair(2, 3);
    b = pair(4, 5);
    c = a * b;
    if (creal(c) != -7 || cimag(c) != 22 || !fetestexcept(FE_DIVBYZERO))
        return 10;
    oldcw = ((unsigned int (*)(void)) "\xd9\x7c\x24\x08\x0f\xb7\x44\x24\x08\xc3")();
    ((void (*)(unsigned int)) "\x89\x4c\x24\x08\xd9\x6c\x24\x08\xd9\xe8\xc3")(0x47f);
    c = a * b;
    cw = ((unsigned int (*)(void)) "\xd9\x7c\x24\x08\x0f\xb7\x44\x24\x08\xc3")();
    stack = ((unsigned long long (*)(void)) "\xdd\x5c\x24\x08\x48\x8b\x44\x24\x08\xc3")();
    ((void (*)(unsigned int)) "\x89\x4c\x24\x08\xd9\x6c\x24\x08\xc3")(oldcw);
    if (cw != 0x47f || stack != 0x3ff0000000000000ULL || creal(c) != -7 || cimag(c) != 22)
        return 11;
    return 0;
}
