#include "src/bootstrap-c99/complex/cross-api.h"
#include <stdarg.h>
D global_value = 3.5;
F global_array[3] = {1, 2, 3};

F float_echo(F x) {
    return x;
}

D double_echo(D x) {
    return x;
}

L long_echo(L x) {
    return x;
}

D make_value(double real, double imaginary) {
    double a[2];
    D x;
    unsigned char *s = (unsigned char *)a, *d = (unsigned char *)&x;
    int i;
    a[0] = real;
    a[1] = imaginary;
    for (i = 0; i < sizeof x; i++)
        d[i] = s[i];
    return x;
}

double imaginary(D x) {
    double a[2];
    unsigned char *s = (unsigned char *)&x, *d = (unsigned char *)a;
    int i;
    for (i = 0; i < sizeof x; i++)
        d[i] = s[i];
    return a[1];
}

D many(int a, D b, int c, F d, L e, D f) {
    return a + b + c + d + e + f;
}

D varied(int count, ...) {
    va_list ap;
    D result;
    F f;
    L l;
    va_start(ap, count);
    f = va_arg(ap, F);
    result = va_arg(ap, D);
    l = va_arg(ap, L);
    va_end(ap);
    return result + f + l;
}

struct Box box_echo(struct Box b) {
    b.tag++;
    b.value += 2;
    return b;
}

void change(D *p) {
    *p += make_value(1, 2);
}
