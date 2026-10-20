#include "src/bootstrap-c99/base/c1/fp-convert.c"
#include "src/bootstrap-c99/base/c1/fp-eval.c"

int main(void) {
    unsigned int (*get)(void);
    void (*set)(unsigned int);
    unsigned int saved, mode;
    unsigned long long got;
    int i;
    get = (unsigned int (*)(void)) "\x0f\xae\x5c\x24\x08\x8b\x44\x24\x08\xc3";
    set = (void (*)(unsigned int)) "\x89\x4c\x24\x08\x0f\xae\x54\x24\x08\xc3";
    saved = get();
    /* Four rounding modes, DAZ/FTZ, and pre-existing exception status.
       Mask all exceptions so deliberately exceptional test inputs are safe. */
    for (i = 0; i < 4; i++) {
        mode = 0x9fc0U | ((unsigned int)i << 13) | 0x25U;
        set(mode);
        got = fpe_arithmetic(13, '+', 0x3f800000, 0x40000000);
        if (got != 0x40400000 || get() != mode)
            return 1;
        if (fpe_arithmetic(13, '-', 0x3f800000, 0x40000000) != 0xbf800000 || get() != mode)
            return 2;
        if (fpe_arithmetic(13, '*', 0x3fc00000, 0x40000000) != 0x40400000 || get() != mode)
            return 3;
        if (fpe_arithmetic(13, '/', 0x3f800000, 0x40400000) != 0x3eaaaaab || get() != mode)
            return 4;
        if (fpe_arithmetic(14, '+', 0x3ff0000000000000ULL, 0x4000000000000000ULL) !=
                0x4008000000000000ULL ||
            get() != mode)
            return 5;
        if (fpe_arithmetic(14, '-', 0x3ff0000000000000ULL, 0x4000000000000000ULL) !=
                0xbff0000000000000ULL ||
            get() != mode)
            return 6;
        if (fpe_arithmetic(14, '*', 0x3ff8000000000000ULL, 0x4000000000000000ULL) !=
                0x4008000000000000ULL ||
            get() != mode)
            return 7;
        if (fpe_arithmetic(15, '/', 0x3ff0000000000000ULL, 0x4008000000000000ULL) !=
                0x3fd5555555555555ULL ||
            get() != mode)
            return 8;
        /* Halfway ties, signed zeros, subnormal input and output. */
        if (fpe_arithmetic(13, '+', 0x3f800000, 0x33800000) != 0x3f800000 || get() != mode)
            return 9;
        if (fpe_arithmetic(13, '+', 0x3f800001, 0x33800000) != 0x3f800002 || get() != mode)
            return 10;
        if (fpe_arithmetic(14, '+', 0x3ff0000000000000ULL, 0x3ca0000000000000ULL) !=
                0x3ff0000000000000ULL ||
            get() != mode)
            return 11;
        if (fpe_arithmetic(14, '+', 0x3ff0000000000001ULL, 0x3ca0000000000000ULL) !=
                0x3ff0000000000002ULL ||
            get() != mode)
            return 12;
        if (fpe_arithmetic(13, '+', 0x80000000, 0x80000000) != 0x80000000 || get() != mode)
            return 13;
        if (fpe_arithmetic(14, '+', 0x8000000000000000ULL, 0) != 0 || get() != mode)
            return 14;
        if (fpe_arithmetic(13, '/', 1, 0x40000000) != 0 || get() != mode)
            return 15;
        if (fpe_arithmetic(13, '/', 3, 0x40000000) != 2 || get() != mode)
            return 16;
        if (fpe_arithmetic(14, '/', 3, 0x4000000000000000ULL) != 2 || get() != mode)
            return 17;
        if (fpe_arithmetic(14, '+', 1, 1) != 2 || get() != mode)
            return 18;
        if (fpe_arithmetic(13, '*', 0x7f7fffff, 0x40000000) != 0x7f800000 || get() != mode)
            return 19;
        if (fpe_arithmetic(14, '/', 0x3ff0000000000000ULL, 0) != 0x7ff0000000000000ULL ||
            get() != mode)
            return 20;
        if (fpe_finite(13, fpe_arithmetic(13, '/', 0, 0)) || get() != mode)
            return 21;
    }
    set(saved);
    if (fpe_truth(13, 0x80000000) || fpe_truth(14, 0x8000000000000000ULL))
        return 22;
    if (!fpe_truth(13, 0x7fc00000) || !fpe_truth(14, 1))
        return 23;
    if (fpe_compare(13, 0x80000000, 0) || fpe_compare(14, 0, 0x8000000000000000ULL))
        return 24;
    if (fpe_compare(13, 0x7fc00000, 0) != 2 || fpe_compare(14, 0, 0x7ff0000000000001ULL) != 2)
        return 25;
    if (fpe_compare(13, 0xbf800000, 0xc0000000) != 1 || fpe_compare(14, 1, 0) != 1)
        return 26;
    if (fpe_compare(14, 0xfff0000000000000ULL, 0) != -1 ||
        fpe_compare(13, 0x7f800000, 0x7f7fffff) != 1)
        return 27;
    if (fpe_convert(11, 14, ~0ULL) || fpe_bits != 0x43f0000000000000ULL)
        return 28;
    if (fpe_convert(10, 13, 0x8000000000000000ULL) || fpe_bits != 0xdf000000)
        return 29;
    if (fpe_convert(5, 14, 255) || fpe_bits != 0xbff0000000000000ULL)
        return 30;
    if (fpe_convert(6, 14, 255) || fpe_bits != 0x406fe00000000000ULL)
        return 31;
    if (fpe_convert(14, 13, 0x3ff0000010000000ULL) || fpe_bits != 0x3f800000)
        return 32;
    if (fpe_convert(14, 13, 0x3ff0000030000000ULL) || fpe_bits != 0x3f800002)
        return 33;
    if (fpe_convert(13, 14, 1) || fpe_bits != 0x36a0000000000000ULL)
        return 34;
    if (fpe_convert(14, 13, 0x3690000000000000ULL) || fpe_bits != 0)
        return 35;
    if (fpe_convert(14, 13, 0xb690000000000000ULL) || fpe_bits != 0x80000000)
        return 36;
    if (!fpe_convert(14, 13, 0x47f0000000000000ULL))
        return 37;
    if (fpe_convert(14, 11, 0x43efffffffffffffULL) || fpe_bits != 0xfffffffffffff800ULL)
        return 38;
    if (!fpe_convert(14, 11, 0x43f0000000000000ULL))
        return 39;
    if (fpe_convert(14, 10, 0xc3e0000000000000ULL) || fpe_bits != 0x8000000000000000ULL)
        return 40;
    if (!fpe_convert(14, 10, 0x43e0000000000000ULL))
        return 41;
    if (fpe_convert(14, 2, 0xbfefffffffffffffULL) || fpe_bits != 0)
        return 42;
    if (!fpe_convert(14, 2, 0xbff0000000000000ULL))
        return 43;
    if (fpe_convert(14, 5, 0x405fffffffffffffULL) || fpe_bits != 127)
        return 44;
    if (fpe_convert(14, 5, 0xc060100000000000ULL) || fpe_bits != 0xffffffffffffff80ULL)
        return 45;
    if (!fpe_convert(14, 5, 0x4060000000000000ULL))
        return 46;
    if (fpe_convert(13, 12, 0x7fc00000) || fpe_bits != 1)
        return 47;
    if (fpe_convert(14, 12, 0x8000000000000000ULL) || fpe_bits != 0)
        return 48;
    if (fpe_convert(5, 11, 255) || fpe_bits != ~0ULL)
        return 49;
    if (fpe_convert(11, 7, 0xffff) || fpe_bits != ~0ULL)
        return 50;
    return 0;
}
