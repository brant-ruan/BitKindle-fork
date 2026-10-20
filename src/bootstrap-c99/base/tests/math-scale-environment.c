#include "src/bootstrap-c99/base/lib/math-scale.c"
#include "src/bootstrap-c99/base/lib/runtime.c"

int main(void) {
    fenv_t saved;
    unsigned int control;
    int i;

    union {
        double f;
        unsigned long long u;
    } a, b;

    union {
        float f;
        unsigned u;
    } x, y;

    fegetenv(&saved);
    for (i = 0; i < 4; i++) {
        control = 0x9fc1U | ((unsigned)i << 13);
        __c1_fe_write(control);
        errno = 123;
        a.u = 1;
        b.f = ldexp(a.f, 1);
        x.u = 1;
        y.f = scalbnf(x.f, 1);
        if (b.u != 2 || y.u != 2 || errno != 123 || __c1_fe_read() != control)
            return 1;
        b.f = scalbln(a.f, -1);
        if (b.u != (i == 2 ? 1ULL : 0ULL) || errno != ERANGE)
            return 2;
        if (__c1_fe_read() != (control | FE_UNDERFLOW | FE_INEXACT))
            return 3;
        __c1_fe_write(control);
        errno = 123;
        x.u = 0x80000001;
        y.f = ldexpf(x.f, -1);
        if (y.u != (i == 1 ? 0x80000001U : 0x80000000U) || errno != ERANGE)
            return 4;
        if (__c1_fe_read() != (control | FE_UNDERFLOW | FE_INEXACT))
            return 5;
    }
    fesetenv(&saved);
    return 0;
}
