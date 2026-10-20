#include <complex.h>
#include <math.h>
#include <fenv.h>

static double complex pair(double r, double i) {
    union {
        double complex z;
        double a[2];
    } v;

    v.a[0] = r;
    v.a[1] = i;
    return v.z;
}

#define PROBE { \
 double complex a,b,c;float complex fa,fb,fc; \
 a=pair(0x1.fffffffffffffp1023,0x1.fffffffffffffp1023);b=a; \
 feclearexcept(FE_ALL_EXCEPT);c=a/b; \
 if(creal(c)!=1||cimag(c)!=0||fetestexcept(FE_OVERFLOW|FE_UNDERFLOW|FE_INVALID|FE_DIVBYZERO))return 1; \
 a=pair(0x1p-1074,0x1p-1074);b=a;c=a/b;if(creal(c)!=1||cimag(c)!=0)return 2; \
 a=pair(0x1p600,0x1p600);b=pair(0x1p-600,-0x1p-600);c=a*b;if(creal(c)!=2||cimag(c)!=0)return 3; \
 a=pair(0x1.8p901,0x1p902);if(cabs(a)!=0x1.4p902)return 4; \
 fa=(float complex)pair(0x1p80,0x1p80);fb=(float complex)pair(0x1p-80,-0x1p-80);fc=fa*fb;if(crealf(fc)!=2||cimagf(fc)!=0)return 5; \
 }
#pragma STDC FENV_ACCESS ON

/* The default CX policy must work before an explicit setting. */
static int absent(void) {
    PROBE return 0;
}

#pragma STDC CX_LIMITED_RANGE ON

static int enabled(void) {
    PROBE return 0;
}

#pragma STDC CX_LIMITED_RANGE OFF

static int disabled(void) {
    PROBE return 0;
}

#pragma STDC CX_LIMITED_RANGE DEFAULT

static int restored(void) {
    PROBE return 0;
}

static int nested(void) {
 #pragma STDC CX_LIMITED_RANGE ON
    PROBE{
  #pragma STDC CX_LIMITED_RANGE OFF
        PROBE{_Pragma("STDC CX_LIMITED_RANGE ON") PROBE} PROBE} PROBE return 0;
}

int main(void) {
    int r;
    if ((r = absent()))
        return 10 + r;
    if ((r = enabled()))
        return 20 + r;
    if ((r = disabled()))
        return 30 + r;
    if ((r = restored()))
        return 40 + r;
    if ((r = nested()))
        return 50 + r;
    return 0;
}
