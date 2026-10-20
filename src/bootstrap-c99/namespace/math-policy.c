#include <math.h>
#include <errno.h>

/* Paired public-interface policy checks. */
static int underflow_result(double x) {
    if (errno != ERANGE)
        return 0;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    return x != 0.0;
#else
    return x == 0.0;
#endif
}

static int check_math(void) {
    double tiny, x;
    double exponent;
    int j;
    tiny = ldexp(1.0, -1074);
    errno = 0;
    x = exp(-710.0);
    if (!underflow_result(x))
        return 1;
    errno = 0;
    x = pow(2.0, -1074.5);
    if (!underflow_result(x))
        return 2;
    errno = 0;
    x = sin(tiny);
    if (!underflow_result(x))
        return 3;
    errno = 0;
    x = tan(tiny);
    if (!underflow_result(x))
        return 4;
    errno = 0;
    x = sinh(tiny);
    if (!underflow_result(x))
        return 5;
    errno = 0;
    x = tanh(tiny);
    if (!underflow_result(x))
        return 6;
    errno = 0;
    x = asin(tiny);
    if (!underflow_result(x))
        return 7;
    errno = 0;
    x = atan(tiny);
    if (!underflow_result(x))
        return 8;
    errno = 0;
    x = atan2(tiny, 1.0);
    if (!underflow_result(x))
        return 16;
    errno = ERANGE;
    if (exp(0.0) != 1.0 || errno != ERANGE)
        return 9;
    errno = 123;
    if (sin(0.0) != 0.0 || errno != 123)
        return 10;
    errno = ERANGE;
    if (pow(2.0, -1074.0) != tiny || errno != ERANGE)
        return 11;
    errno = 123;
    x = asin(2.0);
    if (errno != EDOM)
        return 12;
    errno = 0;
    x = exp(1000.0);
    if (x != HUGE_VAL || errno != ERANGE)
        return 13;
    errno = 0;
    x = sinh(-1000.0);
    if (x != -HUGE_VAL || errno != ERANGE)
        return 14;
    errno = 123;
    if (cos(0.0) != 1.0 || cosh(0.0) != 1.0 || acos(1.0) != 0.0 || errno != 123)
        return 15;
    for (j = 0; j < 3; j++) {
        exponent = j == 0 ? -1.0 : j == 1 ? -2.0 : -0.5;
        errno = 0;
        x = pow(0.0, exponent);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        if (errno != ERANGE || x != HUGE_VAL)
            return 17;
#else
        if (errno != EDOM)
            return 18;
#endif
        errno = 0;
        x = pow(-0.0, exponent);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        if (errno != ERANGE || x != (j == 0 ? -HUGE_VAL : HUGE_VAL))
            return 19;
#else
        if (errno != EDOM)
            return 20;
#endif
    }
    errno = 123;
    if (pow(0.0, 0.0) != 1.0 || pow(0.0, 2.0) != 0.0 || errno != 123)
        return 21;
    return 0;
}

#include <math.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <fenv.h>

/* x64 test harness: set a masked MXCSR with the requested rounding mode. */
static void test_round(int mode) {
    ((void (*)(unsigned int)) "\x89\x4c\x24\x08\x0f\xae\x54\x24\x08\xc3")(0x1f80U |
                                                                          (unsigned int)mode);
}

static int check_scale(void) {
    double x, tiny;
    int i, mode;
    tiny = ldexp(1.0, -1074);
    if (tiny <= 0.0)
        return 1;
    for (i = -1; i <= 1; i += 2) {
        errno = 0;
        x = ldexp(1.5 * i, -1074);
        if (errno != ERANGE)
            return 2;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        if (x != 2.0 * tiny * i)
            return 3;
#else
        if (x != 0.0)
            return 4;
#endif
        errno = 0;
        x = ldexp(DBL_MAX * i, 1);
        if (x != HUGE_VAL * i || errno != ERANGE)
            return 5;
        errno = 123;
        x = ldexp(DBL_MIN * i, -1);
        if (x != DBL_MIN * 0.5 * i || errno != 123)
            return 6;
        errno = 0;
        x = ldexp(1.0 * i, INT_MIN);
        if (x != 0.0 || errno != ERANGE)
            return 7;
        errno = 0;
        x = ldexp(1.0 * i, INT_MAX);
        if (x != HUGE_VAL * i || errno != ERANGE)
            return 8;
    }
    errno = 123;
    if (ldexp(1.25, 3) != 10.0 || errno != 123)
        return 9;
    if (sqrt(4.0) != 2.0 || fmod(5.5, 2.0) != 1.5)
        return 10;
    for (mode = 0; mode < 4; mode++) {
        test_round(mode == 0   ? FE_TONEAREST
                   : mode == 1 ? FE_DOWNWARD
                   : mode == 2 ? FE_UPWARD
                               : FE_TOWARDZERO);
        for (i = -1; i <= 1; i += 2) {
            errno = 0;
            x = ldexp(1.5 * i, -1074);
            if (errno != ERANGE)
                return 11;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
            if (x != tiny * i *
                         ((mode == 0 || (mode == 1 && i < 0) || (mode == 2 && i > 0)) ? 2.0 : 1.0))
                return 12;
#else
            if (x != 0.0)
                return 13;
#endif
            errno = 0;
            x = ldexp(DBL_MAX * i, 1);
            if (errno != ERANGE)
                return 14;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
            if (x != i * ((mode == 0 || (mode == 1 && i < 0) || (mode == 2 && i > 0)) ? HUGE_VAL
                                                                                      : DBL_MAX))
                return 15;
#else
            if (x != i * HUGE_VAL)
                return 16;
#endif
            errno = 123;
            if (ldexp(DBL_MIN * i, -1) != DBL_MIN * 0.5 * i || errno != 123)
                return 17;
        }
    }
    test_round(FE_TONEAREST);
    return 0;
}

int main(void) {
    int r = check_math();
    if (r)
        return r;
    return check_scale();
}
