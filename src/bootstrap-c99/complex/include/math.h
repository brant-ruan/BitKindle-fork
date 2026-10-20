#ifndef __C1_WORKING_MATH_H
#define __C1_WORKING_MATH_H
#include "src/bootstrap-c99/include/math-base.h"
#if !defined(__C1_C89__) || defined(__C1_LIBRARY_EXTENSIONS)
float asinhf(float);
double asinh(double);
long double asinhl(long double);
float acoshf(float);
double acosh(double);
long double acoshl(long double);
float atanhf(float);
double atanh(double);
long double atanhl(long double);
float fdimf(float, float);
double fdim(double, double);
long double fdiml(long double, long double);
float hypotf(float, float);
double hypot(double, double);
long double hypotl(long double, long double);
float remainderf(float, float);
double remainder(double, double);
long double remainderl(long double, long double);
float remquof(float, float, int *);
double remquo(double, double, int *);
long double remquol(long double, long double, int *);
float cbrtf(float);
double cbrt(double);
long double cbrtl(long double);
float fmaf(float, float, float);
double fma(double, double, double);
long double fmal(long double, long double, long double);
float erff(float);
double erf(double);
long double erfl(long double);
float erfcf(float);
double erfc(double);
long double erfcl(long double);
float lgammaf(float);
double lgamma(double);
long double lgammal(long double);
float tgammaf(float);
double tgamma(double);
long double tgammal(long double);
float exp2f(float);
double exp2(double);
long double exp2l(long double);
#endif
#endif
