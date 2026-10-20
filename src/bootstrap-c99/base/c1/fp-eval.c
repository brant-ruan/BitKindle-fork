/* Raw IEEE arithmetic for the constant evaluator. This source is compiled only
   by the bootstrap compiler. No external assembler or binary patch is used.
   The two leaf routines save MXCSR in their caller-provided home area, select
   nearest-even with gradual underflow, and restore every original MXCSR bit.
   Arguments are integer bit carriers in RCX/RDX, operation in R8D.
   Only volatile RAX, XMM0 and XMM1 are changed. */
unsigned long long fpe_arithmetic(int type, int op, unsigned long long a, unsigned long long b) {
    unsigned long long (*leaf)(unsigned long long, unsigned long long, int);
    if (type == 13) {
        leaf = (unsigned long long (*)(unsigned long long, unsigned long long,
                                       int)) "\x0f\xae\x5c\x24\x08"
                                             "\xc7\x44\x24\x0c\x80\x1f\x00\x00"
                                             "\x0f\xae\x54\x24\x0c"
                                             "\x66\x0f\x6e\xc1\x66\x0f\x6e\xca"
                                             "\x41\x83\xf8\x2b\x75\x06\xf3\x0f\x58\xc1\xeb\x1c"
                                             "\x41\x83\xf8\x2d\x75\x06\xf3\x0f\x5c\xc1\xeb\x10"
                                             "\x41\x83\xf8\x2a\x75\x06\xf3\x0f\x59\xc1\xeb\x04"
                                             "\xf3\x0f\x5e\xc1"
                                             "\x66\x0f\x7e\xc0\x0f\xae\x54\x24\x08\xc3";
    } else {
        leaf = (unsigned long long (*)(unsigned long long, unsigned long long,
                                       int)) "\x0f\xae\x5c\x24\x08"
                                             "\xc7\x44\x24\x0c\x80\x1f\x00\x00"
                                             "\x0f\xae\x54\x24\x0c"
                                             "\x66\x48\x0f\x6e\xc1\x66\x48\x0f\x6e\xca"
                                             "\x41\x83\xf8\x2b\x75\x06\xf2\x0f\x58\xc1\xeb\x1c"
                                             "\x41\x83\xf8\x2d\x75\x06\xf2\x0f\x5c\xc1\xeb\x10"
                                             "\x41\x83\xf8\x2a\x75\x06\xf2\x0f\x59\xc1\xeb\x04"
                                             "\xf2\x0f\x5e\xc1"
                                             "\x66\x48\x0f\x7e\xc0\x0f\xae\x54\x24\x08\xc3";
    }
    return leaf(a, b, op);
}

int fpe_truth(int type, unsigned long long bits) {
    if (type == 13)
        return (bits & 0x7fffffffULL) != 0;
    if (type == 14 || type == 15)
        return (bits & 0x7fffffffffffffffULL) != 0;
    return bits != 0;
}

