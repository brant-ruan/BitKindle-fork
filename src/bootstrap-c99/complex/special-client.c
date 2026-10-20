#include "src/bootstrap-c99/complex/lib/internal.h"
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

int main(void) {
    double complex z;
    float complex f;
    long double complex l;
    z = csin(__cx_pair(NAN, INFINITY));
    if (!isnan(creal(z)) || !isinf(cimag(z)))
        return 1;
    z = ccos(__cx_pair(NAN, INFINITY));
    if (!isinf(creal(z)) || !isnan(cimag(z)))
        return 2;
    z = csinh(__cx_pair(INFINITY, NAN));
    if (!isinf(creal(z)) || !isnan(cimag(z)))
        return 3;
    z = ccosh(__cx_pair(INFINITY, NAN));
    if (!isinf(creal(z)) || !isnan(cimag(z)))
        return 4;
    feclearexcept(FE_ALL_EXCEPT);
    z = csin(__cx_pair(INFINITY, INFINITY));
    if (!isnan(creal(z)) || !isinf(cimag(z)) || !fetestexcept(FE_INVALID))
        return 5;
    feclearexcept(FE_ALL_EXCEPT);
    z = ccos(__cx_pair(INFINITY, INFINITY));
    if (!isinf(creal(z)) || !isnan(cimag(z)) || !fetestexcept(FE_INVALID))
        return 6;
    f = csinf(__cx_pairf(NAN, -INFINITY));
    if (!isnan(crealf(f)) || !isinf(cimagf(f)))
        return 7;
    f = ccoshf(__cx_pairf(-INFINITY, NAN));
    if (!isinf(crealf(f)) || !isnan(cimagf(f)))
        return 8;
    l = csinhl(__cx_pairl(-INFINITY, NAN));
    if (!isinf(creall(l)) || !isnan(cimagl(l)))
        return 9;
    l = ccosl(__cx_pairl(NAN, -INFINITY));
    if (!isinf(creall(l)) || !isnan(cimagl(l)))
        return 10;
    z = csin(__cx_pair(-0.0, NAN));
    if (creal(z) != 0 || !signbit(creal(z)) || !isnan(cimag(z)))
        return 11;
    z = ccos(__cx_pair(-0.0, NAN));
    if (!isnan(creal(z)) || cimag(z) != 0)
        return 12;
    z = csinh(__cx_pair(NAN, -0.0));
    if (!isnan(creal(z)) || cimag(z) != 0 || !signbit(cimag(z)))
        return 13;
    return 0;
}
