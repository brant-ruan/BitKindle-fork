#include <math.h>
#include <errno.h>
#include <float.h>

static int domain(double (*f)(double), double x) {
    double value;
    errno = 0;
    value = f(x);
    return errno == EDOM && value != value;
}

int main(void) {
    double value;
    if (!domain(acos, 2) || !domain(asin, -2) || !domain(sqrt, -1))
        return 1;
    if (!domain(log, -1) || !domain(log10, -1))
        return 2;
    errno = 0;
    value = pow(-2, 0.5);
    if (errno != EDOM || value == value)
        return 3;
    errno = 0;
    value = fmod(1, 0);
    if (errno != EDOM || value == value)
        return 4;
    errno = 0;
    value = log(0);
    if (errno != ERANGE || value != -HUGE_VAL)
        return 5;
    errno = 0;
    value = log10(0);
    if (errno != ERANGE || value != -HUGE_VAL)
        return 6;
    errno = 0;
    value = pow(0, -1);
    if (errno != EDOM || value == value)
        return 7;
    errno = 0;
    value = exp(1000);
    if (errno != ERANGE || value != HUGE_VAL)
        return 8;
    errno = 0;
    value = exp(-1000);
    if (errno != ERANGE || value != 0)
        return 9;
    errno = 0;
    value = ldexp(DBL_MIN, -1);
    if (errno || value != DBL_MIN * 0.5)
        return 10;
    errno = 0;
    value = ldexp(DBL_MIN, -1000);
    if (errno != ERANGE || value != 0)
        return 11;
    errno = 0;
    value = pow(0, 0);
    if (errno || value != 1)
        return 12;
    errno = 0;
    value = atan2(0, 0);
    if (errno || value != 0)
        return 13;
    return 0;
}
