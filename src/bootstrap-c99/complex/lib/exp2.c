#include "src/bootstrap-c99/include/math.h"

/* The existing power implementation has an exact power-of-two integer
   exponent path and a compensated fractional-exponent path. */
float exp2f(float x) {
    return powf(2.0f, x);
}

double exp2(double x) {
    return pow(2.0, x);
}

long double exp2l(long double x) {
    return powl(2.0L, x);
}
