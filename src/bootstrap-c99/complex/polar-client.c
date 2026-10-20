#include <complex.h>
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static double complex pair(double r, double i) {
    double a[2];
    double complex z;
    unsigned char *p = (unsigned char *)&z, *q = (unsigned char *)a;
    int n;
    a[0] = r;
    a[1] = i;
    for (n = 0; n < sizeof z; n++)
        p[n] = q[n];
    return z;
}

static int near(double a, double b, double e) {
    return fabs(a - b) <= e;
}

int main(void) {
    double pi = 3.14159265358979323846, r;
    double complex z;
    double (*mag)(double complex) = cabs;
    if (mag(3 + 4 * I) != 5 || cabsf(3 + 4 * I) != 5 || cabsl(3 + 4 * I) != 5)
        return 1;
    if (cabs(pair(0x1.8p900, 0x1p901)) != 0x1.4p901)
        return 2;
    if (cabs(pair(3 * 0x1p-1074, 4 * 0x1p-1074)) != 5 * 0x1p-1074)
        return 3;
    z = pair(-0.0, -0.0);
    r = cabs(z);
    if (r != 0 || signbit(r))
        return 4;
    if (!near(carg(pair(-1, 0.0)), pi, 1e-14) || !near(carg(pair(-1, -0.0)), -pi, 1e-14))
        return 5;
    if (!near(cargf(I), pi / 2, 2e-7) || !near(cargl(-I), -pi / 2, 1e-14))
        return 6;
    r = carg(pair(1, -0.0));
    if (r != 0 || !signbit(r))
        return 7;
    if (!isinf(cabs(pair(INFINITY, NAN))) || !isinf(cabs(pair(NAN, -INFINITY))))
        return 8;
    if (!isnan(cabs(pair(NAN, 1))))
        return 9;
    if (cabs(pair(0x1p1000, 0x1p-1000)) != 0x1p1000)
        return 10;
    if (fesetround(FE_UPWARD))
        return 11;
    r = cabs(pair(1, 0x1p-100));
    if (fesetround(FE_TONEAREST))
        return 12;
    if (r != 0x1.0000000000001p0)
        return 13;
    if (!near(carg(pair(1, 1)), pi / 4, 1e-14) || !near(carg(pair(-1, -1)), -3 * pi / 4, 1e-14))
        return 14;
    if (cabsf((float complex)pair(3 * 0x1p-149, 4 * 0x1p-149)) != 5 * 0x1p-149f)
        return 15;
    if (cabsf((float complex)pair(0x1.8p100, 0x1p101)) != 0x1.4p101f)
        return 16;
    if (cabsl((long double complex)pair(0x1.8p900, 0x1p901)) != 0x1.4p901L)
        return 17;
    feclearexcept(FE_ALL_EXCEPT);
    r = cabs(pair(0x1.8p900, 0x1p901));
    if (fetestexcept(FE_OVERFLOW | FE_UNDERFLOW) || r != 0x1.4p901)
        return 18;
    if (fesetround(FE_DOWNWARD))
        return 19;
    r = cabs(pair(1, 0x1p-100));
    if (fesetround(FE_TONEAREST))
        return 20;
    if (r != 1)
        return 21;
    return 0;
}
