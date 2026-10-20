#ifndef __C1_MATH_FOUNDATION_H
#define __C1_MATH_FOUNDATION_H
/* Implemented subset. The remaining math.h interfaces are not yet present. */
typedef float float_t;
typedef double double_t;
#define HUGE_VAL __fpbits(double,0x7ff0000000000000ULL)
#define HUGE_VALF __fpbits(float,0x7f800000U)
#define HUGE_VALL __fpbits(long double,0x7ff0000000000000ULL)
#define INFINITY HUGE_VALF
#define NAN __fpbits(float,0x7fc00000U)
#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4
int __c1_fpclassifyf(float);
int __c1_fpclassifyl(long double);
int __c1_signbitf(float);
int __c1_signbitl(long double);
#define fpclassify(x) (sizeof(x)==sizeof(float)?__c1_fpclassifyf((x)):__c1_fpclassifyl((x)))
#define signbit(x) (sizeof(x)==sizeof(float)?__c1_signbitf((x)):__c1_signbitl((x)))
#define isfinite(x) (fpclassify(x)>FP_INFINITE)
#define isinf(x) (fpclassify(x)==FP_INFINITE)
#define isnan(x) (fpclassify(x)==FP_NAN)
#define isnormal(x) (fpclassify(x)==FP_NORMAL)
unsigned long long __c1_fpkeyf(float);
unsigned long long __c1_fpkeyl(long double);
unsigned int __c1_fpcmp(unsigned long long, unsigned long long);
#define __C1_FPKEY(x) (sizeof(x)==sizeof(float)?__c1_fpkeyf((x)):__c1_fpkeyl((x)))
#define __C1_FPCMP(x,y) __c1_fpcmp(__C1_FPKEY(x),__C1_FPKEY(y))
#define isgreater(x,y) (__C1_FPCMP(x,y)==2U)
#define isgreaterequal(x,y) ((__C1_FPCMP(x,y)-1U)<=1U)
#define isless(x,y) (__C1_FPCMP(x,y)==0U)
#define islessequal(x,y) (__C1_FPCMP(x,y)<=1U)
#define islessgreater(x,y) ((__C1_FPCMP(x,y)&1U)==0U)
#define isunordered(x,y) (__C1_FPCMP(x,y)==3U)
#define __C1_MATH_DECL(T,S) \
T fabs##S(T); T copysign##S(T,T); T frexp##S(T,int *); \
T modf##S(T,T *); T floor##S(T); T ceil##S(T); T trunc##S(T); T round##S(T); \
T ldexp##S(T,int); T scalbn##S(T,int); T scalbln##S(T,long); T fmod##S(T,T); T sqrt##S(T);
__C1_MATH_DECL(double, )
__C1_MATH_DECL(float, f)
__C1_MATH_DECL(long double, l)
#undef __C1_MATH_DECL
#define FP_ILOGB0 (-2147483647-1)
#define FP_ILOGBNAN 2147483647
#define __C1_MATH_EXT_DECL(T,S) \
T nextafter##S(T,T); T nexttoward##S(T,long double); \
T fmin##S(T,T); T fmax##S(T,T); int ilogb##S(T); T logb##S(T); \
T rint##S(T); T nearbyint##S(T); long lrint##S(T); long long llrint##S(T); \
long lround##S(T); long long llround##S(T);
__C1_MATH_EXT_DECL(double, )
__C1_MATH_EXT_DECL(float, f)
__C1_MATH_EXT_DECL(long double, l)
#undef __C1_MATH_EXT_DECL
double exp(double);
float expf(float);
long double expl(long double);
double log(double);
float logf(float);
long double logl(long double);
double log10(double);
float log10f(float);
long double log10l(long double);
double log2(double);
float log2f(float);
long double log2l(long double);
double sinh(double);
float sinhf(float);
long double sinhl(long double);
double cosh(double);
float coshf(float);
long double coshl(long double);
double tanh(double);
float tanhf(float);
long double tanhl(long double);
double atan(double);
float atanf(float);
long double atanl(long double);
double asin(double);
float asinf(float);
long double asinl(long double);
double acos(double);
float acosf(float);
long double acosl(long double);
double atan2(double, double);
float atan2f(float, float);
long double atan2l(long double, long double);
double sin(double);
float sinf(float);
long double sinl(long double);
double cos(double);
float cosf(float);
long double cosl(long double);
double tan(double);
float tanf(float);
long double tanl(long double);
double pow(double, double);
float powf(float, float);
long double powl(long double, long double);
double expm1(double);
float expm1f(float);
long double expm1l(long double);
double log1p(double);
float log1pf(float);
long double log1pl(long double);
#endif
