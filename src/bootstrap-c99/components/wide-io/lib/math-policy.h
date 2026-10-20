#ifndef __C1_MATH_POLICY_H
#define __C1_MATH_POLICY_H

/* Apply C89 range-result rules at public double interfaces only. Keeping this
   outside internal kernels avoids prematurely rounding their intermediates. */
static int __c1_math_policy_begin(int type) {
    int saved = errno;
#if defined(__C1_C89_MATH)
    if (type == 14)
        errno = 0;
#endif
    return saved;
}

static unsigned long long __c1_math_policy_end(unsigned long long bits, int type, int saved) {
#if defined(__C1_C89_MATH)
    unsigned long long magnitude, sign;
    if (type == 14) {
        if (!errno)
            errno = saved;
        else if (errno == ERANGE) {
            sign = bits & 0x8000000000000000ULL;
            magnitude = bits & 0x7fffffffffffffffULL;
            if (magnitude < 0x0010000000000000ULL)
                return sign;
            if (magnitude >= 0x7fefffffffffffffULL && magnitude <= 0x7ff0000000000000ULL)
                return sign | 0x7ff0000000000000ULL;
        }
    }
#endif
    return bits;
}
#endif
