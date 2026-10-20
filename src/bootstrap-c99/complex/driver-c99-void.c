#include <math.h>
#include <complex.h>

int main(void) {
    double complex z = 1 + 2 * I;
    if (creal(z * z) != -3 || cimag(z * z) != 4 || fma(2, 3, 4) != 10 || tgamma(5) != 24)
        return 1;
    return 0;
}
