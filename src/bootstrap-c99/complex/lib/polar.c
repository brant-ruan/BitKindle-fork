#include "src/bootstrap-c99/include/complex.h"
#include "src/bootstrap-c99/complex/lib/hypot-core.h"
#define CX_POLAR(R,S) \
R cabs##S(R complex z){return __cx_hypot##S(creal##S(z),cimag##S(z));} \
R carg##S(R complex z){return atan2##S(cimag##S(z),creal##S(z));}
CX_POLAR(float, f)
CX_POLAR(double, )
CX_POLAR(long double, l)
#undef CX_POLAR
