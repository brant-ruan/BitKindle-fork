#include <complex.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

static int equal(const void *a, const void *b, int n) {
    const unsigned char *p = a, *q = b;
    int i;
    for (i = 0; i < n; i++)
        if (p[i] != q[i])
            return 0;
    return 1;
}

static void copy(void *a, const void *b, int n) {
    unsigned char *p = a;
    const unsigned char *q = b;
    int i;
    for (i = 0; i < n; i++)
        p[i] = q[i];
}

int main(void) {
    double complex z = 2 + 3 * I, w;
    float complex f = 4 - 5 * I;
    long double complex l = 6 + 7 * I;
    unsigned long long bits[2], result[2];
    unsigned int fb[2], fr[2];
    double r, im;
    double (*real_fn)(double complex) = creal;
    double complex (*conj_fn)(double complex) = conj;
    if (real_fn(z) != 2 || cimag(z) != 3 || crealf(f) != 4 || cimagf(f) != -5 || creall(l) != 6 ||
        cimagl(l) != 7)
        return 1;
    if (conj_fn(z) != 2 - 3 * I || conjf(f) != 4 + 5 * I || conjl(l) != 6 - 7 * I)
        return 2;
    if (cproj(z) != z || cprojf(f) != f || cprojl(l) != l)
        return 3;
    bits[0] = 0x8000000000000000ULL;
    bits[1] = 0x7ff8000000012345ULL;
    copy(&z, bits, 16);
    r = creal(z);
    im = cimag(z);
    if (!equal(&r, bits, 8) || !equal(&im, bits + 1, 8))
        return 4;
    w = conj(z);
    copy(result, &w, 16);
    if (result[0] != bits[0] || result[1] != (bits[1] ^ 0x8000000000000000ULL))
        return 5;
    w = cproj(z);
    if (!equal(&w, &z, 16))
        return 6;
    bits[0] = 0x7ff0000000000000ULL;
    bits[1] = 0xfff8000000000012ULL;
    copy(&z, bits, 16);
    w = cproj(z);
    copy(result, &w, 16);
    if (result[0] != 0x7ff0000000000000ULL || result[1] != 0x8000000000000000ULL)
        return 7;
    bits[0] = 0x7ff8000000001234ULL;
    bits[1] = 0x7ff0000000000000ULL;
    copy(&l, bits, 16);
    l = cprojl(l);
    copy(result, &l, 16);
    if (result[0] != 0x7ff0000000000000ULL || result[1] != 0)
        return 8;
    fb[0] = 0x7fc12345U;
    fb[1] = 0xff800000U;
    copy(&f, fb, 8);
    f = cprojf(f);
    copy(fr, &f, 8);
    if (fr[0] != 0x7f800000U || fr[1] != 0x80000000U)
        return 9;
    fb[0] = 0x80000000U;
    fb[1] = 0x7fc12345U;
    copy(&f, fb, 8);
    f = conjf(f);
    copy(fr, &f, 8);
    if (fr[0] != 0x80000000U || fr[1] != 0xffc12345U)
        return 10;
    bits[0] = 0x7ff0000000001234ULL;
    bits[1] = 0xfff0000000005678ULL;
    copy(&z, bits, 16);
    feclearexcept(FE_ALL_EXCEPT);
    r = creal(z);
    im = cimag(z);
    w = conj(z);
    w = cproj(w);
    if (fetestexcept(FE_ALL_EXCEPT))
        return 11;
    if (!equal(&r, bits, 8) || !equal(&im, bits + 1, 8))
        return 12;
    copy(result, &w, 16);
    if (result[0] != bits[0] || result[1] != (bits[1] ^ 0x8000000000000000ULL))
        return 13;
    return 0;
}
