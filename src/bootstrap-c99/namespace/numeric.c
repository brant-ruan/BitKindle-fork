#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

int main(void) {
    char *end, tail;
    const char *s;
    double x;
    float f;
    long double ld;
    int n;
    FILE *file;
    s = "0x1p2!";
    x = strtod(s, &end);
#if defined(__C1_C89__)
    if (x != 0 || end != s + 1 || atof(s) != 0)
        return 1;
    s = "inf";
    if (strtod(s, &end) != 0 || end != s)
        return 2;
    s = "nan(tag)";
    if (strtod(s, &end) != 0 || end != s)
        return 3;
    if (sscanf("0x1p2", "%lf%c", &x, &tail) != 2 || x != 0 || tail != 'x')
        return 4;
    x = 9;
    if (sscanf("inf", "%lf", &x) != 0 || x != 9)
        return 5;
    if (sscanf("nan", "%f", &f) != 0)
        return 6;
    if (sscanf("0x1p2", "%Lf%c", &ld, &tail) != 2 || ld != 0 || tail != 'x')
        return 7;
#else
    if (x != 4 || *end != '!' || atof(s) != 4)
        return 8;
    s = "inf!";
    x = strtod(s, &end);
    if (x <= 1e308 || *end != '!')
        return 9;
    s = "nan(tag)!";
    x = strtod(s, &end);
    if (x == x || *end != '!')
        return 10;
    if (sscanf("0x1p2", "%lf", &x) != 1 || x != 4)
        return 11;
    if (sscanf("inf", "%f", &f) != 1 || f <= 1e30F)
        return 12;
    if (sscanf("0x1p2", "%Lf", &ld) != 1 || ld != 4)
        return 13;
#endif
    if (sscanf("  -12.5e2!", "%lf%n%c", &x, &n, &tail) != 2 || x != -1250 || n != 9 || tail != '!')
        return 14;
    s = "1e+!";
    if (strtod(s, &end) != 1 || end != s + 1)
        return 15;
    file = tmpfile();
    if (!file || fputs("0x1p2!", file) < 0)
        return 16;
    rewind(file);
    if (fscanf(file, "%lf", &x) != 1)
        return 17;
#if defined(__C1_C89__)
    if (x != 0 || fgetc(file) != 'x')
        return 18;
#else
    if (x != 4 || fgetc(file) != '!')
        return 19;
#endif
    if (fclose(file))
        return 20;
    errno = 0;
    x = strtod("1e999", &end);
    if (x != HUGE_VAL || *end || errno != ERANGE)
        return 21;
    errno = 0;
    x = strtod("-1e999", &end);
    if (x != -HUGE_VAL || *end || errno != ERANGE)
        return 22;
    errno = 0;
    x = strtod("1e-999", &end);
    if (x != 0 || *end || errno != ERANGE)
        return 23;
    errno = 0;
    x = strtod("1e-310", &end);
    if (*end || errno != ERANGE)
        return 24;
#if defined(__C1_C89__)
    if (x != 0)
        return 25;
#else
    if (x <= 0)
        return 26;
#endif
    errno = 123;
    x = strtod("1.25!", &end);
    if (x != 1.25 || *end != '!' || errno != 123)
        return 27;
    return 0;
}
