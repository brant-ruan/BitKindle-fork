#include <math.h>
#include <complex.h>

int main(int argc, char **argv) {
    if (argc != 2 || argv[1][0] != 120)
        return 9;
    double complex z = 1 + 2 * I;
    if (creal(z * z) != -3 || cimag(z * z) != 4 || fma(2, 3, 4) != 10 || tgamma(5) != 24)
        return 1;
    return 0;
}
