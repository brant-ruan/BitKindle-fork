#include "src/bootstrap-c99/include/complex.h"

/* Windows binary32/binary64, little endian. Character copies preserve signed
   zero and NaN payloads and avoid accessing complex objects through real lvalues. */
static void __cx_copy(unsigned char *d, const unsigned char *s, int n) {
    int i;
    for (i = 0; i < n; i++)
        d[i] = s[i];
}

static int __cx_infinite(const unsigned char *p, int n) {
    int i;
    if ((p[n - 1] & 127) != 127)
        return 0;
    if (n == 4) {
        if (p[2] != 128)
            return 0;
    } else if (p[6] != 240)
        return 0;
    for (i = 0; i < n - 2; i++)
        if (p[i])
            return 0;
    return 1;
}

#define CX_BASIC(R,S) \
R creal##S(R complex z){R r;__cx_copy((unsigned char*)&r,(const unsigned char*)&z,sizeof r);return r;} \
R cimag##S(R complex z){R r;__cx_copy((unsigned char*)&r,(const unsigned char*)&z+sizeof r,sizeof r);return r;} \
R complex conj##S(R complex z){unsigned char *p=(unsigned char*)&z;p[sizeof z-1]^=128;return z;} \
R complex cproj##S(R complex z){ \
    unsigned char *p=(unsigned char*)&z;int n=sizeof(R),i,sign=p[sizeof z-1]&128; \
    if(__cx_infinite(p,n)||__cx_infinite(p+n,n)){ \
        for(i=0;i<sizeof z;i++)p[i]=0; \
        p[n-1]=127;p[n-2]=n==4?128:240;p[sizeof z-1]=sign; \
    } \
    return z; \
}
CX_BASIC(float, f)
CX_BASIC(double, )
CX_BASIC(long double, l)