int fpe_finite(int type, unsigned long long bits) {
    if (type == 13)
        return (bits & 0x7f800000ULL) != 0x7f800000ULL;
    return (bits & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL;
}

/* Comparison result -1, 0, 1; unordered is 2. No host floating operations. */
int fpe_compare(int type, unsigned long long a, unsigned long long b) {
    unsigned long long sign, mask, exponent, aa, bb;
    sign = type == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
    mask = sign - 1;
    exponent = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    aa = a & mask;
    bb = b & mask;
    if (aa > exponent || bb > exponent)
        return 2;
    if (a == b || (!aa && !bb))
        return 0;
    if ((a & sign) != (b & sign))
        return (a & sign) ? -1 : 1;
    if (a & sign)
        return aa > bb ? -1 : 1;
    return aa < bb ? -1 : 1;
}

unsigned long long fpe_bits;

int fpe_integer_width(int t) {
    if (t == 5 || t == 6 || t == 9)
        return 8;
    if (t == 7 || t == 8)
        return 16;
    if (t == 10 || t == 11)
        return 64;
    return 32;
}

int fpe_unsigned(int t) {
    return t == 2 || t == 4 || t == 6 || t == 8 || t == 11 || t == 12;
}

/* Convert a finite scalar constant using only integer arithmetic.
   Return 1 for a value outside the destination range, 0 for success.
   Integer sources are normalized here, including narrow signed types.
   Floating to integer truncates before checking the destination bounds. */
int fpe_convert(int from, int to, unsigned long long bits) {
    int negative = 0, exponent = 0, width, field, status;
    unsigned long long magnitude, sign, mask, limit;
    fpe_bits = 0;
    if (to == 12) {
        fpe_bits = fpe_truth(from, bits);
        return 0;
    }
    if (from >= 13) {
        if (!fpe_finite(from, bits)) {
            if (to < 13)
                return 1;
            sign = from == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
            negative = (bits & sign) != 0;
            magnitude = bits & (from == 13 ? 0x7fffffULL : 0xfffffffffffffULL);
            if (from == 13 && to != 13)
                magnitude <<= 29;
            else if (from != 13 && to == 13) {
                bits = magnitude;
                magnitude >>= 29;
                if (bits && !magnitude)
                    magnitude = 1;
            }
            fpe_bits = (to == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL) | magnitude;
            if (negative)
                fpe_bits |= to == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
            return 0;
        }
        width = from == 13 ? 23 : 52;
        sign = from == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
        negative = (bits & sign) != 0;
        field = (int)((bits >> (unsigned int)width) & (from == 13 ? 255ULL : 2047ULL));
        magnitude = bits & ((1ULL << width) - 1);
        if (field)
            magnitude |= 1ULL << width;
        exponent = (field ? field : 1) - (from == 13 ? 127 : 1023) - width;
    } else {
        width = fpe_integer_width(from);
        mask = width == 64 ? ~0ULL : (1ULL << width) - 1;
        bits &= mask;
        negative = !fpe_unsigned(from) && (bits & (1ULL << (width - 1))) != 0;
        magnitude = negative ? (0ULL - bits) & mask : bits;
    }
    if (to >= 13) {
        sign = to == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
        if (!magnitude) {
            fpe_bits = negative ? sign : 0;
            return 0;
        }
        fp_type = to;
        fp_bits = 0;
        fp_inexact = 0;
        fp_underflow = 0;
        fp_nn = 0;
        fp_dn = 1;
        fp_den[0] = 1;
        while (magnitude) {
            fp_num[fp_nn++] = (int)(magnitude & 65535);
            magnitude >>= 16;
        }
        status = fp_round(exponent);
        if (status)
            return 1;
        fpe_bits = fp_bits | (negative ? sign : 0);
        return 0;
    }
    if (from < 13) {
        /* Integer to integer casts keep the target-width low bits. */
        magnitude = bits;
        if (negative)
            magnitude |= ~mask;
    } else if (exponent >= 0) {
        if (magnitude && (exponent >= 64 || magnitude > (~0ULL >> exponent)))
            return 1;
        if (magnitude)
            magnitude <<= exponent;
    } else {
        magnitude = exponent <= -64 ? 0 : magnitude >> (-exponent);
    }
    width = fpe_integer_width(to);
    mask = width == 64 ? ~0ULL : (1ULL << width) - 1;
    if (from >= 13) {
        if (fpe_unsigned(to)) {
            if ((negative && magnitude) || magnitude > mask)
                return 1;
        } else {
            limit = 1ULL << (width - 1);
            if (magnitude > (negative ? limit : limit - 1))
                return 1;
        }
        if (negative)
            magnitude = 0ULL - magnitude;
    }
    magnitude &= mask;
    if (!fpe_unsigned(to) && (magnitude & (1ULL << (width - 1))))
        magnitude |= ~mask;
    fpe_bits = magnitude;
    return 0;
}
