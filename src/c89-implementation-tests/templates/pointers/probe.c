#include <stddef.h>
#include <limits.h>
static int target = 42;

static int function(void) {
    return 37;
}

static int check(size_t n) {
    void *p = (void *)n;
    if ((size_t)p != n)
        return 1;
    if ((unsigned int)p != (unsigned int)n)
        return 2;
    if ((unsigned short)p != (unsigned short)n)
        return 3;
    if ((unsigned char)p != (unsigned char)n)
        return 4;
    return 0;
}

int main(void) {
    size_t n;
    unsigned int i, state = 12345U;
    int *p = &target;
    int (*f)(void) = function;
    char *bytes = (char *)p;
    if ((int *)bytes != p || *(int *)(size_t)p != 42)
        return 1;
    if (((int (*)(void))(size_t)f)() != 37)
        return 2;
    if ((size_t)(void *)(int)-1 != (size_t)-1)
        return 3;
    if ((size_t)(void *)UINT_MAX != (size_t)UINT_MAX)
        return 4;
    if ((size_t)(void *)(short)-32768 != (size_t)(ptrdiff_t)-32768)
        return 5;
    if ((size_t)(void *)(unsigned short)65535 != (size_t)65535)
        return 6;
    if ((size_t)(void *)(signed char)-128 != (size_t)(ptrdiff_t)-128)
        return 7;
    if ((size_t)(void *)(unsigned char)255 != (size_t)255)
        return 8;
    if ((void *)0 != NULL || (size_t)NULL != 0)
        return 9;
    if (check(0) || check((size_t)-1) || check((size_t)1 << 63))
        return 10;
    for (i = 0; i < 4096U; i++) {
        state = state * 1664525U + 1013904223U;
        n = (size_t)state << 32;
        state = state * 1664525U + 1013904223U;
        n |= state;
        if (check(n))
            return 11;
    }
    return 0;
}
