#include <stddef.h>
#include <limits.h>
static signed char sc = (signed char)255;
static short ss = (short)65535;
static int si = (int)4294967295U;
static long sl = (long)4294967295UL;
static int zero = (int)((size_t)1 << 32);
static int negative = (int)(((size_t)1 << 32) - 1);

static int check(size_t value) {
    unsigned int low = (unsigned int)value;
    int byte = (int)(value & 255), half = (int)(value & 65535), word;
    signed char a = (signed char)value;
    short b = (short)value;
    int c = (int)value;
    long d = (long)value;
    if (byte >= 128)
        byte -= 256;
    if (half >= 32768)
        half -= 65536;
    word = low <= 2147483647U ? (int)low : -1 - (int)(~low);
    if (a != byte || b != half || c != word || d != word)
        return 1;
    if ((unsigned char)value != (value & 255))
        return 2;
    if ((unsigned short)value != (value & 65535))
        return 3;
    if ((unsigned long)value != (value & ((size_t)UINT_MAX)))
        return 4;
    return 0;
}

int main(void) {
    unsigned int i, state = 12345U;
    size_t v;
    if (sc != -1 || ss != -1 || si != -1 || sl != -1 || zero || negative != -1)
        return 1;
    for (i = 0; i < 65536U; i++)
        if (check((size_t)i))
            return 2;
    for (i = 0; i < 4096U; i++) {
        state = state * 1664525U + 1013904223U;
        v = (size_t)state << 32;
        state = state * 1664525U + 1013904223U;
        v |= state;
        if (check(v))
            return 3;
    }
    if (check((size_t)-1) || check((size_t)1 << 63))
        return 4;
    return 0;
}
