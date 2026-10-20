#include <tgmath.h>
#include <tgmath.h>
#include "src/bootstrap-c99/complex/tgmath-matrix.c"

int ff(void) {
    return 1;
}

int fd(void) {
    return 2;
}

int fl(void) {
    return 3;
}

int cf(void) {
    return 4;
}

int cd(void) {
    return 5;
}

int cl(void) {
    return 6;
}

#define RANK(x) __c1_tg(1,(x),ff,fd,fl,cf,cd,cl)()
static int calls;

static float arg(void) {
    calls++;
    return 2.0f;
}

int main(void) {
    int e = 0, n = 0;
    float x = 8;
    volatile float v = 0;
    double complex z = 3 + 4 * I, w;
    double (*function)(double) = (sin);
    if (RANK(sin(0.0f)) != 1 || RANK(sin(0.0)) != 2 || RANK(sin(0.0L)) != 3)
        return 1;
    if (RANK(sin(I)) != 4 || RANK(sin((double complex)I)) != 5 ||
        RANK(sin((long double complex)I)) != 6)
        return 2;
    if (RANK(pow(2.0f, 2.0f)) != 1 || RANK(pow(2.0f, 2)) != 2 || RANK(pow(I, 2)) != 5 ||
        RANK(pow(I, 2.0L)) != 6)
        return 3;
    if (RANK(fabs(I)) != 1 || RANK(fabs(z)) != 2 || fabs(z) != 5)
        return 4;
    if (RANK(conj(2)) != 5 || RANK(conj(2.0f)) != 4 || RANK(conj(2.0L)) != 6)
        return 5;
    if (creal(z) != 3 || cimag(z) != 4 || conj(z) != 3 - 4 * I || cproj(z) != z)
        return 6;
    if (RANK(frexp(x, &e)) != 1 || RANK(ldexp(x, 2)) != 1 || RANK(nexttoward(x, 9.0L)) != 1)
        return 7;
    if (frexp(x, &e) != 0.5f || e != 4 || ldexp(1.0f, n++) != 1 || n != 1)
        return 8;
    if (pow(arg(), arg()) != 4 || calls != 2)
        return 9;
    if (sin(v++) != 0 || v != 1)
        return 10;
    if (function(0) != 0 || (sin)(0) != 0)
        return 11;
    w = sqrt(pow(z, 2));
    if (fabs(w - z) > 1e-12)
        return 12;
    if (RANK(fmin(1.0f, 2)) != 2 || RANK(copysign(1.0f, -2.0L)) != 3)
        return 13;
    if (RANK(creal((long double complex)I)) != 3 || RANK(cimag(I)) != 1)
        return 14;
    if (sizeof(pow(1.0f, 2)) != sizeof(double) || sizeof(pow(1.0f, 2.0f)) != sizeof(float))
        return 15;
    if (macro_matrix())
        return 16;
    if (RANK(fdim(1.0f, 0)) != 2 || RANK(fdim(1.0f, 0.0f)) != 1 || RANK(fdim(1.0L, 0.0f)) != 3)
        return 17;
    if (RANK(remquo(7.0f, 2.0f, &e)) != 1 || RANK(remquo(7.0f, 2, &e)) != 2 ||
        RANK(remainder(7.0L, 2.0f)) != 3)
        return 18;
    {
        int q[1], k = 0;
        if (remquo(7.0f, 2.0f, &q[k++]) != -1 || k != 1 || q[0] != 4)
            return 19;
    }
    if (RANK(cbrt(8)) != 2 || RANK(cbrt(8.0f)) != 1 || RANK(cbrt(8.0L)) != 3)
        return 20;
    return 0;
}
