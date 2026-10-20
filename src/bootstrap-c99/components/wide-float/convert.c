#ifndef __C1_WIDEFLOAT_C
#define __C1_WIDEFLOAT_C
#include "wchar.h"
#include <ctype.h>
#include <errno.h>
#include <fenv.h>
#include "core.c"

static int __c1_wf_match(const wchar_t *p, const char *word) {
    int a, b;
    while (*word) {
        a = *p;
        b = (unsigned char)*word;
        if (a >= 'A' && a <= 'Z')
            a += 'a' - 'A';
        if (a != b)
            return 0;
        p++;
        word++;
    }
    return 1;
}

static unsigned long long __c1_wf_finish(int status) {
    unsigned long long infinity = __c1_wf_type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long sign = __c1_wf_type == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
    int away = (__c1_wf_mode == 1 && __c1_wf_negative) || (__c1_wf_mode == 2 && !__c1_wf_negative);
    if (status == 2) {
        __c1_wf_bits = (__c1_wf_mode == 0 || away) ? infinity : infinity - 1;
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW | FE_INEXACT);
    } else if (status) {
        /* The bounded 1200-digit workspace makes this path unreachable for
           binary32/binary64; preserve a defensive resource failure result. */
        __c1_wf_bits = 0;
        errno = ERANGE;
    } else if (__c1_wf_underflow) {
        errno = ERANGE;
        feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
    } else if (__c1_wf_inexact)
        feraiseexcept(FE_INEXACT);
    return __c1_wf_bits | (__c1_wf_negative ? sign : 0);
}

static unsigned long long __c1_wf_parse(const wchar_t *input, wchar_t **end, int type) {
    const wchar_t *p = input, *mark, *q;
    long long significant = 0, fractional = 0, exponent = 0, dropped, order;
    int base = 10, point = 0, any = 0, first = 0, retained = 0, negative = 0, d, top, k, i, status;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long sign = type == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
    __c1_wf_type = type;
    __c1_wf_bits = 0;
    __c1_wf_inexact = 0;
    __c1_wf_underflow = 0;
    __c1_wf_sticky = 0;
    __c1_wf_mode = fegetround() >> 13;
    __c1_wf_nn = 1;
    __c1_wf_dn = 1;
    __c1_wf_num[0] = 0;
    __c1_wf_den[0] = 1;
    if (end)
        *end = (wchar_t *)input;
    while ((*p == 32 || (*p >= 9 && *p <= 13)))
        p++;
    __c1_wf_negative = *p == '-';
    if (*p == '-' || *p == '+')
        p++;
    if (__c1_wf_match(p, "inf")) {
        p += 3;
        if (__c1_wf_match(p, "inity"))
            p += 5;
        if (end)
            *end = (wchar_t *)p;
        return infinity | (__c1_wf_negative ? sign : 0);
    }
    if (__c1_wf_match(p, "nan")) {
        p += 3;
        if (*p == '(') {
            q = p + 1;
            while ((*q >= '0' && *q <= '9') || (*q >= 'a' && *q <= 'z') ||
                   (*q >= 'A' && *q <= 'Z') || *q == '_')
                q++;
            if (*q == ')')
                p = q + 1;
        }
        if (end)
            *end = (wchar_t *)p;
        /* Payload strings select the canonical quiet NaN in this implementation. */
        return infinity | (type == 13 ? 0x400000ULL : 0x8000000000000ULL) |
               (__c1_wf_negative ? sign : 0);
    }
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        if (__c1_wf_digit(p[2], 16) >= 0 || (p[2] == '.' && __c1_wf_digit(p[3], 16) >= 0)) {
            base = 16;
            p += 2;
        }
    }
    while (1) {
        d = __c1_wf_digit(*p, base);
        if (d >= 0) {
            any = 1;
            if (point)
                fractional++;
            if (significant || d) {
                if (!significant)
                    first = d;
                significant++;
                if (retained < 1200) {
                    __c1_wf_nn = __c1_wf_mul_add(__c1_wf_num, __c1_wf_nn, base, d);
                    retained++;
                } else if (d)
                    __c1_wf_sticky = 1;
            }
            p++;
        } else if (*p == '.' && !point) {
            point = 1;
            p++;
        } else
            break;
    }
    if (!any)
        return 0;
    if ((base == 10 && (*p == 'e' || *p == 'E')) || (base == 16 && (*p == 'p' || *p == 'P'))) {
        mark = p;
        q = p + 1;
        negative = *q == '-';
        if (*q == '-' || *q == '+')
            q++;
        if (*q >= '0' && *q <= '9') {
            do {
                if (exponent < 1000000000000LL) {
                    exponent = exponent * 10 + *q - '0';
                    if (exponent > 1000000000000LL)
                        exponent = 1000000000000LL;
                }
                q++;
            } while (*q >= '0' && *q <= '9');
            p = q;
            if (negative)
                exponent = -exponent;
        } else
            p = mark;
    }
    if (end)
        *end = (wchar_t *)p;
    if (!significant)
        return __c1_wf_negative ? sign : 0;
    dropped = significant - retained;
    if (base == 16) {
        top = 0;
        while (first > 1) {
            first >>= 1;
            top++;
        }
        order = (significant - 1) * 4 + top + exponent - fractional * 4;
        if (order > (type == 13 ? 127 : 1023))
            return __c1_wf_finish(2);
        if (order < (type == 13 ? -150 : -1075)) {
            __c1_wf_inexact = 1;
            __c1_wf_underflow = 1;
            __c1_wf_bits = ((__c1_wf_mode == 1 && __c1_wf_negative) ||
                            (__c1_wf_mode == 2 && !__c1_wf_negative))
                               ? 1
                               : 0;
            return __c1_wf_finish(0);
        }
        k = (int)(exponent - fractional * 4 + dropped * 4);
        return __c1_wf_finish(__c1_wf_round(k));
    }
    order = significant - 1 + exponent - fractional;
    if (order > (type == 13 ? 38 : 308))
        return __c1_wf_finish(2);
    if (order < (type == 13 ? -46 : -324)) {
        __c1_wf_inexact = 1;
        __c1_wf_underflow = 1;
        __c1_wf_bits =
            ((__c1_wf_mode == 1 && __c1_wf_negative) || (__c1_wf_mode == 2 && !__c1_wf_negative))
                ? 1
                : 0;
        return __c1_wf_finish(0);
    }
    k = (int)(exponent - fractional + dropped);
    if (k >= 0)
        for (i = 0; i < k; i++)
            __c1_wf_nn = __c1_wf_mul_add(__c1_wf_num, __c1_wf_nn, 5, 0);
    else
        for (i = 0; i < -k; i++)
            __c1_wf_dn = __c1_wf_mul_add(__c1_wf_den, __c1_wf_dn, 5, 0);
    status = __c1_wf_round(k);
    return __c1_wf_finish(status);
}

double wcstod(const wchar_t *s, wchar_t **end) {
    union {
        double f;
        unsigned long long u;
    } a;

    a.u = __c1_wf_parse(s, end, 14);
    return a.f;
}

float wcstof(const wchar_t *s, wchar_t **end) {
    union {
        float f;
        unsigned int u;
    } a;

    a.u = (unsigned int)__c1_wf_parse(s, end, 13);
    return a.f;
}

long double wcstold(const wchar_t *s, wchar_t **end) {
    union {
        long double f;
        unsigned long long u;
    } a;

    a.u = __c1_wf_parse(s, end, 15);
    return a.f;
}

#endif
