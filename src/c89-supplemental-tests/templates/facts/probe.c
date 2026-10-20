#include <limits.h>
#include <stddef.h>
#include <float.h>

struct Alignment {
    char c;
    void *p;
};

struct Bits {
    unsigned int a : 3;
    unsigned int b : 3;
};

int main(void) {
    unsigned int word = 0x01020304U;
    unsigned char *bytes = (unsigned char *)&word;
    struct Bits bits;
    int x = -7, y = 3;
    if (CHAR_BIT != 8 || sizeof(char) != 1 || sizeof(short) != 2)
        return 1;
    if (sizeof(int) != 4 || sizeof(long) != 4 || sizeof(void *) != 8)
        return 2;
    if (sizeof(size_t) != 8 || sizeof(ptrdiff_t) != 8 || sizeof(wchar_t) != 2)
        return 3;
    if ((char)255 != -1 || CHAR_MIN != -128 || CHAR_MAX != 127)
        return 4;
    if (bytes[0] != 4 || bytes[1] != 3 || bytes[2] != 2 || bytes[3] != 1)
        return 5;
    if (x / y != -2 || x % y != -1 || (x >> 1) != -4)
        return 6;
    if (offsetof(struct Alignment, p) != 8 || sizeof(struct Alignment) != 16)
        return 7;
    bits.a = 5;
    bits.b = 3;
    if ((*(unsigned char *)&bits & 63) != 29)
        return 8;
    if ('AB' != 0x4142)
        return 9;
    if (sizeof(float) != 4 || sizeof(double) != 8 || sizeof(long double) != 8)
        return 10;
    if (FLT_RADIX != 2 || FLT_MANT_DIG != 24 || DBL_MANT_DIG != 53 || LDBL_MANT_DIG != 53)
        return 11;
    return 0;
}
