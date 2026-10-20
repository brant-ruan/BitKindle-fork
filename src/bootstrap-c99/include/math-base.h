#if defined(__C1_C89__) && !defined(__C1_LIBRARY_EXTENSIONS)
#ifndef __C1_MATH_FOUNDATION_H
#define __C1_MATH_FOUNDATION_H
#define HUGE_VAL __fpbits(double,0x7ff0000000000000ULL)
double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);
double cos(double);
double sin(double);
double tan(double);
double cosh(double);
double sinh(double);
double tanh(double);
double exp(double);
double frexp(double, int *);
double ldexp(double, int);
double log(double);
double log10(double);
double modf(double, double *);
double pow(double, double);
double sqrt(double);
double ceil(double);
double fabs(double);
double floor(double);
double fmod(double, double);
#endif
#else
#include "src/bootstrap-c99/base/include/math.h"
#endif