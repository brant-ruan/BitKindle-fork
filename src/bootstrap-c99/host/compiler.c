#include "src/bootstrap-c99/components/object-runtime/lib/winimports.h"
/* Integer-only conversion of C floating tokens to IEEE binary32/binary64.
   Built by the existing custom C compiler. No host floating arithmetic.
   fp_parse accepts an ASCII int array without a sign or whitespace.
   Return: 0 success, 1 invalid token, 2 overflow, 3 resource limit.
   fp_type: 13 float, 14 double, 15 long double (Windows binary64).
   fp_inexact and fp_underflow describe rounding; tininess is after rounding.
   Limits: 1 MiB token, 8192 significant mantissa digits, 32800-bit workspace.
   The workspaces make this helper non-reentrant. */
int fp_num[2050];
int fp_den[2050];
int fp_nn;
int fp_dn;
int fp_type;
int fp_inexact;
int fp_underflow;
unsigned long long fp_bits;

int fp_digit(int c, int base) {
    int d = -1;
    if (c >= '0' && c <= '9')
        d = c - '0';
    if (c >= 'a' && c <= 'f')
        d = c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        d = c - 'A' + 10;
    return d < base ? d : -1;
}

int fp_mul_add(int *a, int n, int m, int carry) {
    int i, x;
    for (i = 0; i < n; i++) {
        x = a[i] * m + carry;
        a[i] = x & 65535;
        carry = x >> 16;
    }
    if (carry) {
        if (n == 2050)
            return 0;
        a[n++] = carry;
    }
    return n;
}

int fp_bit_length(int *a, int n) {
    int bits = (n - 1) * 16, top = a[n - 1];
    while (top) {
        bits++;
        top >>= 1;
    }
    return bits;
}

int fp_shifted_word(int *a, int n, int shift, int word) {
    int k = word - shift / 16, r = shift % 16, x = 0;
    if (k >= 0 && k < n)
        x = (a[k] << r) & 65535;
    if (r && k > 0 && k <= n)
        x |= a[k - 1] >> (16 - r);
    return x;
}

int fp_compare(int *a, int an, int ash, int *b, int bn, int bsh) {
    int ab = fp_bit_length(a, an), bb = fp_bit_length(b, bn), i, x, y;
    if (ab)
        ab += ash;
    if (bb)
        bb += bsh;
    if (ab != bb)
        return ab > bb ? 1 : -1;
    for (i = (ab + 15) / 16 - 1; i >= 0; i--) {
        x = fp_shifted_word(a, an, ash, i);
        y = fp_shifted_word(b, bn, bsh, i);
        if (x != y)
            return x > y ? 1 : -1;
    }
    return 0;
}

int fp_shift(int *a, int n, int shift) {
    int words = shift / 16, r = shift % 16, extra, i, x;
    if (!fp_bit_length(a, n))
        return 1;
    extra = r && (a[n - 1] >> (16 - r)) != 0;
    if (n + words + extra > 2050)
        return 0;
    for (i = n + words + extra - 1; i >= 0; i--) {
        x = fp_shifted_word(a, n, shift, i);
        a[i] = x;
    }
    return n + words + extra;
}

int fp_subtract(int *a, int an, int *b, int bn, int shift) {
    int i, x, borrow = 0;
    for (i = 0; i < an; i++) {
        x = a[i] - fp_shifted_word(b, bn, shift, i) - borrow;
        borrow = x < 0;
        if (borrow)
            x += 65536;
        a[i] = x;
    }
    while (an > 1 && !a[an - 1])
        an--;
    return an;
}

int fp_round(int binary_exponent) {
    int p = fp_type == 13 ? 24 : 53, emin = fp_type == 13 ? -126 : -1022;
    int emax = fp_type == 13 ? 127 : 1023, bias = fp_type == 13 ? 127 : 1023;
    int e, ratio, cmp, qexp, shift, i;
    unsigned long long q = 0, leading = 1ULL << (p - 1);
    ratio = fp_bit_length(fp_num, fp_nn) - fp_bit_length(fp_den, fp_dn);
    if (ratio >= 0)
        cmp = fp_compare(fp_num, fp_nn, 0, fp_den, fp_dn, ratio);
    else
        cmp = fp_compare(fp_num, fp_nn, -ratio, fp_den, fp_dn, 0);
    if (cmp < 0)
        ratio--;
    e = ratio + binary_exponent;
    if (e > emax)
        return 2;
    if (e < emin - p) {
        fp_inexact = 1;
        fp_underflow = 1;
        return 0;
    }
    qexp = (e < emin ? emin : e) - (p - 1);
    shift = binary_exponent - qexp;
    if (shift >= 0)
        fp_nn = fp_shift(fp_num, fp_nn, shift);
    else
        fp_dn = fp_shift(fp_den, fp_dn, -shift);
    if (!fp_nn || !fp_dn)
        return 3;
    i = fp_bit_length(fp_num, fp_nn) - fp_bit_length(fp_den, fp_dn);
    if (i > 63)
        return 3;
    for (; i >= 0; i--) {
        if (fp_compare(fp_num, fp_nn, 0, fp_den, fp_dn, i) >= 0) {
            fp_nn = fp_subtract(fp_num, fp_nn, fp_den, fp_dn, i);
            q |= 1ULL << i;
        }
    }
    fp_inexact = fp_bit_length(fp_num, fp_nn) != 0;
    cmp = fp_compare(fp_num, fp_nn, 1, fp_den, fp_dn, 0);
    if (cmp > 0 || (cmp == 0 && (q & 1)))
        q++;
    if (e < emin) {
        fp_bits = q;
        fp_underflow = fp_inexact && q < leading;
    } else {
        if (q == (leading << 1)) {
            q >>= 1;
            e++;
        }
        if (e > emax)
            return 2;
        fp_bits = ((unsigned long long)(e + bias) << (p - 1)) | (q - leading);
    }
    return 0;
}

int fp_parse(int *s, int length) {
    int i = 0, base = 10, point = 0, exponent_seen = 0, digits = 0, fractional = 0;
    int significant = 0, first = 0, oversized = 0, d, c, exponent = 0, negative = 0;
    int k, order, top, precision, minimum, maximum;
    fp_bits = 0;
    fp_type = 14;
    fp_inexact = 0;
    fp_underflow = 0;
    fp_nn = 1;
    fp_dn = 1;
    fp_num[0] = 0;
    fp_den[0] = 1;
    if (length < 1)
        return 1;
    if (length > 1048576)
        return 3;
    if (length >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        i = 2;
    }
    while (i < length) {
        c = s[i];
        d = fp_digit(c, base);
        if (d >= 0) {
            digits++;
            if (point)
                fractional++;
            if (significant || d) {
                if (!significant)
                    first = d;
                significant++;
                if (significant > 8192)
                    oversized = 1;
                else {
                    fp_nn = fp_mul_add(fp_num, fp_nn, base, d);
                    if (!fp_nn)
                        oversized = 1;
                }
            }
            i++;
        } else if (c == '.' && !point) {
            point = 1;
            i++;
        } else
            break;
    }
    if (!digits)
        return 1;
    if (i < length && ((base == 10 && (s[i] == 'e' || s[i] == 'E')) ||
                       (base == 16 && (s[i] == 'p' || s[i] == 'P')))) {
        exponent_seen = 1;
        i++;
        if (i < length && (s[i] == '+' || s[i] == '-')) {
            negative = s[i] == '-';
            i++;
        }
        if (i == length || s[i] < '0' || s[i] > '9')
            return 1;
        while (i < length && s[i] >= '0' && s[i] <= '9') {
            if (exponent < 10000000) {
                exponent = exponent * 10 + s[i] - '0';
                if (exponent > 10000000)
                    exponent = 10000000;
            }
            i++;
        }
    }
    if (base == 16 && !exponent_seen)
        return 1;
    if (base == 10 && !point && !exponent_seen)
        return 1;
    if (i < length) {
        c = s[i++];
        if (c == 'f' || c == 'F')
            fp_type = 13;
        else if (c == 'l' || c == 'L')
            fp_type = 15;
        else
            return 1;
    }
    if (i != length)
        return 1;
    if (!significant)
        return 0;
    if (negative)
        exponent = -exponent;
    precision = fp_type == 13 ? 24 : 53;
    minimum = fp_type == 13 ? -126 : -1022;
    maximum = fp_type == 13 ? 127 : 1023;
    if (base == 16) {
        k = exponent - fractional * 4;
        top = 0;
        while (first > 1) {
            first >>= 1;
            top++;
        }
        order = (significant - 1) * 4 + top + k;
        if (order > maximum)
            return 2;
        if (order < minimum - precision) {
            fp_inexact = 1;
            fp_underflow = 1;
            return 0;
        }
        if (oversized)
            return 3;
        return fp_round(k);
    }
    k = exponent - fractional;
    order = significant - 1 + k;
    if (order > (fp_type == 13 ? 38 : 308))
        return 2;
    if (order < (fp_type == 13 ? -46 : -324)) {
        fp_inexact = 1;
        fp_underflow = 1;
        return 0;
    }
    if (oversized)
        return 3;
    if (k >= 0) {
        for (i = 0; i < k; i++) {
            fp_nn = fp_mul_add(fp_num, fp_nn, 5, 0);
            if (!fp_nn)
                return 3;
        }
    } else {
        for (i = 0; i < -k; i++) {
            fp_dn = fp_mul_add(fp_den, fp_dn, 5, 0);
            if (!fp_dn)
                return 3;
        }
    }
    return fp_round(k);
}

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

/* C1 compiler, built by the frozen C-written bootstrap compiler.
   Reads C from stdin and directly emits a Windows x64 PE image on stdout. */
int source[1048576];
int pp_condition;
int c99_mode;
int hosted_startup_call;
int c89_identifiers;
char *
    hosted_startup_source = "/* Self-contained startup component: only the existing kernel32 imports. */\nstatic int __c1_parse_arguments_cp(const char *text,int *count,char ***arguments,unsigned int cp){\n typedef int Lead(unsigned int,unsigned char);\n Lead *lead=*(Lead **)0x401118;\n typedef void *Heap(void);\n typedef void *Allocate(void *,unsigned int,unsigned long long);\n Heap *heap=*(Heap **)0x401080;\n Allocate *allocate=*(Allocate **)0x401088;\n unsigned long long length=0;\n const char *p;\n char **v,*out;\n int n=0,quoted=0,slashes,i;\n if(!text||!count||!arguments)return 1;\n while(text[length]){if(length==2147483646)return 1;length++;}\n v=allocate(heap(),0,(length+2)*9);\n if(!v)return 1;\n out=(char *)(v+length+2);p=text;\n /* argv[0] treats quotes specially, but backslashes literally. */\n v[n++]=out;\n while(*p){\n  if(*p=='\"'){quoted=!quoted;p++;continue;}\n  if(!quoted&&(*p==' '||*p=='\\t'))break;\n  if((unsigned char)*p>=128 && p[1] && lead(cp,(unsigned char)*p))*out++=*p++;\n  *out++=*p++;\n }\n *out++=0;\n for(;;){\n  while(*p==' '||*p=='\\t')p++;\n  if(!*p)break;\n  v[n++]=out;quoted=0;\n  while(*p){\n   if(!quoted&&(*p==' '||*p=='\\t'))break;\n   slashes=0;\n   while(*p=='\\\\'){slashes++;p++;}\n   if(*p=='\"'){\n    for(i=0;i<slashes/2;i++)*out++='\\\\';\n    if(slashes&1){*out++='\"';p++;}\n    else if(quoted&&p[1]=='\"'){*out++='\"';p+=2;}\n    else{quoted=!quoted;p++;}\n   }else{\n    for(i=0;i<slashes;i++)*out++='\\\\';\n    if(!*p||(!quoted&&(*p==' '||*p=='\\t')))break;\n    if((unsigned char)*p>=128 && p[1] && lead(cp,(unsigned char)*p))*out++=*p++;\n  *out++=*p++;\n   }\n  }\n  *out++=0;\n }\n v[n]=0;*count=n;*arguments=v;return 0;\n}\nstatic int __c1_parse_arguments(const char *text,int *count,char ***arguments){return __c1_parse_arguments_cp(text,count,arguments,0);}\nstatic int __c1_get_arguments(int *count,char ***arguments){\n typedef char *CommandLine(void);\n CommandLine *command=*(CommandLine **)0x4010d8;\n return __c1_parse_arguments(command(),count,arguments);\n}\n\nstatic int __c1_host_main(void){\n int argc; char **argv;\n typedef void Stop(unsigned int);\n Stop *stop=*(Stop **)0x401058;\n if(__c1_get_arguments(&argc,&argv)){stop(1);return 1;}\n return main(argc,argv);\n}";
unsigned char names[524288];
int lengths[2048];
int spelling_start[2048];
int spelling_length[2048];
int size;
int cursor;
int count;
int failed;
char *diagnostic_reason;
int value;
int literal_type;
int literal_high;
int string_pool[1048576];
int string_used;
int literal_string_start;
int literal_string_length;
int joined_string_start;
int joined_string_length;
int literal_string_wide;
int joined_string_wide;
int emitted_string_address;
int line;
int phase_last_line;

int trigraph(int c) {
    if (c == '=')
        return '#';
    if (c == '/')
        return 92;
    if (c == 39)
        return '^';
    if (c == '(')
        return '[';
    if (c == ')')
        return ']';
    if (c == '!')
        return '|';
    if (c == '<')
        return '{';
    if (c == '>')
        return '}';
    if (c == '-')
        return '~';
    return 0;
}

/* Physical newline mapping and trigraph replacement precede splicing.
   Separate forward passes prevent rescanning newly joined character pairs. */
int source_phases(void) {
    int input = 0, output = 0, c, replacement, physical = 1;
    while (input < size) {
        c = source[input];
        input = input + 1;
        if (c == 13) {
            if (input < size && source[input] == 10)
                input = input + 1;
            c = 10;
        } else if (c == '?' && input + 1 < size && source[input] == '?') {
            replacement = trigraph(source[input + 1]);
            if (replacement) {
                c = replacement;
                input = input + 2;
            }
        }
        source[output] = c | (physical << 8);
        output = output + 1;
        if (c == 10)
            physical++;
    }
    size = output;
    input = 0;
    output = 0;
    while (input < size) {
        c = source[input];
        input = input + 1;
        if ((c & 255) == 92 && input < size && (source[input] & 255) == 10)
            input = input + 1;
        else {
            source[output] = c;
            output = output + 1;
        }
    }
    size = output;
    phase_last_line = physical;
    return 0;
}

int seed(int id, int a, int b, int c) {
    int i, word, ch, n = 0;
    for (i = 0; i < 12; i = i + 1) {
        if (i < 4)
            word = a;
        else if (i < 8)
            word = b;
        else
            word = c;
        ch = (word >> ((i & 3) * 8)) & 255;
        if (ch != 0) {
            names[id * 256 + n] = ch;
            n = n + 1;
        }
    }
    lengths[id] = n;
    return 0;
}

int initialize(void) {
    seed(0, 0x746e69, 0, 0);
    seed(1, 0x75746572, 0x6e72, 0);
    seed(2, 0x6669, 0, 0);
    seed(3, 0x65736c65, 0, 0);
    seed(4, 0x6c696877, 0x65, 0);
    seed(5, 0x64696f76, 0, 0);
    seed(6, 0x6e69616d, 0, 0);
    seed(7, 0x61657262, 0x6b, 0);
    seed(8, 0x746e6f63, 0x65756e69, 0);
    seed(9, 0x726f66, 0, 0);
    seed(10, 0x30635f5f, 0x7465675f, 0x72616863);
    seed(11, 0x30635f5f, 0x7475705f, 0x72616863);
    seed(12, 0x6f747561, 0, 0);
    seed(13, 0x65736163, 0, 0);
    seed(14, 0x72616863, 0, 0);
    seed(15, 0x736e6f63, 0x74, 0);
    seed(16, 0x61666564, 0x746c75, 0);
    seed(17, 0x6f64, 0, 0);
    seed(18, 0x62756f64, 0x656c, 0);
    seed(19, 0x6d756e65, 0, 0);
    seed(20, 0x65747865, 0x6e72, 0);
    seed(21, 0x616f6c66, 0x74, 0);
    seed(22, 0x6f746f67, 0, 0);
    seed(23, 0x676e6f6c, 0, 0);
    seed(24, 0x69676572, 0x72657473, 0);
    seed(25, 0x726f6873, 0x74, 0);
    seed(26, 0x6e676973, 0x6465, 0);
    seed(27, 0x657a6973, 0x666f, 0);
    seed(28, 0x74617473, 0x6369, 0);
    seed(29, 0x75727473, 0x7463, 0);
    seed(30, 0x74697773, 0x6863, 0);
    seed(31, 0x65707974, 0x666564, 0);
    seed(32, 0x6f696e75, 0x6e, 0);
    seed(33, 0x69736e75, 0x64656e67, 0);
    seed(34, 0x616c6f76, 0x656c6974, 0);
    seed(35, 0x696c6e69, 0x656e, 0);
    seed(36, 0x74736572, 0x74636972, 0);
    seed(37, 0x6f6f425f, 0x6c, 0);
    seed(38, 0x6d6f435f, 0x78656c70, 0);
    seed(39, 0x616d495f, 0x616e6967, 0x7972);
    seed(40, 0x666f5f5f, 0x74657366, 0x666f);
    seed(41, 0x61765f5f, 0x6174735f, 0x7472);
    seed(42, 0x61765f5f, 0x6772615f, 0);
    seed(43, 0x70665f5f, 0x73746962, 0);
    seed(44, 0x31635f5f, 0x695f, 0);
    seed(45, 0x31635f5f, 0x67745f, 0);
    count = 46;
    line = 1;
    return 0;
}

int peek(void) {
    if (cursor == size)
        return -1;
    return source[cursor];
}

int take(void) {
    int c = peek();
    if (c != -1) {
        cursor = cursor + 1;
        if (c == 10)
            line = line + 1;
    }
    return c;
}

int alpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int digit(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* C99 N1256 Annex D. Source table: annex-d.txt. */
int ucn_letters[470] = {
    0x00aa, 0x00aa, 0x00b5, 0x00b5, 0x00b7, 0x00b7, 0x00ba, 0x00ba, 0x00c0, 0x00d6, 0x00d8, 0x00f6,
    0x00f8, 0x01f5, 0x01fa, 0x0217, 0x0250, 0x02a8, 0x02b0, 0x02b8, 0x02bb, 0x02bb, 0x02bd, 0x02c1,
    0x02d0, 0x02d1, 0x02e0, 0x02e4, 0x037a, 0x037a, 0x0386, 0x0386, 0x0388, 0x038a, 0x038c, 0x038c,
    0x038e, 0x03a1, 0x03a3, 0x03ce, 0x03d0, 0x03d6, 0x03da, 0x03da, 0x03dc, 0x03dc, 0x03de, 0x03de,
    0x03e0, 0x03e0, 0x03e2, 0x03f3, 0x0401, 0x040c, 0x040e, 0x044f, 0x0451, 0x045c, 0x045e, 0x0481,
    0x0490, 0x04c4, 0x04c7, 0x04c8, 0x04cb, 0x04cc, 0x04d0, 0x04eb, 0x04ee, 0x04f5, 0x04f8, 0x04f9,
    0x0531, 0x0556, 0x0559, 0x0559, 0x0561, 0x0587, 0x05b0, 0x05b9, 0x05bb, 0x05bd, 0x05bf, 0x05bf,
    0x05c1, 0x05c2, 0x05d0, 0x05ea, 0x05f0, 0x05f2, 0x0621, 0x063a, 0x0640, 0x0652, 0x0670, 0x06b7,
    0x06ba, 0x06be, 0x06c0, 0x06ce, 0x06d0, 0x06dc, 0x06e5, 0x06e8, 0x06ea, 0x06ed, 0x0901, 0x0903,
    0x0905, 0x0939, 0x093d, 0x093d, 0x093e, 0x094d, 0x0950, 0x0952, 0x0958, 0x0963, 0x0981, 0x0983,
    0x0985, 0x098c, 0x098f, 0x0990, 0x0993, 0x09a8, 0x09aa, 0x09b0, 0x09b2, 0x09b2, 0x09b6, 0x09b9,
    0x09be, 0x09c4, 0x09c7, 0x09c8, 0x09cb, 0x09cd, 0x09dc, 0x09dd, 0x09df, 0x09e3, 0x09f0, 0x09f1,
    0x0a02, 0x0a02, 0x0a05, 0x0a0a, 0x0a0f, 0x0a10, 0x0a13, 0x0a28, 0x0a2a, 0x0a30, 0x0a32, 0x0a33,
    0x0a35, 0x0a36, 0x0a38, 0x0a39, 0x0a3e, 0x0a42, 0x0a47, 0x0a48, 0x0a4b, 0x0a4d, 0x0a59, 0x0a5c,
    0x0a5e, 0x0a5e, 0x0a74, 0x0a74, 0x0a81, 0x0a83, 0x0a85, 0x0a8b, 0x0a8d, 0x0a8d, 0x0a8f, 0x0a91,
    0x0a93, 0x0aa8, 0x0aaa, 0x0ab0, 0x0ab2, 0x0ab3, 0x0ab5, 0x0ab9, 0x0abd, 0x0ac5, 0x0ac7, 0x0ac9,
    0x0acb, 0x0acd, 0x0ad0, 0x0ad0, 0x0ae0, 0x0ae0, 0x0b01, 0x0b03, 0x0b05, 0x0b0c, 0x0b0f, 0x0b10,
    0x0b13, 0x0b28, 0x0b2a, 0x0b30, 0x0b32, 0x0b33, 0x0b36, 0x0b39, 0x0b3d, 0x0b3d, 0x0b3e, 0x0b43,
    0x0b47, 0x0b48, 0x0b4b, 0x0b4d, 0x0b5c, 0x0b5d, 0x0b5f, 0x0b61, 0x0b82, 0x0b83, 0x0b85, 0x0b8a,
    0x0b8e, 0x0b90, 0x0b92, 0x0b95, 0x0b99, 0x0b9a, 0x0b9c, 0x0b9c, 0x0b9e, 0x0b9f, 0x0ba3, 0x0ba4,
    0x0ba8, 0x0baa, 0x0bae, 0x0bb5, 0x0bb7, 0x0bb9, 0x0bbe, 0x0bc2, 0x0bc6, 0x0bc8, 0x0bca, 0x0bcd,
    0x0c01, 0x0c03, 0x0c05, 0x0c0c, 0x0c0e, 0x0c10, 0x0c12, 0x0c28, 0x0c2a, 0x0c33, 0x0c35, 0x0c39,
    0x0c3e, 0x0c44, 0x0c46, 0x0c48, 0x0c4a, 0x0c4d, 0x0c60, 0x0c61, 0x0c82, 0x0c83, 0x0c85, 0x0c8c,
    0x0c8e, 0x0c90, 0x0c92, 0x0ca8, 0x0caa, 0x0cb3, 0x0cb5, 0x0cb9, 0x0cbe, 0x0cc4, 0x0cc6, 0x0cc8,
    0x0cca, 0x0ccd, 0x0cde, 0x0cde, 0x0ce0, 0x0ce1, 0x0d02, 0x0d03, 0x0d05, 0x0d0c, 0x0d0e, 0x0d10,
    0x0d12, 0x0d28, 0x0d2a, 0x0d39, 0x0d3e, 0x0d43, 0x0d46, 0x0d48, 0x0d4a, 0x0d4d, 0x0d60, 0x0d61,
    0x0e01, 0x0e3a, 0x0e40, 0x0e5b, 0x0e81, 0x0e82, 0x0e84, 0x0e84, 0x0e87, 0x0e88, 0x0e8a, 0x0e8a,
    0x0e8d, 0x0e8d, 0x0e94, 0x0e97, 0x0e99, 0x0e9f, 0x0ea1, 0x0ea3, 0x0ea5, 0x0ea5, 0x0ea7, 0x0ea7,
    0x0eaa, 0x0eab, 0x0ead, 0x0eae, 0x0eb0, 0x0eb9, 0x0ebb, 0x0ebd, 0x0ec0, 0x0ec4, 0x0ec6, 0x0ec6,
    0x0ec8, 0x0ecd, 0x0edc, 0x0edd, 0x0f00, 0x0f00, 0x0f18, 0x0f19, 0x0f35, 0x0f35, 0x0f37, 0x0f37,
    0x0f39, 0x0f39, 0x0f3e, 0x0f47, 0x0f49, 0x0f69, 0x0f71, 0x0f84, 0x0f86, 0x0f8b, 0x0f90, 0x0f95,
    0x0f97, 0x0f97, 0x0f99, 0x0fad, 0x0fb1, 0x0fb7, 0x0fb9, 0x0fb9, 0x10a0, 0x10c5, 0x10d0, 0x10f6,
    0x1e00, 0x1e9b, 0x1ea0, 0x1ef9, 0x1f00, 0x1f15, 0x1f18, 0x1f1d, 0x1f20, 0x1f45, 0x1f48, 0x1f4d,
    0x1f50, 0x1f57, 0x1f59, 0x1f59, 0x1f5b, 0x1f5b, 0x1f5d, 0x1f5d, 0x1f5f, 0x1f7d, 0x1f80, 0x1fb4,
    0x1fb6, 0x1fbc, 0x1fbe, 0x1fbe, 0x1fc2, 0x1fc4, 0x1fc6, 0x1fcc, 0x1fd0, 0x1fd3, 0x1fd6, 0x1fdb,
    0x1fe0, 0x1fec, 0x1ff2, 0x1ff4, 0x1ff6, 0x1ffc, 0x203f, 0x2040, 0x207f, 0x207f, 0x2102, 0x2102,
    0x2107, 0x2107, 0x210a, 0x2113, 0x2115, 0x2115, 0x2118, 0x211d, 0x2124, 0x2124, 0x2126, 0x2126,
    0x2128, 0x2128, 0x212a, 0x2131, 0x2133, 0x2138, 0x2160, 0x2182, 0x3005, 0x3007, 0x3021, 0x3029,
    0x3041, 0x3093, 0x309b, 0x309c, 0x30a1, 0x30f6, 0x30fb, 0x30fc, 0x3105, 0x312c, 0x4e00, 0x9fa5,
    0xac00, 0xd7a3};
int ucn_digits[28] = {0x0660, 0x0669, 0x06f0, 0x06f9, 0x0966, 0x096f, 0x09e6,
                      0x09ef, 0x0a66, 0x0a6f, 0x0ae6, 0x0aef, 0x0b66, 0x0b6f,
                      0x0be7, 0x0bef, 0x0c66, 0x0c6f, 0x0ce6, 0x0cef, 0x0d66,
                      0x0d6f, 0x0e50, 0x0e59, 0x0ed0, 0x0ed9, 0x0f20, 0x0f33};

int ucn_in_ranges(unsigned int code, int *ranges, int count) {
    int low = 0, high = count, mid;
    while (low < high) {
        mid = low + (high - low) / 2;
        if (code < (unsigned int)ranges[mid * 2])
            high = mid;
        else if (code > (unsigned int)ranges[mid * 2 + 1])
            low = mid + 1;
        else
            return 1;
    }
    return 0;
}

int ucn_identifier_class(unsigned int code) {
    if (ucn_in_ranges(code, ucn_digits, sizeof(ucn_digits) / sizeof(int) / 2))
        return 2;
    if (ucn_in_ranges(code, ucn_letters, sizeof(ucn_letters) / sizeof(int) / 2))
        return 1;
    return 0;
}

int identifier_ucn(int *text, int end, int *at, int first) {
    unsigned int code = 0;
    int n, i, d, p = *at;
    if (p + 1 >= end || (text[p] & 255) != 92 ||
        ((text[p + 1] & 255) != 'u' && (text[p + 1] & 255) != 'U')) {
        failed = 1;
        return 0;
    }
    n = (text[p + 1] & 255) == 'u' ? 4 : 8;
    if (p + 2 + n > end) {
        failed = 1;
        return 0;
    }
    for (i = 0; i < n; i++) {
        d = digit(text[p + 2 + i] & 255);
        if (d < 0) {
            failed = 1;
            return 0;
        }
        code = code * 16 + (unsigned int)d;
    }
    i = ucn_identifier_class(code);
    if (!i || (first && i == 2)) {
        failed = 1;
        return 0;
    }
    *at = p + 2 + n;
    return (int)code;
}

int intern(void) {
    int start = cursor, n = 0, i, j, equal, c, characters = 0, has_ucn = 0;
    int canonical[256];
    while (alpha(peek()) || (peek() >= '0' && peek() <= '9') || peek() == 92) {
        if (peek() == 92) {
            c = identifier_ucn(source, size, &cursor, characters == 0);
            if (failed)
                return 0;
            has_ucn = 1;
        } else
            c = take();
        if (characters < 63) {
            if (c < 128)
                canonical[n++] = c;
            else if (c < 2048) {
                canonical[n++] = 192 + (c >> 6);
                canonical[n++] = 128 + (c & 63);
            } else {
                canonical[n++] = 224 + (c >> 12);
                canonical[n++] = 128 + ((c >> 6) & 63);
                canonical[n++] = 128 + (c & 63);
            }
        }
        characters++;
    }
    if (has_ucn)
        c89_extension("universal character name in identifier");
    for (i = 0; i < count; i++) {
        if (c89_identifiers && (i == 35 || i == 36))
            continue;
        equal = lengths[i] == n;
        for (j = 0; j < n && equal; j++)
            if (names[i * 256 + j] != canonical[j])
                equal = 0;
        if (equal) {
            spelling_start[i] = start;
            spelling_length[i] = cursor - start;
            return 256 + i;
        }
    }
    if (count == 2048) {
        diagnostic_reason = "identifier capacity exceeded";
        failed = 1;
        return 0;
    }
    for (j = 0; j < n; j++)
        names[count * 256 + j] = canonical[j];
    lengths[count] = n;
    spelling_start[count] = start;
    spelling_length[count] = cursor - start;
    count++;
    return 255 + count;
}

int word_bits(int low, int high) {
    int n = (high & 32767) * 65536 + low;
    if (high >= 32768)
        n = (-2147483647 - 1) + n;
    return n;
}

int floating_number(void) {
    int start = cursor, end = cursor, c, previous = 0, hex = 0, found = 0, status;
    if (start + 1 < size && source[start] == '0' &&
        (source[start + 1] == 'x' || source[start + 1] == 'X'))
        hex = 1;
    while (end < size) {
        c = source[end];
        if (!((c >= '0' && c <= '9') || alpha(c) || c == '.' ||
              ((c == '+' || c == '-') &&
               (previous == 'e' || previous == 'E' || previous == 'p' || previous == 'P'))))
            break;
        if (c == '.' || (hex ? (c == 'p' || c == 'P') : (c == 'e' || c == 'E')))
            found = 1;
        previous = c;
        end++;
    }
    if (!found)
        return 0;
    if (pp_condition) {
        failed = 1;
        return 144;
    }
    status = fp_parse(&source[start], end - start);
    if (status) {
        failed = 1;
        return 144;
    }
    if (hex)
        c89_extension("hexadecimal floating constant");
    cursor = end;
    literal_type = fp_type;
    value = (int)fp_bits;
    literal_high = (int)(fp_bits >> 32);
    return 144;
}

int number(void) {
    int base = 10;
    int d, n = 0, low = 0, high = 0, upper = 0, top = 0, u = 0, l = 0, c;
    if (floating_number())
        return 144;
    value = 0;
    if (peek() == '0') {
        base = 8;
        if (cursor + 1 < size && (source[cursor + 1] == 'x' || source[cursor + 1] == 'X')) {
            base = 16;
            take();
            take();
        }
    }
    while (digit(peek()) >= 0) {
        d = digit(take());
        if (d >= base) {
            failed = 1;
            return 0;
        }
        low = low * base + d;
        high = high * base + low / 65536;
        low = low & 65535;
        upper = upper * base + high / 65536;
        high = high & 65535;
        top = top * base + upper / 65536;
        upper = upper & 65535;
        if (top > 65535) {
            failed = 1;
            return 0;
        }
        n = n + 1;
    }
    if (peek() == 'u' || peek() == 'U') {
        take();
        u = 1;
    }
    if (peek() == 'l' || peek() == 'L') {
        c = take();
        l = 1;
        if (peek() == c) {
            take();
            l = 2;
        }
    }
    if (!u && (peek() == 'u' || peek() == 'U')) {
        take();
        u = 1;
    }
    if (n == 0 || alpha(peek())) {
        failed = 1;
        return 0;
    }
    if (l == 2)
        c89_extension("long long integer suffix");
    else if (!pp_condition && (upper || top))
        c89_extension("integer constant exceeds C89 unsigned long range");
    value = word_bits(low, high);
    literal_high = word_bits(upper, top);
    if (pp_condition) {
        literal_type = (u || ((c89_identifiers || base != 10) && top >= 32768)) ? 11 : 10;
        if ((!c89_identifiers || l == 2) && !u && base == 10 && top >= 32768) {
            failed = 1;
            return 0;
        }
        return 144;
    }
    if (l == 2 || top || upper) {
        literal_type = 10;
        if (u || top >= 32768)
            literal_type = 11;
        if (!u && base == 10 && top >= 32768) {
            failed = 1;
            return 0;
        }
        return 144;
    }
    literal_type = 1;
    if (l)
        literal_type = 3;
    if (u)
        literal_type = literal_type + 1;
    if (high >= 32768 && !u) {
        if (c99_mode && base == 10)
            literal_type = 10;
        else if (l || base == 10)
            literal_type = 4;
        else
            literal_type = 2;
    }
    return 144;
}

/* The execution repertoires retain byte / 16-bit unit values. UCNs outside
   the chosen repertoire translate to '?' rather than truncating their code. */
int universal_escape(int count, int limit) {
    unsigned int code = 0;
    int i, d;
    c89_extension("universal character name");
    for (i = 0; i < count; i++) {
        d = digit(peek());
        if (d < 0) {
            failed = 1;
            return 0;
        }
        take();
        code = code * 16 + (unsigned int)d;
    }
    if ((code < 160 && code != 36 && code != 64 && code != 96) ||
        (code >= 55296 && code <= 57343)) {
        failed = 1;
        return 0;
    }
    if (code > (unsigned int)limit)
        return 63;
    return (int)code;
}

int escape(int limit) {
    int c = take(), n, escape_count = 0;
    if (c == 'u' || c == 'U')
        return universal_escape(c == 'u' ? 4 : 8, limit);
    if (c == 'a')
        return 7;
    if (c == 'b')
        return 8;
    if (c == 'f')
        return 12;
    if (c == 'n')
        return 10;
    if (c == 'r')
        return 13;
    if (c == 't')
        return 9;
    if (c == 'v')
        return 11;
    if (c == 92 || c == 39 || c == 34 || c == '?')
        return c;
    if (c >= '0' && c <= '7') {
        n = c - '0';
        escape_count = 1;
        while (escape_count < 3 && peek() >= '0' && peek() <= '7') {
            n = n * 8 + take() - '0';
            escape_count = escape_count + 1;
        }
        if (n > limit) {
            failed = 1;
            return 0;
        }
        return n;
    }
    if (c == 'x') {
        n = 0;
        while (digit(peek()) >= 0) {
            n = n * 16 + digit(take());
            escape_count = escape_count + 1;
            if (n > limit) {
                failed = 1;
                return 0;
            }
        }
        if (!escape_count)
            failed = 1;
        return n;
    }
    failed = 1;
    return 0;
}

int string_token(int wide) {
    int c;
    literal_string_wide = wide;
    literal_string_start = string_used;
    while (!failed) {
        c = take();
        if (c == 34)
            break;
        if (c == -1 || c == 10 || c == 13) {
            failed = 1;
            return 0;
        }
        if (c == 92)
            c = escape(wide ? 65535 : 255);
        if (string_used == 1048576) {
            failed = 1;
            return 0;
        }
        string_pool[string_used] = c;
        string_used = string_used + 1;
    }
    literal_string_length = string_used - literal_string_start;
    return 145;
}

int character(int wide) {
    int c, count = 0;
    unsigned int bits = 0;
    while (!failed) {
        c = take();
        if (c == 39)
            break;
        if (c == -1 || c == 10 || c == 13) {
            failed = 1;
            return 0;
        }
        if (c == 92)
            c = escape(wide ? 65535 : 255);
        /* Defined implementation choice: narrow constants pack bytes from
           left to right modulo 32 bits; wide constants use the last unit. */
        if (wide)
            bits = (unsigned int)c;
        else
            bits = (bits << 8) | (unsigned int)c;
        count = count + 1;
    }
    if (!count || failed) {
        failed = 1;
        return 0;
    }
    value = (int)bits;
    if (!wide && count == 1 && bits >= 128)
        value = (int)bits - 256;
    literal_type = wide ? 8 : 1;
    literal_high = !wide && value < 0 ? -1 : 0;
    return 144;
}

int lex(void) {
    int c, d, previous;
    while (1) {
        c = peek();
        if (c == -1)
            return 0;
        if (c == 32 || (c >= 9 && c <= 13)) {
            take();
            continue;
        }
        if (c < 32) {
            failed = 1;
            return 0;
        }
        if (c == 'L' && cursor + 1 < size &&
            (source[cursor + 1] == 34 || source[cursor + 1] == 39)) {
            take();
            c = take();
            return c == 34 ? string_token(1) : character(1);
        }
        if (alpha(c) || c == 92)
            return intern();
        if (c >= '0' && c <= '9')
            return number();
        take();
        d = peek();
        if (!c89_identifiers && c == '/' && d == '/') {
            while (peek() != -1 && peek() != 10)
                take();
            continue;
        }
        if (c == '/' && d == '*') {
            take();
            previous = 0;
            while (1) {
                d = take();
                if (d == -1) {
                    failed = 1;
                    return 0;
                }
                if (previous == '*' && d == '/')
                    break;
                previous = d;
            }
            continue;
        }
        if (c == '.' && d >= '0' && d <= '9') {
            cursor--;
            return floating_number();
        }
        if (c == 39)
            return character(0);
        if (c == 34)
            return string_token(0);
        if (c == '<' && d == ':') {
            take();
            return '[';
        }
        if (c == ':' && d == '>') {
            take();
            return ']';
        }
        if (c == '<' && d == '%') {
            take();
            return '{';
        }
        if (c == '%' && d == '>') {
            take();
            return '}';
        }
        if (c == '%' && d == ':') {
            take();
            if (cursor + 1 < size && source[cursor] == '%' && source[cursor + 1] == ':') {
                take();
                take();
                return 172;
            }
            return '#';
        }
        if (c == '+' && d == '+') {
            take();
            return 170;
        }
        if (c == '-' && d == '-') {
            take();
            return 171;
        }
        if (c == '-' && d == '>') {
            take();
            return 173;
        }
        if (c == '.' && d == '.' && cursor + 1 < size && source[cursor + 1] == '.') {
            take();
            take();
            return 174;
        }
        if (c == '=' && d == '=') {
            take();
            return 128;
        }
        if (c == '!' && d == '=') {
            take();
            return 129;
        }
        if (c == '<' && d == '=') {
            take();
            return 130;
        }
        if (c == '>' && d == '=') {
            take();
            return 131;
        }
        if (c == '<' && d == '<') {
            take();
            if (peek() == '=') {
                take();
                return 168;
            }
            return 132;
        }
        if (c == '>' && d == '>') {
            take();
            if (peek() == '=') {
                take();
                return 169;
            }
            return 133;
        }
        if (c == '&' && d == '&') {
            take();
            return 134;
        }
        if (c == '|' && d == '|') {
            take();
            return 135;
        }
        if (d == '=') {
            if (c == '+') {
                take();
                return 160;
            }
            if (c == '-') {
                take();
                return 161;
            }
            if (c == '*') {
                take();
                return 162;
            }
            if (c == '/') {
                take();
                return 163;
            }
            if (c == '%') {
                take();
                return 164;
            }
            if (c == '&') {
                take();
                return 165;
            }
            if (c == '|') {
                take();
                return 166;
            }
            if (c == '^') {
                take();
                return 167;
            }
        }
        return c;
    }
}

/* Machine code and PE image are generated here, inside the C compiler. */
int image[1049088];
int position;
int token;
int label_address[2048];
int label_head[2048];
int label_vm[2048];
int vm_current, vm_count, vm_goto_count;
int vm_parent[4096], vm_binding[4096], vm_stack[4096];
int vm_goto_label[4096], vm_goto_from[4096], vm_goto_code[4096];
int local_offset[2048];
int local_depth[2048];
int binding_name[256];
int binding_offset[256];
int binding_depth[256];
int local_type[2048];
int binding_type[256];
int binding_bytes[256];
int alias_type[2048];
int binding_alias[256];
int decl_typedef;
int decl_storage;
int decl_inline;
int inline_seen[2048];
int inline_external[2048];
int inline_body = -1;
int inline_invalid[2048];
int register_object[2048];
int binding_register[256];
int local_static_address[2048];
int binding_static_address[256];
int unevaluated_depth;
int for_declaration;
int local_bytes;
int frame_peak;
int frame_patch;
int initializer_depth;
int initializer_designated;
int compound_static_type;
int initializer_pending;
int initializer_probe;
int inferred_elements;
int initializer_start;
int initializer_tail;
int initializer_call;
int array_base[2048];
int array_length[2048];
int array_bytes[2048];
int array_count;
int decl_kind[256];
int decl_next[256];
int decl_length[256];
int decl_count;
int decl_name;
int decl_result_name;
int declarator_top_function;
int prototype_depth;
int prototype_star_seen;
int definition_star_seen;
int parameter_replay_base[128], parameter_replay_token[128];
int parameter_replay_cursor[128], parameter_replay_line[128], parameter_replay_storage[128];
int parameter_replay_valid[128];
int definition_ready;
int definition_old;
int definition_parameter_count;
int definition_parameter_name[128];
int definition_parameter_type[128];
int function_type_count;
int function_result[2048];
int function_arity[2048];
int function_parameter[131072];
int decl_depth;
int decl_parentheses;
int declarator_active;
int global_type[2048];
int file_function_type[2048];
int parameter_type[262144];
int expression_type;
int sizeof_value;
int evaluated_sizeof;
int expression_zero;
/* 0: not an integer constant expression; 1: valid; 2: invalid if evaluated. */
int expression_ice;
unsigned long long expression_value;
unsigned long long ce_normalize(unsigned long long x, int t);
int expression_load;
int expression_record_ready;
int expression_bit;
int expression_array_bytes;
int expression_array_type;
int global_extent[2048];
int return_type;
int global_address[2048];
int global_initialized[2048];
int global_tentative[2048];
int object_ref_position[16384];
int object_ref_symbol[16384];
int object_ref_count;
int object_mode;
int address_ref_count;
int address_ref_position[16384];
int address_ref_target[16384];
int symbol_flags[2048];
int global_visible[2048];
int entity_type[2048];
int enum_count;
int enum_complete[2048];
int enum_global[2048];
int enum_global_value[2048];
int tag_type[2048];
int tag_depth[2048];
int tag_count;
int tag_binding_name[2048];
int tag_binding_type[2048];
int tag_binding_depth[2048];
int parameter_count;
int parameter_name[128];
int current_variadic;
int current_oldstyle;
int current_va_offset;
int parameter_offset[128];
int parameter_decl_type[128];
int return_address_offset;
int record_count;
int record_depth;
int record_size[2048];
int record_align[2048];
int record_kind[2048];
int record_open[2048];
int record_first[2048];
int record_last[2048];
int member_count;
int member_name[8192];
int member_type[8192];
int member_offset[8192];
int member_next[8192];
int member_bits[8192];
int member_shift[8192];
int expression_register;
int arity[2048];
int call_arity[16384];
int function_address[2048];
int call_position[16384];
int call_symbol[16384];
int call_count;
int allocation;
int local_count;
int block_depth;
int expression_depth;
int unary_depth;
int statement_depth;
int break_head;
int break_local_count;
int continue_local_count;
int continue_target;
int loop_depth;
int switch_depth;
int switch_vm;
int switch_type;
int switch_head;
int switch_default;
int case_count;
int case_next[4096];
int case_address[4096];
unsigned long long case_value[4096];

typedef long long DiagnosticHandle(int);
typedef int DiagnosticWrite(long long, char *, int, int *, void *);
char diagnostic_buffer[4096];
int diagnostic_used;
int diagnostic_sent;

int diagnostic_broken;

int diagnostic_flush(void) {
    DiagnosticHandle *get = (DiagnosticHandle *)__c1_win_GetStdHandle;
    DiagnosticWrite *write = (DiagnosticWrite *)__c1_win_WriteFile;
    long long handle;
    int offset = 0, done, ok;
    if (!diagnostic_used || diagnostic_broken)
        return 0;
    handle = get(-12);
    while (offset < diagnostic_used) {
        done = 0;
        ok = write(handle, diagnostic_buffer + offset, diagnostic_used - offset, &done, 0);
        if (!ok || done <= 0 || done > diagnostic_used - offset) {
            diagnostic_broken = 1;
            break;
        }
        offset += done;
    }
    diagnostic_used = 0;
    return 0;
}

int diagnostic_put(int c) {
    if (diagnostic_broken)
        return 0;
    diagnostic_buffer[diagnostic_used++] = (char)c;
    if (diagnostic_used == 4096)
        diagnostic_flush();
    return 0;
}

int diagnostic_text(char *text) {
    int i = 0;
    while (text[i])
        diagnostic_put(text[i++]);
    return 0;
}

int diagnostic_number(unsigned int number) {
    char digits[10];
    int n = 0;
    do {
        digits[n++] = (char)('0' + number % 10);
        number /= 10;
    } while (number);
    while (n)
        diagnostic_put(digits[--n]);
    return 0;
}

int diagnostic_end(void) {
    diagnostic_put(10);
    diagnostic_flush();
    diagnostic_sent = 1;
    return 0;
}

int c89_extension(char *message) {
    if (c89_identifiers) {
        diagnostic_text("C1 warning: C99 extension in C89 mode: ");
        diagnostic_text(message);
        diagnostic_put(10);
        diagnostic_flush();
    }
    return 0;
}

int error(void) {
    failed = 1;
    token = 0;
    return 0;
}

int error_message(char *message) {
    if (!failed)
        diagnostic_reason = message;
    return error();
}

int emit(int b) {
    if (failed)
        return 0;
    if (position >= 1049088)
        return error();
    image[position] = b & 255;
    position = position + 1;
    return 0;
}

int sequence(int word, int n) {
    while (n > 0 && !failed) {
        emit(word);
        word = word >> 8;
        n = n - 1;
    }
    return 0;
}

int emit32(int n) {
    return sequence(n, 4);
}

int write32(int p, int n) {
    int i;
    if (p < 0 || p > 1049084)
        return error();
    for (i = 0; i < 4; i = i + 1) {
        image[p + i] = n & 255;
        n = n >> 8;
    }
    return 0;
}

int read32(int p) {
    return image[p] | (image[p + 1] << 8) | (image[p + 2] << 16) | (image[p + 3] << 24);
}

int relative(int address) {
    if (object_mode) {
        if (address_ref_count == 16384)
            return error_message("object relocation capacity exceeded");
        address_ref_position[address_ref_count] = position;
        address_ref_target[address_ref_count++] = address;
    }
    return emit32(address - (position + 0x400e00 + 4));
}

int indirect(int address) {
    sequence(0x15ff, 2);
    return relative(address);
}

int jump(int condition) {
    int p;
    if (condition == 0)
        emit(0xe9);
    else {
        emit(0x0f);
        emit(condition);
    }
    p = position;
    emit32(0);
    return p;
}

int patch(int p, int target) {
    return write32(p, target - p - 4);
}

int call(int id) {
    if (call_count == 16384)
        return error();
    emit(0xe8);
    call_position[call_count] = position;
    call_symbol[call_count] = id;
    call_count = call_count + 1;
    return emit32(0);
}

int function_exists(int id) {
    return local_offset[id] == -4 ||
           (!local_offset[id] && !alias_type[id] && file_function_type[id]);
}

int function_signature(int id) {
    if (local_offset[id] == -4)
        return local_type[id];
    return file_function_type[id];
}

int function_entity_signature(int id) {
    return make_function(global_type[id], (symbol_flags[id] & 4) ? arity[id] : -1,
                         &parameter_type[id * 128]);
}

int function_reference(int id) {
    if (call_count == 16384)
        return error();
    call_position[call_count] = position;
    call_symbol[call_count] = id;
    call_arity[call_count] = -1;
    call_count++;
    return emit32(0);
}

int function_value(int t) {
    sequence(0xc28948, 3);
    expression_load = position;
    expression_type = make_pointer(t);
    expression_array_type = t;
    expression_array_bytes = 0;
    expression_zero = 0;
    expression_ice = 0;
    expression_register = 0;
    return -1;
}

int invoke(int id, int target, int n) {
    if (id >= 0) {
        call(id);
        call_arity[call_count - 1] = n;
    } else {
        sequence(0x858b48, 3);
        emit32(-target);
        sequence(0xd0ff, 2);
    }
    return 0;
}

int next(void) {
    if (failed) {
        token = 0;
        return 0;
    }
    token = lex();
    if (pp_condition && token == 144 && literal_type != 11)
        literal_type = 10;
    if (failed)
        token = 0;
    return token;
}

int expect(int wanted) {
    if (token != wanted)
        return error();
    next();
    return 0;
}

int name(void) {
    if (token == 262 || token == 266 || token == 267 || token >= 302)
        return token - 256;
    return error();
}

/* Labels use their own function-wide namespace. Peek without consuming the
   expression's leading identifier or retaining speculative string bytes. */
int label_ahead(void) {
    int saved_cursor = cursor, saved_line = line, saved_strings = string_used, found;
    if (!(token == 262 || token == 266 || token == 267 || token >= 302))
        return 0;
    found = lex() == ':';
    cursor = saved_cursor;
    line = saved_line;
    string_used = saved_strings;
    return found;
}

int prologue(void) {
    int start, tail, branch;
    emit(0x55);
    sequence(0xe58948, 3);
    emit(0xb8);
    frame_patch = position;
    emit32(0);
    start = position;
    emit(0x3d);
    emit32(4096);
    tail = jump(0x82);
    sequence(0xec8148, 3);
    emit32(4096);
    sequence(0x240485, 3);
    emit(0x2d);
    emit32(4096);
    branch = jump(0);
    patch(branch, start);
    patch(tail, position);
    sequence(0xc42948, 3);
    sequence(0x240485, 3);
    return 0;
}

int epilogue(void) {
    return sequence(0xc3c9, 2);
}

/* Types: void=0, int=1, unsigned int=2, long=3, unsigned long=4,
   signed char=5, unsigned char=6, short=7, unsigned short=8, char=9.
   The Windows data model uses 32-bit int/long and signed plain char. */
int qualifier(int t) {
    if (t == 271)
        return 1;
    if (t == 290)
        return 2;
    if (t == 292)
        return 4;
    return 0;
}

int type_start(int t) {
    return t == 294 || t == 291 || qualifier(t) || t == 274 || t == 277 || t == 256 || t == 261 ||
           t == 270 || t == 279 || t == 281 || t == 282 || t == 289 || t == 293 || t == 287 ||
           t == 268 || t == 280 || t == 284 || t == 276 || t == 275 || t == 285 || t == 288 ||
           (t >= 256 && alias_type[t - 256]);
}

/* Complex layout uses the existing two-component aggregate storage machinery.
   Record kinds 13..15 retain distinct real component types, not user tags. */
int complex_type(int base) {
    int i;
    for (i = 0; i < record_count; i++)
        if (record_kind[i] == base)
            return 0x20000e + i * 256;
    if (record_count == 1024)
        return error();
    i = record_count++;
    record_kind[i] = base;
    record_size[i] = base == 13 ? 8 : 16;
    record_align[i] = base == 13 ? 4 : 8;
    record_first[i] = 0;
    record_last[i] = 0;
    record_open[i] = 0;
    return 0x20000e + i * 256;
}

int complex_component(int t) {
    int k;
    if (!is_record(t))
        return 0;
    k = record_kind[(unqualified(t) - 0x20000e) / 256];
    return k >= 13 && k <= 15 ? k : 0;
}

int specifier_qualifiers;

int specifier_base(void) {
    int i = 0, ch = 0, sh = 0, lo = 0, si = 0, un = 0, vo = 0, bo = 0, fl = 0, db = 0, cx = 0,
        n = 0, t, alias = 0, saved_storage, saved_inline;
    decl_typedef = 0;
    decl_storage = 0;
    decl_inline = 0;
    while (type_start(token) && !failed) {
        if (token == 291) {
            decl_inline = 1;
            next();
            continue;
        }
        if (qualifier(token)) {
            if (c89_identifiers && (specifier_qualifiers & qualifier(token)))
                return error_message("duplicate C89 type qualifier");
            specifier_qualifiers = specifier_qualifiers | qualifier(token);
            next();
            continue;
        }
        if (token == 287 || token == 268 || token == 280 || token == 284 || token == 276) {
            if (decl_storage)
                return error();
            if (token == 287) {
                decl_storage = 1;
                decl_typedef = 1;
            }
            if (token == 268)
                decl_storage = 2;
            if (token == 280)
                decl_storage = 3;
            if (token == 284)
                decl_storage = 4;
            if (token == 276)
                decl_storage = 5;
            next();
            continue;
        }
        if (token == 275 || token == 285 || token == 288) {
            if (n)
                return error();
            saved_storage = decl_storage;
            saved_inline = decl_inline;
            if (token == 275)
                alias = enum_specifier() + 1;
            else
                alias = record_specifier() + 1;
            decl_storage = saved_storage;
            decl_inline = saved_inline;
            decl_typedef = decl_storage == 1;
            n++;
            continue;
        }
        if (token >= 256 && alias_type[token - 256]) {
            if (n)
                break;
            alias = alias_type[token - 256];
            next();
            n++;
            continue;
        }
        t = token;
        next();
        n = n + 1;
        if (t == 256)
            i = i + 1;
        if (t == 261)
            vo = vo + 1;
        if (t == 270)
            ch = ch + 1;
        if (t == 279)
            lo = lo + 1;
        if (t == 281)
            sh = sh + 1;
        if (t == 282)
            si = si + 1;
        if (t == 289)
            un = un + 1;
        if (t == 293)
            bo = bo + 1;
        if (t == 277)
            fl++;
        if (t == 274)
            db++;
        if (t == 294)
            cx++;
    }
    if (alias) {
        if (n != 1)
            return error();
        return alias - 1;
    }
    if (bo) {
        if (n != 1)
            return error();
        return 12;
    }
    if (cx) {
        if (cx != 1 || i || ch || sh || si || un || vo || bo || (fl && db))
            return error_message("invalid complex type specifiers");
        if (fl == 1 && n == 2) {
            c89_extension("complex type");
            return complex_type(13);
        }
        if (db == 1 && lo <= 1 && n == 2 + lo) {
            c89_extension("complex type");
            return complex_type(14 + lo);
        }
        return error_message("complex requires float, double or long double");
    }
    if (fl || db) {
        if (fl == 1 && n == 1)
            return 13;
        if (db == 1 && lo <= 1 && n == db + lo)
            return 14 + lo;
        return error_message("invalid floating type specifiers");
    }
    if (!n && (decl_storage || specifier_qualifiers))
        return 1;
    if (!n || i > 1 || ch > 1 || sh > 1 || lo > 2 || si > 1 || un > 1 || vo > 1)
        return error();
    if ((si && un) || (sh && lo) || (ch && (i || sh || lo)))
        return error();
    if (vo) {
        if (n != 1)
            return error();
        return 0;
    }
    if (ch) {
        if (un)
            return 6;
        if (si)
            return 5;
        return 9;
    }
    if (sh)
        return 7 + un;
    if (lo == 2) {
        c89_extension("long long type");
        return 10 + un;
    }
    if (lo)
        return 3 + un;
    return 1 + un;
}

int qualified_count;
int qualified_base[4096];
int qualified_flags[4096];

int type_qualifiers(int t) {
    if (t >= 0x400000 && t % 256 == 12)
        return qualified_flags[(t - 0x40000c) / 256];
    return 0;
}

int unqualified(int t) {
    if (t >= 0x400000 && t % 256 == 12)
        return qualified_base[(t - 0x40000c) / 256];
    return t;
}

int specifiers(void) {
    int saved = specifier_qualifiers, t, q, element;
    specifier_qualifiers = 0;
    t = specifier_base();
    q = specifier_qualifiers;
    specifier_qualifiers = saved;
    if (c89_identifiers && q) {
        element = t;
        while (is_array(element))
            element = array_base[(unqualified(element) - 4096) / 256];
        if (type_qualifiers(element) & q)
            return error_message("duplicate C89 typedef qualifier");
    }
    return qualify_type(t, q);
}

int is_array(int t) {
    t = unqualified(t);
    return t >= 4096 && t % 256 == 0;
}

int is_enum(int t) {
    t = unqualified(t);
    return t >= 0x100000 && t % 256 == 13;
}

int is_record(int t) {
    t = unqualified(t);
    return t >= 0x200000 && t % 256 == 14;
}

int is_function(int t) {
    t = unqualified(t);
    return t >= 0x300000 && t % 256 == 15;
}

int pointed_type(int t) {
    return unqualified(t) - 16;
}

int function_pointer(int t) {
    return is_pointer(t) && is_function(pointed_type(t));
}

int bitfield_node(int t) {
    t = unqualified(t);
    return t >= 0x500000 && t % 256 == 10 ? (t - 0x50000a) / 256 : 0;
}

int bit_value_type(int t) {
    int node = bitfield_node(t);
    return node ? unqualified(member_type[node]) : unqualified(t);
}

int is_pointer(int t) {
    t = unqualified(t);
    return t >= 16 && !is_array(t) && !is_enum(t) && !is_record(t) && !is_function(t) &&
           !bitfield_node(t);
}

int floating_type(int t) {
    t = unqualified(t);
    return t >= 13 && t <= 15;
}

int wide_type(int t) {
    t = unqualified(t);
    return t == 10 || t == 11 || t == 14 || t == 15 || is_pointer(t);
}

int default_promoted(int t) {
    return unqualified(t) == 13 ? 14 : promoted(t);
}

int type_size(int t) {
    t = bit_value_type(t);
    if (is_function(t))
        return error();
    if (is_record(t)) {
        t = record_size[(unqualified(t) - 0x20000e) / 256];
        if (!t)
            return error();
        return t;
    }
    if (is_enum(t)) {
        if (!enum_complete[(t - 0x10000d) / 256])
            return error();
        return 4;
    }
    if (is_array(t)) {
        t = array_bytes[(t - 4096) / 256];
        if (t <= 0)
            return error_message("array size is not statically known");
        return t;
    }
    if (wide_type(t))
        return 8;
    if (t == 5 || t == 6 || t == 9 || t == 12)
        return 1;
    if (t == 7 || t == 8)
        return 2;
    if ((t >= 1 && t <= 4) || t == 13)
        return 4;
    return error();
}

int type_align(int t) {
    t = unqualified(t);
    while (is_array(t))
        t = array_base[(t - 4096) / 256];
    if (is_record(t)) {
        type_size(t);
        return record_align[(unqualified(t) - 0x20000e) / 256];
    }
    return type_size(t);
}

int pointers(int t) {
    while (token == '*' && !failed) {
        next();
        t = t + 16;
        if (t > 265)
            return error();
    }
    return t;
}

int make_pointer(int t) {
    if (t >= 4096 && t % 256 >= 240)
        return error();
    if (t < 4096 && t >= 240)
        return error();
    return t + 16;
}

/* Type-only transformations retain captured extents without emitting code. */
int rebase_array(int t, int base) {
    int old = (unqualified(t) - 4096) / 256, i;
    for (i = 0; i < array_count; i++)
        if (array_base[i] == base && array_length[i] == array_length[old] &&
            array_bytes[i] == array_bytes[old])
            return 4096 + i * 256;
    if (array_count == 1024)
        return error();
    i = array_count++;
    array_base[i] = base;
    array_length[i] = array_length[old];
    array_bytes[i] = array_bytes[old];
    return 4096 + i * 256;
}

int qualify_type(int t, int q) {
    int i;
    if (!q || failed)
        return t;
    q = q | type_qualifiers(t);
    t = unqualified(t);
    if (is_array(t))
        return rebase_array(t, qualify_type(array_base[(t - 4096) / 256], q));
    if (is_function(t))
        return error_message("function type cannot be qualified");
    if ((q & 4) && (!is_pointer(t) || is_function(pointed_type(t))))
        return error_message("restrict requires pointer to object or incomplete type");
    for (i = 0; i < qualified_count; i++) {
        if (qualified_base[i] == t && qualified_flags[i] == q)
            return 0x40000c + i * 256;
    }
    if (qualified_count == 2048)
        return error_message("qualified type capacity exceeded");
    i = qualified_count++;
    qualified_base[i] = t;
    qualified_flags[i] = q;
    return 0x40000c + i * 256;
}

int contains_const(int t) {
    int node;
    if (type_qualifiers(t) & 1)
        return 1;
    if (is_array(t))
        return contains_const(array_base[(t - 4096) / 256]);
    if (is_record(t)) {
        for (node = record_first[(unqualified(t) - 0x20000e) / 256]; node;
             node = member_next[node]) {
            if (contains_const(member_type[node]))
                return 1;
        }
    }
    return 0;
}

int contains_volatile(int t) {
    int node;
    if (type_qualifiers(t) & 2)
        return 1;
    if (is_array(t))
        return contains_volatile(array_base[(t - 4096) / 256]);
    if (is_record(t)) {
        for (node = record_first[(unqualified(t) - 0x20000e) / 256]; node; node = member_next[node])
            if (contains_volatile(member_type[node]))
                return 1;
    }
    return 0;
}

int incomplete_array(int t) {
    return is_array(t) && !array_length[(t - 4096) / 256];
}

int record_has_flexible(int t) {
    int index, node;
    if (!is_record(t))
        return 0;
    index = (unqualified(t) - 0x20000e) / 256;
    if (record_kind[index] == 1) {
        node = record_last[index];
        return node && incomplete_array(member_type[node]);
    }
    for (node = record_first[index]; node; node = member_next[node]) {
        if (record_has_flexible(member_type[node]))
            return 1;
    }
    return 0;
}

/* Values below -1 encode a frame slot as -(offset+1). */
int extent_slot(void) {
    int saved = unevaluated_depth, slot;
    unevaluated_depth = 0;
    slot = temporary(8, 8);
    unevaluated_depth = saved;
    return slot;
}

int emit_extent(int value) {
    if (value < -1) {
        sequence(0x858b48, 3);
        emit32(value + 1);
    } else
        constant(value);
    expression_type = 11;
    expression_ice = 0;
    expression_zero = 0;
    return 0;
}

int runtime_array_bound(void) {
    int start = position, calls = call_count, t, slot, saved = unevaluated_depth;
    unsigned long long value;
    unevaluated_depth = 0;
    expression(1);
    unevaluated_depth = saved;
    t = unqualified(expression_type);
    value = expression_value;
    if (failed)
        return 0;
    if (!t || floating_type(t) || is_pointer(t) || is_record(t) || is_array(t) || is_function(t))
        return error_message("array bound requires integer type");
    if (expression_ice == 1) {
        if (!value || value > 0x1a00000ULL)
            return error_message("invalid constant array bound");
        rewind_code(start);
        call_count = calls;
        return (int)value;
    }
    c89_extension("runtime variable array bound");
    if (!wide_type(t) && !unsigned_type(promoted(t)))
        sequence(0xc06348, 3);
    slot = extent_slot();
    store(slot, 11);
    return -slot - 1;
}

int make_array(int base, int n) {
    int i, bytes, slot;
    /* -1 bytes denotes a complete prototype array with unknown size. */
    if (is_array(base) && array_bytes[(unqualified(base) - 4096) / 256] < 0)
        bytes = array_bytes[(unqualified(base) - 4096) / 256];
    else
        bytes = type_size(base);
    if (failed)
        return 0;
    if (record_has_flexible(base))
        return error_message("array element contains flexible array member");
    if (bytes > 0 && n > 0x1a00000 / bytes)
        return error();
    if (n && (n < -1 || bytes < -1)) {
        if (n == -1 || bytes == -1)
            return error_message("incomplete runtime array extent");
        emit_extent(n);
        emit(0x50);
        emit_extent(bytes);
        emit(0x59);
        sequence(0xc1af0f48, 4);
        slot = extent_slot();
        store(slot, 11);
        if (array_count == 1024)
            return error();
        i = array_count++;
        array_base[i] = base;
        array_length[i] = n;
        array_bytes[i] = -slot - 1;
        return 4096 + i * 256;
    }
    for (i = 0; i < array_count; i = i + 1) {
        if (array_base[i] == base && array_length[i] == n)
            return 4096 + i * 256;
    }
    if (array_count == 1024)
        return error();
    array_base[array_count] = base;
    array_length[array_count] = n;
    array_bytes[array_count] = n == 0 ? 0 : (n < 0 || bytes < 0 ? -1 : n * bytes);
    array_count = array_count + 1;
    return 4096 + (array_count - 1) * 256;
}

/* A variadic signature stores 128 plus its fixed parameter count. */
int fixed_arity(int n) {
    return n >= 128 ? n - 128 : n;
}

int arity_matches(int expected, int actual) {
    return expected >= 128 ? actual >= expected - 128 : expected == actual;
}

int make_function(int result, int n, int *parameters) {
    int i, j, same;
    if (is_array(result) || is_function(result))
        return error();
    for (i = 0; i < function_type_count; i++) {
        same = function_result[i] == result && function_arity[i] == n;
        for (j = 0; j < fixed_arity(n) && same; j++)
            if (function_parameter[i * 128 + j] != unqualified(parameters[j]))
                same = 0;
        if (same)
            return 0x30000f + i * 256;
    }
    if (function_type_count == 1024)
        return error();
    i = function_type_count++;
    function_result[i] = result;
    function_arity[i] = n;
    for (j = 0; j < fixed_arity(n); j++)
        function_parameter[i * 128 + j] = unqualified(parameters[j]);
    return 0x30000f + i * 256;
}

int variably_modified_type(int t) {
    t = unqualified(t);
    if (is_array(t)) {
        if (array_length[(t - 4096) / 256] < 0)
            return 1;
        return variably_modified_type(array_base[(t - 4096) / 256]);
    }
    if (is_pointer(t))
        return variably_modified_type(pointed_type(t));
    if (is_function(t))
        return variably_modified_type(function_result[(t - 0x30000f) / 256]);
    return 0;
}

/* Prototype bounds are checked, but produce no runtime evaluation. */
int prototype_array_bound(void) {
    int start = position, calls = call_count, data = allocation;
    int bytes = local_bytes, peak = frame_peak, t, kind;
    unsigned long long value;
    unevaluated_depth++;
    expression(1);
    t = unqualified(expression_type);
    kind = expression_ice;
    value = expression_value;
    unevaluated_depth--;
    allocation = data;
    rewind_code(start);
    call_count = calls;
    local_bytes = bytes;
    frame_peak = peak;
    if (failed)
        return 0;
    if (!t || floating_type(t) || is_pointer(t) || is_record(t) || is_array(t) || is_function(t))
        return error_message("array bound requires integer type");
    if (kind == 1) {
        if (!value || value > 0x1a00000ULL)
            return error_message("invalid constant array bound");
        return (int)value;
    }
    /* Failure to be an ICE does not invalidate an assignment expression. */
    c89_extension("nonconstant prototype array bound");
    prototype_star_seen |= 2;
    return -1;
}

int save_parameter_declarator(int n, int base) {
    if (n < 0 || n >= 128)
        return error();
    parameter_replay_valid[n] = 1;
    parameter_replay_base[n] = base;
    parameter_replay_token[n] = token;
    parameter_replay_cursor[n] = cursor;
    parameter_replay_line[n] = line;
    parameter_replay_storage[n] = decl_storage;
    return 0;
}

int evaluate_parameter_declarators(void) {
    int i, t, saved_cursor = cursor, saved_line = line, saved_token = token;
    int saved_storage = decl_storage, saved_alias = decl_typedef, saved_inline = decl_inline;
    for (i = 0; i < parameter_count && !failed; i++) {
        if (!parameter_replay_valid[i])
            continue;
        cursor = parameter_replay_cursor[i];
        line = parameter_replay_line[i];
        token = parameter_replay_token[i];
        decl_storage = parameter_replay_storage[i];
        decl_typedef = 0;
        decl_inline = 0;
        t = declarator(parameter_replay_base[i], 2);
        if (is_array(t))
            t = make_pointer(array_base[(unqualified(t) - 4096) / 256]);
        if (is_function(t))
            t = make_pointer(t);
        if (decl_name != parameter_name[i])
            return error_message("parameter replay name mismatch");
        parameter_decl_type[i] = t;
        local_type[parameter_name[i]] = t;
    }
    cursor = saved_cursor;
    line = saved_line;
    token = saved_token;
    decl_storage = saved_storage;
    decl_typedef = saved_alias;
    decl_inline = saved_inline;
    return 0;
}

int function_suffix_ahead(void) {
    int saved_cursor = cursor, saved_line = line, saved_strings = string_used, t;
    t = lex();
    cursor = saved_cursor;
    line = saved_line;
    string_used = saved_strings;
    return t == ')' || type_start(t);
}

int function_suffix(int capture) {
    int saved_locals = local_count, saved_tags = tag_count, saved_bytes = local_bytes;
    int saved_peak = frame_peak, saved_block = block_depth, saved_storage = decl_storage,
        saved_alias = decl_typedef, saved_inline = decl_inline;
    int parameters[128], n = 0, t, id, storage, result, old = 0, variadic = 0;
    int saved_star = prototype_star_seen;
    prototype_depth++;
    prototype_star_seen = 0;
    block_depth++;
    expect('(');
    if (token == ')')
        n = -1;
    if (capture && token != ')' && !type_start(token)) {
        old = 1;
        while (!failed) {
            if (n == 127)
                return error();
            id = name();
            next();
            definition_parameter_name[n] = id;
            definition_parameter_type[n] = 1;
            n++;
            local(11, id);
            local_type[id] = 1;
            if (token != ',')
                break;
            next();
            if (token == ')')
                return error();
        }
    } else
        while (!failed && token != ')') {
            t = specifiers();
            storage = decl_storage;
            if (!t && !n && token == ')' && !storage)
                break;
            if (decl_inline || (storage && storage != 3))
                return error();
            if (capture)
                save_parameter_declarator(n, t);
            t = declarator(t, 2);
            id = decl_result_name;
            if (is_array(t))
                t = make_pointer(array_base[(t - 4096) / 256]);
            if (is_function(t))
                t = make_pointer(t);
            if (!unqualified(t) || n == 127)
                return error();
            if (capture) {
                definition_parameter_name[n] = id;
                definition_parameter_type[n] = t;
            }
            parameters[n++] = t;
            if (id >= 0) {
                local(11, id);
                local_type[id] = t;
                register_object[id] = storage == 3;
            }
            if (token != ',')
                break;
            next();
            if (token == ')')
                return error();
            if (token == 174) {
                variadic = 1;
                next();
                break;
            }
        }
    expect(')');
    result = make_function(1, old ? -1 : n + variadic * 128, parameters);
    if (capture) {
        definition_star_seen = prototype_star_seen;
        definition_ready = 1;
        definition_old = old;
        definition_parameter_count = n < 0 ? 0 : n;
    } else {
        restore_locals(saved_locals);
        restore_tags(saved_tags);
        local_bytes = saved_bytes;
        frame_peak = saved_peak;
        block_depth = saved_block;
    }
    prototype_depth--;
    prototype_star_seen = saved_star;
    decl_storage = saved_storage;
    decl_inline = saved_inline;
    decl_typedef = saved_alias;
    return result;
}

int decl_node(int kind, int n) {
    int id;
    if (decl_count == 255)
        return error();
    decl_count = decl_count + 1;
    id = decl_count;
    decl_kind[id] = kind;
    decl_length[id] = n;
    decl_next[id] = 0;
    return id;
}

/* Constructors are linked from the identifier outwards. Parentheses keep
   their inner chain before following suffixes; prefix stars come last. */
int declarator_chain(int abstract) {
    int stars = 0, head = 0, tail = 0, node, n, qualifiers[256], array_qualifiers, minimum_bound;
    decl_depth = decl_depth + 1;
    if (decl_depth > 64)
        return error();
    while (token == '*' && !failed) {
        if (stars == 255)
            return error();
        next();
        n = 0;
        while (qualifier(token)) {
            if (c89_identifiers && (n & qualifier(token)))
                return error_message("duplicate C89 pointer qualifier");
            n = n | qualifier(token);
            next();
        }
        qualifiers[stars++] = n;
    }
    if (token == '(' && (!abstract || !function_suffix_ahead())) {
        decl_parentheses = 1;
        next();
        head = declarator_chain(abstract);
        expect(')');
        tail = head;
        while (tail && decl_next[tail])
            tail = decl_next[tail];
    } else if (token == 262 || token == 266 || token == 267 || token >= 302) {
        if (abstract == 1 || decl_name != -1)
            return error();
        decl_name = name();
        next();
    } else if (!abstract)
        return error();
    while (!failed && (token == '[' || token == '(')) {
        if (token == '(') {
            if (declarator_top_function && declarator_active == 1 && decl_depth == 1 && !head)
                break;
            n = function_suffix(declarator_top_function && declarator_active == 1 && !head &&
                                !definition_ready);
            node = decl_node(3, n);
        } else {
            next();
            n = 0;
            array_qualifiers = 0;
            minimum_bound = 0;
            if (token == 284) {
                minimum_bound = 1;
                next();
                while (qualifier(token)) {
                    array_qualifiers |= qualifier(token);
                    next();
                }
            } else {
                while (qualifier(token)) {
                    array_qualifiers |= qualifier(token);
                    next();
                }
                if (token == 284) {
                    minimum_bound = 1;
                    next();
                }
            }
            if ((array_qualifiers || minimum_bound) && (abstract != 2 || head))
                return error_message("array qualifiers/static require outermost parameter array");
            if (minimum_bound && token == ']')
                return error_message("static array parameter requires a bound");
            if (array_qualifiers || minimum_bound)
                c89_extension("array parameter qualifiers or static bound");
            if (token == '*') {
                if (!prototype_depth || minimum_bound)
                    return error_message("star array requires function prototype scope");
                c89_extension("prototype star array");
                prototype_star_seen |= 1;
                n = -1;
                next();
            } else if (token != ']')
                n = prototype_depth ? prototype_array_bound()
                                    : (block_depth ? runtime_array_bound() : constant_bound());
            expect(']');
            node = decl_node(array_qualifiers || minimum_bound ? 16 + array_qualifiers : 2, n);
        }
        if (tail)
            decl_next[tail] = node;
        else
            head = node;
        tail = node;
    }
    while (stars && !failed) {
        stars--;
        node = decl_node(1, qualifiers[stars]);
        if (tail)
            decl_next[tail] = node;
        else
            head = node;
        tail = node;
    }
    decl_depth = decl_depth - 1;
    return head;
}

int apply_declarator(int node, int base) {
    int t;
    if (!node || failed)
        return base;
    t = apply_declarator(decl_next[node], base);
    if (decl_kind[node] >= 16) {
        make_array(t, decl_length[node]);
        return qualify_type(make_pointer(t), decl_kind[node] - 16);
    }
    if (decl_kind[node] == 1)
        return qualify_type(make_pointer(t), decl_length[node]);
    if (decl_kind[node] == 3) {
        node = (decl_length[node] - 0x30000f) / 256;
        return make_function(t, function_arity[node], &function_parameter[node * 128]);
    }
    return make_array(t, decl_length[node]);
}

int declarator(int base, int abstract) {
    int head, result, i, saved_count = decl_count, saved_name = decl_name;
    int saved_depth = decl_depth, saved_parentheses = decl_parentheses;
    int saved_kind[256], saved_next[256], saved_length[256];
    if (abstract && (decl_typedef || decl_inline))
        return error();
    if ((abstract == 1 && decl_storage) ||
        (abstract == 2 && (decl_storage == 2 || decl_storage >= 4)))
        return error();
    declarator_active++;
    if (declarator_active > 64)
        return error();
    if (declarator_active > 1) {
        for (i = 1; i <= saved_count; i++) {
            saved_kind[i] = decl_kind[i];
            saved_next[i] = decl_next[i];
            saved_length[i] = decl_length[i];
        }
    }
    decl_count = 0;
    decl_name = -1;
    decl_depth = 0;
    decl_parentheses = 0;
    head = declarator_chain(abstract);
    for (i = head; i; i = decl_next[i])
        if (decl_kind[i] >= 16 && i != head)
            return error_message("array qualifiers/static require outermost parameter array");
    result = apply_declarator(head, base);
    decl_result_name = decl_name;
    if (declarator_active > 1) {
        for (i = 1; i <= saved_count; i++) {
            decl_kind[i] = saved_kind[i];
            decl_next[i] = saved_next[i];
            decl_length[i] = saved_length[i];
        }
        decl_count = saved_count;
        decl_name = saved_name;
        decl_depth = saved_depth;
        decl_parentheses = saved_parentheses;
    }
    declarator_active--;
    return result;
}

int promoted(int t) {
    int node = bitfield_node(t);
    if (node)
        return member_bits[node] < 32 ? 1 : bit_value_type(t);
    t = unqualified(t);
    if ((t >= 5 && t <= 9) || t == 12 || is_enum(t))
        return 1;
    return t;
}

int unsigned_type(int t) {
    t = bit_value_type(t);
    return t == 2 || t == 4 || t == 11;
}

int void_pointer(int t) {
    return is_pointer(t) && !unqualified(pointed_type(t));
}

int pointer_common(int a, int b, int allow_void) {
    int q, base;
    a = pointed_type(a);
    b = pointed_type(b);
    q = type_qualifiers(a) | type_qualifiers(b);
    a = unqualified(a);
    b = unqualified(b);
    if (allow_void && (!a || !b)) {
        if (is_function(a) || is_function(b))
            return error();
        base = 0;
    } else
        base = composite_type(a, b);
    return make_pointer(qualify_type(base, q));
}

int common_type(int a, int b) {
    a = unqualified(a);
    b = unqualified(b);
    if ((void_pointer(a) && function_pointer(b)) || (void_pointer(b) && function_pointer(a)))
        return error();
    if (complex_component(a) || complex_component(b))
        return complex_common(a, b);
    if (is_record(a) || is_record(b)) {
        if (a == b)
            return a;
        return error();
    }
    if (is_pointer(a) || is_pointer(b)) {
        if (a == b)
            return a;
        if (is_pointer(a) && is_pointer(b))
            return pointer_common(a, b, 1);
        return error();
    }
    if (floating_type(a) || floating_type(b)) {
        if (!a || !b)
            return error();
        if (a == 15 || b == 15)
            return 15;
        if (a == 14 || b == 14)
            return 14;
        return 13;
    }
    a = promoted(a);
    b = promoted(b);
    if (!a || !b)
        return error();
    if (a == 11 || b == 11)
        return 11;
    if (a == 10 || b == 10)
        return 10;
    if (a == 4 || b == 4 || (a + b == 5))
        return 4;
    if (a == 3 || b == 3)
        return 3;
    if (a == 2 || b == 2)
        return 2;
    return 1;
}

/* Aggregate-valued expressions carry their storage address in RAX. */
int complex_part(int slot, int component, int imaginary) {
    load_opcode(11, 0x85);
    emit32(-slot);
    if (imaginary) {
        sequence(0xc08348, 3);
        emit(type_size(component));
    }
    return load_opcode(component, 0);
}

int complex_truth(void) {
    int component = complex_component(expression_type), slot;
    record_read();
    slot = temporary(8, 8);
    store(slot, 11);
    complex_part(slot, component, 0);
    floating_absolute_bits(component);
    emit(0x50);
    complex_part(slot, component, 1);
    floating_absolute_bits(component);
    emit(0x59);
    sequence(0xc80948, 3);
    expression_type = 11;
    expression_zero = 0;
    expression_ice = 0;
    return 0;
}

int complex_convert(int t) {
    int from = complex_component(expression_type), to = complex_component(t), slot = 0, result;
    if (!t) {
        expression_type = 0;
        expression_zero = 0;
        return ice_set(0, 0);
    }
    if (is_pointer(t) || is_pointer(expression_type) || is_array(t) || is_function(t) ||
        !expression_type)
        return error_message("invalid complex conversion");
    if ((is_record(t) && !to) || (is_record(expression_type) && !from))
        return error();
    if (t == 12) {
        complex_truth();
        boolean(0x95);
        expression_type = 12;
        return 0;
    }
    if (from) {
        slot = temporary(8, 8);
        store(slot, 11);
        complex_part(slot, from, 0);
    }
    if (!to)
        return convert(t);
    result = temporary(type_size(t), type_align(t));
    convert(to);
    store(result, to);
    if (from) {
        complex_part(slot, from, 1);
        convert(to);
    } else {
        constant(0);
        convert(to);
    }
    store(result - type_size(to), to);
    sequence(0x858d48, 3);
    emit32(-result);
    return record_value(unqualified(t));
}

int convert(int t) {
    int a, b;
    record_read();
    t = bit_value_type(t);
    expression_type = bit_value_type(expression_type);
    expression_bit = 0;
    if (complex_component(t) || complex_component(expression_type))
        return complex_convert(t);
    if (is_function(t) || is_function(expression_type))
        return error();
    if ((void_pointer(t) && function_pointer(expression_type)) ||
        (void_pointer(expression_type) && function_pointer(t) && !expression_zero))
        return error();
    if (is_record(t)) {
        type_size(t);
        if (expression_type != t)
            return error();
        return 0;
    }
    if (t && is_record(expression_type))
        return error();
    if (is_enum(t))
        type_size(t);
    if ((floating_type(t) && is_pointer(expression_type)) ||
        (is_pointer(t) && floating_type(expression_type)))
        return error();
    if (is_array(t))
        return error();
    if (t && !expression_type)
        return error();
    if (t == 12) {
        a = expression_ice;
        boolean(0x95);
        expression_ice = a;
        expression_type = 12;
        ice_convert(t);
        return 0;
    }
    if (is_pointer(t)) {
        if (is_pointer(expression_type)) {
            if (function_pointer(t) && void_pointer(expression_type) && expression_zero) {
                expression_type = t;
                expression_ice = 0;
                return 0;
            }
            a = pointed_type(t);
            b = pointed_type(expression_type);
            if ((type_qualifiers(a) & type_qualifiers(b)) != type_qualifiers(b))
                return error_message("pointer conversion discards qualifier");
            pointer_common(t, expression_type, 1);
        } else if (!expression_zero)
            return error();
        expression_type = t;
        expression_ice = 0;
        return 0;
    }
    if (t && is_pointer(expression_type))
        return error();
    if (floating_type(t) || floating_type(expression_type))
        return floating_convert(t);
    if (wide_type(t) && !wide_type(expression_type)) {
        if (unsigned_type(promoted(expression_type)))
            sequence(0xc089, 2);
        else
            sequence(0xc06348, 3);
    }
    if (t && !wide_type(t) && wide_type(expression_type))
        sequence(0xc089, 2);
    if (t == 5 || t == 9)
        sequence(0xc0be0f, 3);
    if (t == 6)
        sequence(0xc0b60f, 3);
    if (t == 7)
        sequence(0xc0bf0f, 3);
    if (t == 8)
        sequence(0xc0b70f, 3);
    expression_type = t;
    ice_convert(t);
    return 0;
}

int cast_to(int t) {
    int literal, from;
    unsigned long long bits;
    record_read();
    t = bit_value_type(t);
    expression_type = bit_value_type(expression_type);
    expression_bit = 0;
    literal = expression_ice == 4;
    from = unqualified(expression_type);
    bits = expression_value;
    if (is_function(t))
        return error();
    if (complex_component(t) || complex_component(expression_type))
        return complex_convert(t);
    if (is_record(t) || (t && is_record(expression_type)))
        return error();
    if (t && !expression_type)
        return error();
    if ((floating_type(t) && is_pointer(expression_type)) ||
        (is_pointer(t) && floating_type(expression_type)))
        return error();
    if (is_pointer(t)) {
        if (!wide_type(expression_type) && !unsigned_type(promoted(expression_type)))
            sequence(0xc06348, 3);
        expression_zero = expression_ice == 1 && !expression_value && t == 16;
        expression_ice = 0;
        expression_type = t;
        expression_ice = 0;
        return 0;
    }
    if (is_pointer(expression_type)) {
        expression_type = 11;
        expression_zero = 0;
        expression_ice = 0;
    }
    convert(t);
    if (literal && t && !floating_type(t)) {
        if (fpe_convert(from, is_enum(t) ? 1 : unqualified(t), bits))
            ice_set(2, 0);
        else
            ice_set(1, fpe_bits);
    }
    if (!t) {
        expression_zero = 0;
        expression_ice = 0;
    }
    return 0;
}

int constant(int n) {
    expression_bit = 0;
    expression_type = 1;
    expression_ice = 1;
    expression_value = (long long)n;
    expression_zero = n == 0;
    emit(0xb8);
    return emit32(n);
}

int load_opcode(int t, int modrm) {
    int declared = t;
    t = unqualified(t);
    expression_array_bytes = 0;
    expression_array_type = 0;
    expression_load = position;
    expression_zero = 0;
    expression_ice = 0;
    if (wide_type(t)) {
        emit(0x48);
        emit(0x8b);
    } else if ((t >= 5 && t <= 9) || t == 12) {
        emit(0x0f);
        if (t == 5 || t == 9)
            emit(0xbe);
        if (t == 6 || t == 12)
            emit(0xb6);
        if (t == 7)
            emit(0xbf);
        if (t == 8)
            emit(0xb7);
    } else
        emit(0x8b);
    emit(modrm);
    expression_type = declared;
    return 0;
}

int array_value(int t) {
    expression_array_type = t;
    expression_array_bytes = array_bytes[(t - 4096) / 256];
    expression_type = make_pointer(array_base[(t - 4096) / 256]);
    expression_zero = 0;
    expression_ice = 0;
    return 0;
}

int indirect_value(int t) {
    if (is_function(t))
        return function_value(t);
    expression_register = 0;
    sequence(0xc28948, 3);
    if (is_record(t))
        record_value(t);
    else if (is_array(t)) {
        expression_load = position;
        array_value(t);
    } else
        load_opcode(t, 0);
    return -1;
}

int object_address(int id) {
    if (local_offset[id] > 0 || local_offset[id] == -3 || alias_type[id])
        return 0;
    if (local_offset[id] == -1)
        return local_static_address[id];
    return global_address[id];
}

int object_type(int id) {
    if (local_offset[id] < 0)
        return local_type[id];
    return global_type[id];
}

int object_exists(int id) {
    if (local_offset[id] > 0 || local_offset[id] == -3 || alias_type[id])
        return 0;
    return local_offset[id] == -1 || local_offset[id] == -2 || global_visible[id];
}

int object_reference(int id) {
    if (object_address(id) && (!object_mode || local_offset[id] == -1))
        return relative(object_address(id));
    if (object_ref_count == 16384)
        return error();
    object_ref_position[object_ref_count] = position;
    object_ref_symbol[object_ref_count] = id;
    object_ref_count++;
    return emit32(0);
}

int rewind_code(int target) {
    position = target;
    while (address_ref_count && address_ref_position[address_ref_count - 1] >= target)
        address_ref_count--;
    while (object_ref_count && object_ref_position[object_ref_count - 1] >= target)
        object_ref_count--;
    return 0;
}

int load(int id) {
    int start = position;
    expression_register = local_offset[id] > 0 && register_object[id];
    if (local_offset[id] > 0) {
        if (is_record(local_type[id])) {
            sequence(0x858d48, 3);
            emit32(-local_offset[id]);
            record_value(local_type[id]);
            expression_load = start;
            return local_offset[id];
        }
        if (is_array(local_type[id])) {
            if (register_object[id] && !unevaluated_depth && !evaluated_sizeof)
                return error();
            if (runtime_array_type(local_type[id])) {
                sequence(0x858b48, 3);
                emit32(-local_offset[id]);
                return indirect_value(local_type[id]);
            }
            expression_load = position;
            sequence(0x858d48, 3);
            emit32(-local_offset[id]);
            array_value(local_type[id]);
            return local_offset[id];
        }
        load_opcode(local_type[id], 0x85);
        emit32(-local_offset[id]);
        return local_offset[id];
    }
    if (object_exists(id)) {
        if (is_record(object_type(id))) {
            sequence(0x058d48, 3);
            object_reference(id);
            record_value(object_type(id));
            expression_load = start;
            return -id - 2;
        }
        if (is_array(object_type(id))) {
            expression_load = position;
            sequence(0x058d48, 3);
            object_reference(id);
            array_value(object_type(id));
            return -id - 2;
        }
        load_opcode(object_type(id), 5);
        object_reference(id);
        return -id - 2;
    }
    return error();
}

int store(int lvalue, int t) {
    int n = type_size(t), opcode = 0x89;
    if (is_record(t))
        return store_record(lvalue, t);
    if (n == 1)
        opcode = 0x88;
    if (lvalue == -1)
        emit(0x59);
    if (n == 8)
        emit(0x48);
    if (n == 2)
        emit(0x66);
    emit(opcode);
    if (lvalue == -1)
        return emit(1);
    if (lvalue < -1) {
        emit(5);
        return object_reference(-lvalue - 2);
    }
    emit(0x85);
    return emit32(-lvalue);
}

int floating_to_xmm(int t, int reg, int gpr) {
    emit(0x66);
    if (unqualified(t) != 13)
        emit(0x48);
    sequence(0x6e0f, 2);
    return emit(0xc0 + reg * 8 + gpr);
}

int floating_from_xmm(int t, int reg) {
    emit(0x66);
    if (unqualified(t) != 13)
        emit(0x48);
    sequence(0x7e0f, 2);
    return emit(0xc0 + reg * 8);
}

int floating_instruction(int t, int opcode, int modrm) {
    emit(unqualified(t) == 13 ? 0xf3 : 0xf2);
    emit(0x0f);
    emit(opcode);
    return emit(modrm);
}

int floating_absolute_bits(int t) {
    if (unqualified(t) == 13) {
        emit(0x25);
        emit32(0x7fffffff);
    } else {
        sequence(0xf0ba0f48, 4);
        emit(63);
    }
    return 0;
}

int floating_negate(int t) {
    if (unqualified(t) == 13) {
        emit(0x35);
        emit32(0x80000000);
    } else {
        sequence(0xf8ba0f48, 4);
        emit(63);
    }
    return 0;
}

int floating_int_instruction(int t, int opcode, int wide) {
    emit(unqualified(t) == 13 ? 0xf3 : 0xf2);
    if (wide)
        emit(0x48);
    emit(0x0f);
    emit(opcode);
    return emit(0xc0);
}

int floating_convert(int t) {
    int from = bit_value_type(expression_type), branch, end, wide;
    t = unqualified(t);
    if (!t) {
        expression_type = 0;
        return ice_set(0, 0);
    }
    if (floating_type(from) && floating_type(t)) {
        if ((from == 13) != (t == 13)) {
            floating_to_xmm(from, 0, 0);
            floating_instruction(from, 0x5a, 0xc0);
            floating_from_xmm(t, 0);
        }
    } else if (floating_type(t)) {
        if (is_pointer(from) || is_record(from) || !from)
            return error();
        wide = wide_type(from) || unsigned_type(promoted(from));
        if (unsigned_type(promoted(from)) && !wide_type(from))
            sequence(0xc089, 2);
        if (from == 11) {
            sequence(0xc08548, 3);
            branch = jump(0x89);
            sequence(0xc28948, 3);
            sequence(0x01e283, 3);
            sequence(0xe8d148, 3);
            sequence(0xd00948, 3);
            floating_int_instruction(t, 0x2a, 1);
            floating_instruction(t, 0x58, 0xc0);
            end = jump(0);
            patch(branch, position);
            floating_int_instruction(t, 0x2a, 1);
            patch(end, position);
        } else
            floating_int_instruction(t, 0x2a, wide);
        floating_from_xmm(t, 0);
    } else {
        if (is_pointer(t) || is_record(t))
            return error();
        floating_to_xmm(from, 0, 0);
        if (t == 11) {
            if (from == 13) {
                emit(0xba);
                emit32(0x5f000000);
            } else {
                sequence(0xba48, 2);
                emit32(0);
                emit32(0x43e00000);
            }
            floating_to_xmm(from, 1, 2);
            if (from != 13)
                emit(0x66);
            sequence(0xc12e0f, 3);
            branch = jump(0x82);
            floating_instruction(from, 0x5c, 0xc1);
            floating_int_instruction(from, 0x2c, 1);
            sequence(0xe8ba0f48, 4);
            emit(63);
            end = jump(0);
            patch(branch, position);
            floating_int_instruction(from, 0x2c, 1);
            patch(end, position);
        } else {
            wide = wide_type(t) || unsigned_type(t);
            floating_int_instruction(from, 0x2c, wide);
            if (!wide_type(t) && wide)
                sequence(0xc089, 2);
            if (t == 5 || t == 9)
                sequence(0xc0be0f, 3);
            if (t == 6)
                sequence(0xc0b60f, 3);
            if (t == 7)
                sequence(0xc0bf0f, 3);
            if (t == 8)
                sequence(0xc0b70f, 3);
        }
    }
    expression_type = t;
    return ice_set(0, 0);
}

int floating_arguments(int *types, int n) {
    int i, t, gpr;
    for (i = 0; i < n && i < 4; i++) {
        t = unqualified(types[i]);
        if (!floating_type(t))
            continue;
        emit(0x66);
        if (t != 13)
            emit(i >= 2 ? 0x49 : 0x48);
        else if (i >= 2)
            emit(0x41);
        gpr = i == 0 ? 1 : (i == 1 ? 2 : i - 2);
        sequence(0x6e0f, 2);
        emit(0xc0 + i * 8 + gpr);
    }
    return 0;
}

int floating_binary(int op, int left_type, int right_type) {
    int t, opcode = 0, cc = 0;
    if (is_pointer(left_type) || is_pointer(right_type) || !left_type || !right_type)
        return error();
    if (op == '+')
        opcode = 0x58;
    if (op == '-')
        opcode = 0x5c;
    if (op == '*')
        opcode = 0x59;
    if (op == '/')
        opcode = 0x5e;
    if (op == 128)
        cc = 0x94;
    if (op == 129)
        cc = 0x95;
    if (op == '<')
        cc = 0x92;
    if (op == '>')
        cc = 0x97;
    if (op == 130)
        cc = 0x96;
    if (op == 131)
        cc = 0x93;
    if (!opcode && !cc)
        return error_message("operator requires integer operands");
    t = common_type(left_type, right_type);
    expression_type = right_type;
    convert(t);
    sequence(0xc18948, 3);
    emit(0x58);
    expression_type = left_type;
    convert(t);
    floating_to_xmm(t, 0, 0);
    floating_to_xmm(t, 1, 1);
    if (opcode) {
        floating_instruction(t, opcode, 0xc1);
        floating_from_xmm(t, 0);
        expression_type = t;
    } else {
        if (t != 13)
            emit(0x66);
        sequence(0xc12e0f, 3);
        emit(0x0f);
        emit(cc);
        emit(0xc0);
        emit(0x0f);
        emit(op == 129 ? 0x9a : 0x9b);
        emit(0xc2);
        if (op == 129)
            sequence(0xd008, 2);
        else
            sequence(0xd020, 2);
        sequence(0xc0b60f, 3);
        expression_type = 1;
    }
    return ice_set(0, 0);
}

int boolean(int cc) {
    if (complex_component(expression_type))
        complex_truth();
    if (!expression_type || is_record(expression_type))
        return error();
    if (floating_type(expression_type))
        floating_absolute_bits(expression_type);
    if (wide_type(expression_type))
        emit(0x48);
    expression_type = 1;
    expression_zero = 0;
    expression_ice = 0;
    sequence(0xc085, 2);
    emit(0x0f);
    emit(cc);
    emit(0xc0);
    return sequence(0xc0b60f, 3);
}

int comparison(int cc) {
    if (wide_type(expression_type))
        emit(0x48);
    expression_type = 1;
    sequence(0xc839, 2);
    emit(0x0f);
    emit(cc);
    emit(0xc0);
    return sequence(0xc0b60f, 3);
}

int precedence(int op) {
    if (op == '=' || (op >= 160 && op <= 169))
        return 1;
    if (op == '?')
        return 2;
    if (op == 135)
        return 3;
    if (op == 134)
        return 4;
    if (op == '|')
        return 5;
    if (op == '^')
        return 6;
    if (op == '&')
        return 7;
    if (op == 128 || op == 129)
        return 8;
    if (op == '<' || op == '>' || op == 130 || op == 131)
        return 9;
    if (op == 132 || op == 133)
        return 10;
    if (op == '+' || op == '-')
        return 11;
    if (op == '*' || op == '/' || op == '%')
        return 12;
    return 0;
}

int compound(int op) {
    if (op == 160)
        return '+';
    if (op == 161)
        return '-';
    if (op == 162)
        return '*';
    if (op == 163)
        return '/';
    if (op == 164)
        return '%';
    if (op == 165)
        return '&';
    if (op == 166)
        return '|';
    if (op == 167)
        return '^';
    if (op == 168)
        return 132;
    if (op == 169)
        return 133;
    return 0;
}

int pointer_stride(int t) {
    t = unqualified(pointed_type(t));
    if (is_array(t) && array_bytes[(t - 4096) / 256] < -1)
        return array_bytes[(t - 4096) / 256];
    return type_size(t);
}

int pointer_binary(int op, int a, int b, int left_zero) {
    int n, cc = 0;
    a = unqualified(a);
    b = unqualified(b);
    if (op == '+' || op == '-') {
        if (op == '-' && (is_pointer(a) && a != 16) && (is_pointer(b) && b != 16)) {
            pointer_common(a, b, 0);
            pointer_stride(b);
            n = pointer_stride(a);
            sequence(0xc18948, 3);
            emit(0x58);
            sequence(0xc82948, 3);
            sequence(0x9948, 2);
            if (n < -1) {
                sequence(0x8d8b48, 3);
                emit32(n + 1);
            } else {
                emit(0xb9);
                emit32(n);
            }
            sequence(0xf9f748, 3);
            expression_type = 10;
        } else if (is_pointer(a) && b > 0 && !is_pointer(b)) {
            n = pointer_stride(a);
            if (!wide_type(b) && !unsigned_type(promoted(b)))
                sequence(0xc06348, 3);
            sequence(0xc18948, 3);
            emit(0x58);
            if (n < -1) {
                sequence(0x8daf0f48, 4);
                emit32(n + 1);
            } else {
                sequence(0xc96948, 3);
                emit32(n);
            }
            if (op == '+')
                sequence(0xc80148, 3);
            else
                sequence(0xc82948, 3);
            expression_type = a;
        } else if (op == '+' && is_pointer(b) && a > 0 && !is_pointer(a)) {
            n = pointer_stride(b);
            sequence(0xc18948, 3);
            emit(0x58);
            if (!wide_type(a) && !unsigned_type(promoted(a)))
                sequence(0xc06348, 3);
            if (n < -1) {
                sequence(0x85af0f48, 4);
                emit32(n + 1);
            } else {
                sequence(0xc06948, 3);
                emit32(n);
            }
            sequence(0xc80148, 3);
            expression_type = b;
        } else
            return error();
    } else {
        if (!is_pointer(a) && !left_zero)
            return error();
        if (!is_pointer(b) && !expression_zero)
            return error();
        if ((void_pointer(a) && function_pointer(b) && !left_zero) ||
            (void_pointer(b) && function_pointer(a) && !expression_zero))
            return error();
        if (is_pointer(a) && is_pointer(b) &&
            !((void_pointer(a) && left_zero && function_pointer(b)) ||
              (void_pointer(b) && expression_zero && function_pointer(a))))
            pointer_common(a, b, 1);
        if (op == 128)
            cc = 0x94;
        if (op == 129)
            cc = 0x95;
        if (op == '<')
            cc = 0x92;
        if (op == '>')
            cc = 0x97;
        if (op == 130)
            cc = 0x96;
        if (op == 131)
            cc = 0x93;
        if (!cc)
            return error();
        if (op != 128 && op != 129 && (!is_pointer(a) || !is_pointer(b)))
            return error();
        if (op != 128 && op != 129) {
            if (function_pointer(a) || function_pointer(b))
                return error();
            pointer_common(a, b, 0);
        }
        sequence(0xc18948, 3);
        emit(0x58);
        sequence(0xc83948, 3);
        emit(0x0f);
        emit(cc);
        emit(0xc0);
        sequence(0xc0b60f, 3);
        expression_type = 1;
    }
    expression_zero = 0;
    expression_ice = 0;
    return 0;
}

int complex_common(int a, int b) {
    int x = complex_component(a), y = complex_component(b);
    if (x)
        a = x;
    if (y)
        b = y;
    if (is_pointer(a) || is_pointer(b) || is_record(a) || is_record(b) || !a || !b)
        return error();
    return complex_type(common_type(a, b));
}

int complex_unary(int op) {
    int t = unqualified(expression_type), part = complex_component(t), source, result, i;
    record_read();
    source = temporary(8, 8);
    store(source, 11);
    result = temporary(type_size(t), type_align(t));
    for (i = 0; i < 2; i++) {
        complex_part(source, part, i);
        if (op == '-')
            floating_negate(part);
        store(result - i * type_size(part), part);
    }
    sequence(0x858d48, 3);
    emit32(-result);
    return record_value(t);
}

int complex_product(int left, int right, int part, int li, int ri) {
    complex_part(left, part, li);
    emit(0x50);
    complex_part(right, part, ri);
    return floating_binary('*', part, part);
}

int complex_slot_op(int op, int x, int y, int part, int dest) {
    load_opcode(part, 0x85);
    emit32(-x);
    emit(0x50);
    load_opcode(part, 0x85);
    emit32(-y);
    floating_binary(op, part, part);
    return store(dest, part);
}

int complex_real_bits(int part, int low, int high) {
    if (part != 13) {
        sequence(0xb848, 2);
        emit32(low);
        emit32(high);
    } else {
        emit(0xb8);
        emit32(low);
    }
    expression_type = part;
    expression_zero = 0;
    return ice_set(0, 0);
}

int complex_classify_slot(int slot, int part) {
    /* Unsigned comparison of magnitude bits with infinity: equal is
       infinity, above is NaN. This classification raises no exceptions. */
    load_opcode(part, 0x85);
    emit32(-slot);
    floating_absolute_bits(part);
    if (part == 13) {
        emit(0x3d);
        emit32(0x7f800000);
    } else {
        sequence(0xb948, 2);
        emit32(0);
        emit32(0x7ff00000);
        sequence(0xc83948, 3);
    }
    return 0;
}

int complex_signed_box(int slot, int part, int one) {
    load_opcode(part, 0x85);
    emit32(-slot);
    if (part == 13) {
        emit(0x25);
        emit32(0x80000000);
        if (one) {
            emit(0x0d);
            emit32(0x3f800000);
        }
    } else {
        sequence(0xb948, 2);
        emit32(0);
        emit32(0x80000000);
        sequence(0xc82148, 3);
        if (one) {
            sequence(0xb948, 2);
            emit32(0);
            emit32(0x3ff00000);
            sequence(0xc80948, 3);
        }
    }
    return store(slot, part);
}

int complex_wide_begin(int *state) {
    int saved, mx, control;
    saved = temporary(108, 4);
    mx = temporary(4, 4);
    control = temporary(4, 4);
    state[0] = saved;
    state[1] = mx;
    sequence(0xb5dd, 2);
    emit32(-saved);
    sequence(0x9dae0f, 3);
    emit32(-mx);
    load_opcode(1, 0x85);
    emit32(-mx);
    sequence(0xc189, 2);
    sequence(0x03e8c1, 3);
    emit(0x25);
    emit32(0xc00);
    sequence(0x07e9c1, 3);
    sequence(0x3fe183, 3);
    sequence(0xc809, 2);
    emit(0x0d);
    emit32(0x340);
    store(control, 1);
    sequence(0xadd9, 2);
    emit32(-control);
    return 0;
}

int complex_wide_end(int *state) {
    emit(0x9b);
    sequence(0xe0df, 2);
    emit(0x25);
    emit32(0x3f);
    sequence(0x8509, 2);
    emit32(-state[1]);
    sequence(0xa5dd, 2);
    emit32(-state[0]);
    sequence(0x95ae0f, 3);
    emit32(-state[1]);
    return 0;
}

int complex_multiply_finite(int left, int right, int part, int result) {
    int values[4], nonfinite[4], i, state[2], end;
    for (i = 0; i < 4; i++) {
        values[i] = temporary(8, 8);
        complex_part(i < 2 ? left : right, part, i & 1);
        store(values[i], part);
        complex_classify_slot(values[i], part);
        nonfinite[i] = jump(0x83);
    }
    /* Extended intermediates keep products and sums in range. */
    complex_wide_begin(state);
    for (i = 0; i < 2; i++) {
        emit(part == 13 ? 0xd9 : 0xdd);
        emit(0x85);
        emit32(-values[0]);
        emit(part == 13 ? 0xd8 : 0xdc);
        emit(0x8d);
        emit32(-values[2 + i]);
        emit(part == 13 ? 0xd9 : 0xdd);
        emit(0x85);
        emit32(-values[1]);
        emit(part == 13 ? 0xd8 : 0xdc);
        emit(0x8d);
        emit32(-values[3 - i]);
        sequence(i ? 0xc1de : 0xe9de, 2);
        emit(part == 13 ? 0xd9 : 0xdd);
        emit(0x9d);
        emit32(i * type_size(part) - result);
    }
    complex_wide_end(state);
    end = jump(0);
    for (i = 0; i < 4; i++)
        patch(nonfinite[i], position);
    return end;
}

int complex_divide_finite(int *values, int part, int result) {
    int nonfinite[4], state[2], i, den, end;
    for (i = 0; i < 4; i++) {
        complex_classify_slot(values[i], part);
        nonfinite[i] = jump(0x83);
    }
    complex_wide_begin(state);
    den = temporary(16, 8);
    for (i = 2; i < 4; i++) {
        emit(part == 13 ? 0xd9 : 0xdd);
        emit(0x85);
        emit32(-values[i]);
        emit(part == 13 ? 0xd8 : 0xdc);
        emit(0x8d);
        emit32(-values[i]);
    }
    sequence(0xc1de, 2);
    sequence(0xbddb, 2);
    emit32(-den);
    for (i = 0; i < 2; i++) {
        emit(part == 13 ? 0xd9 : 0xdd);
        emit(0x85);
        emit32(-values[i]);
        emit(part == 13 ? 0xd8 : 0xdc);
        emit(0x8d);
        emit32(-values[2]);
        emit(part == 13 ? 0xd9 : 0xdd);
        emit(0x85);
        emit32(-values[1 - i]);
        emit(part == 13 ? 0xd8 : 0xdc);
        emit(0x8d);
        emit32(-values[3]);
        sequence(i ? 0xe9de : 0xc1de, 2);
        sequence(0xaddb, 2);
        emit32(-den);
        sequence(0xf9de, 2);
        emit(part == 13 ? 0xd9 : 0xdd);
        emit(0x9d);
        emit32(i * type_size(part) - result);
    }
    complex_wide_end(state);
    end = jump(0);
    for (i = 0; i < 4; i++)
        patch(nonfinite[i], position);
    return end;
}

int complex_divide_infinite(int *values, int part, int result) {
    int fail[3], state[2], i, j, k, base, hit, other, boxed, first = 0, end = 0, scale;
    for (i = 0; i < 2; i++) {
        /* One operand is finite; the other contains an infinity. */
        base = i ? 0 : 2;
        for (j = 0; j < 2; j++) {
            complex_classify_slot(values[base + j], part);
            fail[j] = jump(0x83);
        }
        base = i ? 2 : 0;
        complex_classify_slot(values[base], part);
        hit = jump(0x84);
        complex_classify_slot(values[base + 1], part);
        fail[2] = jump(0x85);
        patch(hit, position);
        for (j = 0; j < 2; j++) {
            complex_classify_slot(values[base + j], part);
            other = jump(0x85);
            complex_signed_box(values[base + j], part, 1);
            boxed = jump(0);
            patch(other, position);
            complex_signed_box(values[base + j], part, 0);
            patch(boxed, position);
        }
        scale = temporary(8, 8);
        complex_real_bits(part, !i && part == 13 ? 0x7f800000 : 0, i ? 0 : 0x7ff00000);
        store(scale, part);
        complex_wide_begin(state);
        /* Wide cross-products avoid an undue overflow before scaling to
           signed zero or infinity. The denominator magnitude is irrelevant. */
        for (k = 0; k < 2; k++) {
            emit(part == 13 ? 0xd9 : 0xdd);
            emit(0x85);
            emit32(-values[k]);
            emit(part == 13 ? 0xd8 : 0xdc);
            emit(0x8d);
            emit32(-values[2]);
            emit(part == 13 ? 0xd9 : 0xdd);
            emit(0x85);
            emit32(-values[1 - k]);
            emit(part == 13 ? 0xd8 : 0xdc);
            emit(0x8d);
            emit32(-values[3]);
            sequence(k ? 0xe9de : 0xc1de, 2);
            emit(part == 13 ? 0xd8 : 0xdc);
            emit(0x8d);
            emit32(-scale);
            emit(part == 13 ? 0xd9 : 0xdd);
            emit(0x9d);
            emit32(k * type_size(part) - result);
        }
        complex_wide_end(state);
        if (!i)
            first = jump(0);
        else {
            patch(first, position);
            end = jump(0);
        }
        for (j = 0; j < 3; j++)
            patch(fail[j], position);
    }
    return end;
}

int complex_multiply_recover(int left, int right, int part, int result) {
    int values[4], flags[2], done[3], overflow[4], i, j, branch, end, hit, hit2, temp, other;
    /* Keep the ordinary result unless both components are NaNs. */
    for (i = 0; i < 2; i++) {
        complex_classify_slot(result - i * type_size(part), part);
        done[i] = jump(0x86);
    }
    for (i = 0; i < 4; i++) {
        values[i] = temporary(8, 8);
        complex_part(i < 2 ? left : right, part, i & 1);
        store(values[i], part);
    }
    for (i = 0; i < 2; i++) {
        flags[i] = temporary(4, 4);
        complex_classify_slot(values[2 * i], part);
        hit = jump(0x84);
        complex_classify_slot(values[2 * i + 1], part);
        hit2 = jump(0x84);
        emit(0xb8);
        emit32(0);
        end = jump(0);
        patch(hit, position);
        patch(hit2, position);
        emit(0xb8);
        emit32(1);
        patch(end, position);
        store(flags[i], 1);
    }
    load_opcode(1, 0x85);
    emit32(-flags[0]);
    sequence(0xc189, 2);
    load_opcode(1, 0x85);
    emit32(-flags[1]);
    sequence(0xc809, 2);
    sequence(0xc085, 2);
    branch = jump(0x85);
    /* A finite component product may overflow while a NaN in another
       component contaminates both sums. Recover that case as well. */
    temp = temporary(8, 8);
    for (i = 0; i < 4; i++) {
        complex_product(left, right, part, i / 2, i & 1);
        store(temp, part);
        complex_classify_slot(temp, part);
        overflow[i] = jump(0x84);
    }
    done[2] = jump(0);
    patch(branch, position);
    for (i = 0; i < 4; i++)
        patch(overflow[i], position);
    for (i = 0; i < 2; i++) {
        load_opcode(1, 0x85);
        emit32(-flags[i]);
        sequence(0xc085, 2);
        branch = jump(0x84);
        for (j = 0; j < 2; j++) {
            complex_classify_slot(values[2 * i + j], part);
            other = jump(0x85);
            complex_signed_box(values[2 * i + j], part, 1);
            end = jump(0);
            patch(other, position);
            complex_signed_box(values[2 * i + j], part, 0);
            patch(end, position);
        }
        end = jump(0);
        patch(branch, position);
        for (j = 0; j < 2; j++) {
            complex_classify_slot(values[2 * i + j], part);
            other = jump(0x86);
            complex_signed_box(values[2 * i + j], part, 0);
            patch(other, position);
        }
        patch(end, position);
    }
    temp = temporary(8, 8);
    other = temporary(8, 8);
    for (i = 0; i < 2; i++) {
        complex_slot_op('*', values[0], values[2 + i], part, temp);
        complex_slot_op('*', values[1], values[3 - i], part, other);
        complex_slot_op(i ? '+' : '-', temp, other, part, temp);
        emit(0x50);
        complex_real_bits(part, part == 13 ? 0x7f800000 : 0, 0x7ff00000);
        floating_binary('*', part, part);
        store(result - i * type_size(part), part);
    }
    for (i = 0; i < 3; i++)
        patch(done[i], position);
    return 0;
}

int complex_scale_large_denominator(int *values, int part) {
    int i, scale = 0, skip = 0;
    /* If either divisor component is at least 2^(emax-1), halve all
       components before the ratio formula. The quotient is unchanged. */
    for (i = 2; i < 4; i++) {
        load_opcode(part, 0x85);
        emit32(-values[i]);
        floating_absolute_bits(part);
        emit(0x50);
        complex_real_bits(part, part == 13 ? 0x7e800000 : 0, 0x7fd00000);
        floating_binary(131, part, part);
        scalar_test();
        if (i == 2)
            scale = jump(0x85);
        else
            skip = jump(0x84);
    }
    patch(scale, position);
    for (i = 0; i < 4; i++) {
        load_opcode(part, 0x85);
        emit32(-values[i]);
        emit(0x50);
        complex_real_bits(part, part == 13 ? 0x3f000000 : 0, 0x3fe00000);
        floating_binary('*', part, part);
        store(values[i], part);
    }
    patch(skip, position);
    return 0;
}

int complex_divide(int left, int right, int part, int result) {
    int values[4], i, ratio, den, temp, branch, end = 0, c, d, axis_real, axis_imaginary, finite,
                                                infinite;
    for (i = 0; i < 4; i++) {
        values[i] = temporary(8, 8);
        complex_part(i < 2 ? left : right, part, i & 1);
        store(values[i], part);
    }
    /* Axis divisors need no ratio or zero cross-products. */
    load_opcode(part, 0x85);
    emit32(-values[3]);
    scalar_test();
    branch = jump(0x85);
    complex_slot_op('/', values[0], values[2], part, result);
    complex_slot_op('/', values[1], values[2], part, result - type_size(part));
    axis_real = jump(0);
    patch(branch, position);
    load_opcode(part, 0x85);
    emit32(-values[2]);
    scalar_test();
    branch = jump(0x85);
    complex_slot_op('/', values[1], values[3], part, result);
    complex_slot_op('/', values[0], values[3], part, result - type_size(part));
    floating_negate(part);
    store(result - type_size(part), part);
    axis_imaginary = jump(0);
    patch(branch, position);
    finite = complex_divide_finite(values, part, result);
    infinite = complex_divide_infinite(values, part, result);
    complex_scale_large_denominator(values, part);
    ratio = temporary(8, 8);
    den = temporary(8, 8);
    temp = temporary(8, 8);
    load_opcode(part, 0x85);
    emit32(-values[2]);
    floating_absolute_bits(part);
    emit(0x50);
    load_opcode(part, 0x85);
    emit32(-values[3]);
    floating_absolute_bits(part);
    floating_binary(131, part, part);
    scalar_test();
    branch = jump(0x84);
    for (i = 0; i < 2; i++) {
        if (i)
            patch(branch, position);
        c = values[i ? 3 : 2];
        d = values[i ? 2 : 3];
        complex_slot_op('/', d, c, part, ratio);
        complex_slot_op('*', d, ratio, part, temp);
        complex_slot_op('+', c, temp, part, den);
        complex_slot_op('*', values[i ? 0 : 1], ratio, part, temp);
        complex_slot_op('+', values[i ? 1 : 0], temp, part, temp);
        complex_slot_op('/', temp, den, part, result);
        complex_slot_op('*', values[i ? 1 : 0], ratio, part, temp);
        if (i)
            complex_slot_op('-', temp, values[0], part, temp);
        else
            complex_slot_op('-', values[1], temp, part, temp);
        complex_slot_op('/', temp, den, part, result - type_size(part));
        if (!i)
            end = jump(0);
    }
    patch(end, position);
    patch(finite, position);
    patch(infinite, position);
    patch(axis_real, position);
    patch(axis_imaginary, position);
    return 0;
}

int complex_binary(int op, int a, int b) {
    int t, part, left, right, result, i, finite = 0, equality = op == 128 || op == 129;
    if (op != '+' && op != '-' && op != '*' && op != '/' && !equality)
        return error_message("unsupported complex operator");
    t = complex_common(a, b);
    if (failed)
        return 0;
    part = complex_component(t);
    right = temporary(8, 8);
    left = temporary(8, 8);
    expression_type = b;
    convert(t);
    store(right, 11);
    emit(0x58);
    expression_type = a;
    convert(t);
    store(left, 11);
    result = temporary(type_size(t), type_align(t));
    if (op == '/' && complex_component(b)) {
        complex_divide(left, right, part, result);
        sequence(0x858d48, 3);
        emit32(-result);
        return record_value(t);
    }
    if (op == '*' && complex_component(a) && complex_component(b))
        finite = complex_multiply_finite(left, right, part, result);
    for (i = 0; i < 2; i++) {
        if (op == '*') {
            if (complex_component(a) && complex_component(b)) {
                complex_product(left, right, part, 0, i);
                emit(0x50);
                complex_product(left, right, part, 1, 1 - i);
                floating_binary(i ? '+' : '-', part, part);
            } else
                complex_product(left, right, part, complex_component(a) ? i : 0,
                                complex_component(b) ? i : 0);
        } else if (op == '/') {
            complex_part(left, part, i);
            emit(0x50);
            complex_part(right, part, 0);
            floating_binary('/', part, part);
        } else if (i && !equality && (!complex_component(a) || !complex_component(b))) {
            complex_part(complex_component(a) ? left : right, part, 1);
            if (op == '-' && !complex_component(a))
                floating_negate(part);
        } else {
            complex_part(left, part, i);
            emit(0x50);
            complex_part(right, part, i);
            floating_binary(op, part, part);
        }
        store(result - i * type_size(part), equality ? 1 : part);
    }
    if (op == '*' && complex_component(a) && complex_component(b))
        complex_multiply_recover(left, right, part, result);
    if (finite)
        patch(finite, position);
    if (equality) {
        load_opcode(1, 0x85);
        emit32(-result);
        sequence(0xc189, 2);
        load_opcode(1, 0x85);
        emit32(type_size(part) - result);
        sequence(op == 128 ? 0xc821 : 0xc809, 2);
        expression_type = 1;
        return ice_set(0, 0);
    }
    sequence(0x858d48, 3);
    emit32(-result);
    return record_value(t);
}

int binary(int op, int left_type, int right_type, int left_zero) {
    int t, u, wide;
    if (complex_component(left_type) || complex_component(right_type))
        return complex_binary(op, left_type, right_type);
    if (is_record(left_type) || is_record(right_type))
        return error();
    if (floating_type(left_type) || floating_type(right_type))
        return floating_binary(op, left_type, right_type);
    if (is_pointer(left_type) || is_pointer(right_type))
        return pointer_binary(op, left_type, right_type, left_zero);
    t = common_type(left_type, right_type);
    expression_zero = 0;
    expression_ice = 0;
    if (op == 132 || op == 133)
        t = promoted(left_type);
    u = unsigned_type(t);
    wide = wide_type(t);
    expression_type = right_type;
    convert(t);
    if (wide)
        sequence(0xc18948, 3);
    else
        sequence(0xc189, 2);
    emit(0x58);
    expression_type = left_type;
    convert(t);
    if (op == '/' || op == '%') {
        if (u)
            sequence(0xd231, 2);
        else {
            if (wide)
                emit(0x48);
            emit(0x99);
        }
        if (wide)
            emit(0x48);
        if (u)
            sequence(0xf1f7, 2);
        else
            sequence(0xf9f7, 2);
        if (op == '%') {
            if (wide)
                emit(0x48);
            sequence(0xd089, 2);
        }
    } else if (op == 128)
        comparison(0x94);
    else if (op == 129)
        comparison(0x95);
    else if (op == '<') {
        if (u)
            comparison(0x92);
        else
            comparison(0x9c);
    } else if (op == '>') {
        if (u)
            comparison(0x97);
        else
            comparison(0x9f);
    } else if (op == 130) {
        if (u)
            comparison(0x96);
        else
            comparison(0x9e);
    } else if (op == 131) {
        if (u)
            comparison(0x93);
        else
            comparison(0x9d);
    } else {
        if (wide)
            emit(0x48);
        if (op == '+')
            sequence(0xc801, 2);
        else if (op == '-')
            sequence(0xc829, 2);
        else if (op == '*')
            sequence(0xc1af0f, 3);
        else if (op == '&')
            sequence(0xc821, 2);
        else if (op == '|')
            sequence(0xc809, 2);
        else if (op == '^')
            sequence(0xc831, 2);
        else if (op == 132)
            sequence(0xe0d3, 2);
        else if (op == 133) {
            if (u)
                sequence(0xe8d3, 2);
            else
                sequence(0xf8d3, 2);
        } else
            error();
    }
    return 0;
}

int expression(int minimum) {
    int left, op, prec, end, alternate, left_type, middle_type, left_zero, middle_zero;
    int right_type, result_type, join, field, left_ice, middle_ice, right_ice;
    unsigned long long left_value, middle_value, right_value;
    if (failed)
        return 0;
    expression_depth = expression_depth + 1;
    if (expression_depth > 128)
        return error();
    left = unary();
    while (!failed && precedence(token) >= minimum) {
        op = token;
        prec = precedence(op);
        left_type = expression_type;
        left_zero = expression_zero;
        field = expression_bit;
        left_ice = expression_ice;
        left_value = expression_value;
        if (prec == 1) {
            if (left == 0 || expression_array_type)
                return error();
            if (contains_const(left_type))
                return error_message("assignment requires modifiable non-const object");
            /* Computing an assignment destination does not read its value. */
            if (op == '=')
                rewind_code(expression_load);
            if (left == -1)
                emit(0x52);
            if (op != '=')
                emit(0x50);
            next();
            expression(1);
            if (op != '=')
                binary(compound(op), left_type, expression_type, left_zero);
            if (field)
                bitfield_store(field);
            else {
                convert(left_type);
                store(left, left_type);
            }
            expression_record_ready = 1;
            expression_zero = 0;
            expression_ice = 0;
        } else if (op == '?') {
            boolean(0x95);
            alternate = jump(0x84);
            next();
            full_expression();
            record_read();
            expect(':');
            middle_type = expression_type;
            middle_zero = expression_zero;
            middle_ice = expression_ice;
            middle_value = expression_value;
            end = jump(0);
            patch(alternate, position);
            expression(2);
            record_read();
            right_type = expression_type;
            right_ice = expression_ice;
            right_value = expression_value;
            if (is_pointer(middle_type) && expression_zero)
                expression_type = middle_type;
            else if (is_pointer(expression_type) && middle_zero)
                expression_type = expression_type;
            else if (middle_type || expression_type)
                expression_type = common_type(middle_type, expression_type);
            result_type = expression_type;
            expression_type = right_type;
            convert(result_type);
            join = jump(0);
            patch(end, position);
            expression_type = middle_type;
            expression_zero = middle_zero;
            convert(result_type);
            patch(join, position);
            ice_conditional(left_ice, left_value, middle_ice, middle_value, right_ice, right_value,
                            result_type);
        } else if (op == 134 || op == 135) {
            boolean(0x95);
            if (op == 134)
                end = jump(0x84);
            else
                end = jump(0x85);
            next();
            expression(prec + 1);
            right_ice = expression_ice;
            right_value = expression_value;
            boolean(0x95);
            patch(end, position);
            ice_logical(op, left_ice, left_value, right_ice, right_value);
        } else {
            emit(0x50);
            next();
            expression(prec + 1);
            right_type = expression_type;
            right_ice = expression_ice;
            right_value = expression_value;
            binary(op, left_type, right_type, left_zero);
            ice_binary(op, left_ice, left_value, left_type, right_ice, right_value, right_type);
        }
        left = 0;
        expression_bit = 0;
        expression_register = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
    }
    expression_depth = expression_depth - 1;
    return left;
}

/* Commas here sequence full expressions; argument and initializer parsers
   continue to call expression(1), where comma is a delimiter. */
int full_expression(void) {
    int left = expression(1), comma = 0, all_ice = expression_ice != 0;
    while (!failed && token == ',') {
        record_read();
        next();
        expression(1);
        comma = 1;
        all_ice = all_ice && expression_ice != 0;
        left = 0;
        expression_zero = 0;
        expression_ice = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
        expression_register = 0;
    }
    if (comma) {
        record_read();
        expression_bit = 0;
        expression_ice = all_ice && !c89_identifiers ? 2 : 0;
    }
    return left;
}

int increment(int left, int op, int postfix) {
    int t = expression_type, step = 1, field = expression_bit;
    if (is_record(t))
        return error();
    if (left == 0 || expression_array_type)
        return error();
    if (contains_const(t))
        return error_message("increment requires modifiable non-const object");
    if (left == -1)
        emit(0x52);
    if (is_pointer(t))
        step = pointer_stride(t);
    if (postfix) {
        if (wide_type(t))
            sequence(0xc08949, 3);
        else
            sequence(0xc08941, 3);
    }
    if (floating_type(t)) {
        emit(0x50);
        if (unqualified(t) == 13) {
            emit(0xb8);
            emit32(0x3f800000);
        } else {
            sequence(0xb848, 2);
            emit32(0);
            emit32(0x3ff00000);
        }
        expression_type = t;
        binary(op == 170 ? '+' : '-', t, t, 0);
    } else {
        if (step < -1) {
            if (op == 170)
                sequence(0x850348, 3);
            else
                sequence(0x852b48, 3);
            emit32(step + 1);
        } else {
            if (wide_type(t))
                emit(0x48);
            if (op == 170)
                sequence(0xc081, 2);
            else
                sequence(0xe881, 2);
            emit32(step);
        }
    }
    if (field)
        bitfield_store(field);
    else {
        convert(t);
        store(left, t);
    }
    if (postfix) {
        if (wide_type(t))
            sequence(0xc0894c, 3);
        else
            sequence(0xc08944, 3);
    }
    expression_zero = 0;
    expression_ice = 0;
    expression_array_bytes = 0;
    expression_array_type = 0;
    return 0;
}

int postfix_expression(int result) {
    int t, op;
    while (!failed && (token == 170 || token == 171 || token == '[' || token == '.' ||
                       token == 173 || token == '(')) {
        if (token == '(') {
            if (!function_pointer(expression_type))
                return error();
            result = arguments(-1, (pointed_type(expression_type) - 0x30000f) / 256);
        } else if (token == '.' || token == 173)
            result = member_access(result, token == 173);
        else if (token == '[') {
            t = expression_type;
            emit(0x50);
            next();
            full_expression();
            expect(']');
            binary('+', t, expression_type, 0);
            if ((!is_pointer(expression_type) || expression_type == 16))
                return error();
            t = pointed_type(expression_type);
            result = indirect_value(t);
        } else {
            op = token;
            next();
            increment(result, op, 1);
            result = 0;
        }
    }
    return result;
}

int unary(void) {
    int op = token;
    int result, t, saved_position, saved_calls, saved_allocation, bytes, register_id, ice,
        value_type, size_type_name, size_cursor, size_line, size_locals, size_peak;
    unsigned long long bits;
    if (failed)
        return 0;
    unary_depth = unary_depth + 1;
    expression_bit = 0;
    expression_ice = 0;
    if (unary_depth > 128)
        return error();
    if (op == 297) {
        next();
        expect('(');
        register_id = name();
        if (!current_variadic || !parameter_count)
            return error_message("va_start outside variadic definition");
        if (register_id != parameter_name[parameter_count - 1] ||
            local_offset[register_id] != parameter_offset[parameter_count - 1] ||
            register_object[register_id])
            return error_message("va_start requires last named parameter");
        next();
        expect(')');
        sequence(0x858d48, 3);
        emit32(current_va_offset);
        expression_type = 25;
        expression_zero = 0;
        expression_ice = 0;
        expression_register = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
        result = 0;
    } else if (op == 298) {
        next();
        expect('(');
        expression(1);
        t = expression_type;
        if (!is_pointer(t) || unqualified(pointed_type(t)) != 25 ||
            (type_qualifiers(pointed_type(t)) & 1))
            return error_message("va_arg requires writable va_list");
        expect(',');
        t = declarator(specifiers(), 1);
        expect(')');
        if (!unqualified(t) || is_array(t) || is_function(t))
            return error_message("invalid va_arg type");
        bytes = type_size(t);
        sequence(0x108b48, 3);
        sequence(0x084a8d48, 4);
        sequence(0x088948, 3);
        sequence(0xd08948, 3);
        if (is_record(t)) {
            if (record_indirect(t))
                sequence(0x008b48, 3);
            record_value(unqualified(t));
        } else
            load_opcode(unqualified(t), 0);
        expression_type = unqualified(t);
        expression_zero = 0;
        expression_ice = 0;
        expression_register = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
        result = 0;
    } else if (op == 296) {
        bytes = offset_expression();
        constant(bytes);
        expression_type = 11;
        expression_array_bytes = 0;
        expression_array_type = 0;
        result = 0;
    } else if (op == 283) {
        size_type_name = 0;
        size_cursor = cursor;
        size_line = line;
        size_locals = local_bytes;
        size_peak = frame_peak;
        saved_position = position;
        saved_calls = call_count;
        saved_allocation = allocation;
        unevaluated_depth++;
        next();
        if (token == '(') {
            next();
            if (type_start(token)) {
                size_type_name = 1;
                t = declarator(specifiers(), 1);
                expression_array_bytes = 0;
                expression_array_type = 0;
                expect(')');
                if (token == '{') {
                    postfix_expression(compound_literal(t));
                    t = expression_type;
                }
            } else {
                full_expression();
                t = expression_type;
                expect(')');
            }
        } else {
            unary();
            t = expression_type;
        }
        if (expression_bit)
            return error_message("sizeof cannot be applied to bit-field");
        bytes = expression_array_bytes;
        if (expression_array_type)
            t = expression_array_type;
        if (!prototype_depth && is_array(t) && array_bytes[(unqualified(t) - 4096) / 256] < -1) {
            bytes = array_bytes[(unqualified(t) - 4096) / 256];
            unevaluated_depth--;
            if (!size_type_name) {
                allocation = saved_allocation;
                rewind_code(saved_position);
                call_count = saved_calls;
                local_bytes = size_locals;
                frame_peak = size_peak;
                cursor = size_cursor;
                line = size_line;
                evaluated_sizeof++;
                next();
                if (token == '(') {
                    next();
                    full_expression();
                    expect(')');
                } else
                    unary();
                evaluated_sizeof--;
                if (expression_array_type)
                    bytes = array_bytes[(unqualified(expression_array_type) - 4096) / 256];
            }
            emit_extent(bytes);
            sizeof_value = 0;
            expression_array_bytes = 0;
            expression_array_type = 0;
            result = 0;
            unary_depth--;
            return result;
        }
        if (prototype_depth && is_array(t) && array_bytes[(unqualified(t) - 4096) / 256] < 0)
            bytes = -1;
        else if (!bytes)
            bytes = type_size(t);
        sizeof_value = bytes;
        unevaluated_depth--;
        allocation = saved_allocation;
        rewind_code(saved_position);
        call_count = saved_calls;
        constant(bytes < 0 ? 0 : bytes);
        expression_type = 11;
        expression_array_bytes = 0;
        expression_array_type = 0;
        result = 0;
        if (bytes < 0) {
            expression_ice = 0;
            expression_zero = 0;
        }
    } else if (op == '&') {
        next();
        result = unary();
        if (!result || expression_register || expression_bit)
            return error_message("cannot take address of this operand");
        if (result > 0) {
            for (register_id = 0; register_id < count; register_id++) {
                if (local_offset[register_id] == result && register_object[register_id])
                    return error();
            }
        }
        rewind_code(expression_load);
        if (result == -1)
            sequence(0xd08948, 3);
        else if (result < -1) {
            sequence(0x058d48, 3);
            object_reference(-result - 2);
        } else {
            sequence(0x858d48, 3);
            emit32(-result);
        }
        if (expression_array_type)
            expression_type = expression_array_type;
        expression_type = make_pointer(expression_type);
        expression_zero = 0;
        expression_ice = 0;
        result = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
    } else if (op == '*') {
        next();
        unary();
        if ((!is_pointer(expression_type) || expression_type == 16))
            return error();
        t = pointed_type(expression_type);
        result = indirect_value(t);
    } else if (op == 170 || op == 171) {
        next();
        result = unary();
        increment(result, op, 0);
        result = 0;
    } else if (op == '+' || op == '-' || op == '!' || op == '~') {
        next();
        unary();
        if ((op == '+' || op == '-') && complex_component(expression_type)) {
            complex_unary(op);
            unary_depth--;
            return 0;
        }
        if (op == '!' && complex_component(expression_type))
            complex_truth();
        if (!expression_type || is_record(expression_type))
            return error();
        if (is_pointer(expression_type) && op != '!')
            return error();
        expression_type = promoted(expression_type);
        expression_bit = 0;
        ice = expression_ice;
        bits = expression_value;
        value_type = expression_type;
        if (floating_type(expression_type)) {
            if (op == '~')
                return error();
            if (op == '-')
                floating_negate(expression_type);
            if (op == '!')
                boolean(0x94);
            ice_set(0, 0);
        } else {
            if (op == '-') {
                if (wide_type(expression_type))
                    emit(0x48);
                sequence(0xd8f7, 2);
            }
            if (op == '!')
                boolean(0x94);
            if (op == '~') {
                if (wide_type(expression_type))
                    emit(0x48);
                sequence(0xd0f7, 2);
            }
            ice_unary(op, ice, bits, value_type);
        }
        expression_array_bytes = 0;
        expression_array_type = 0;
        result = 0;
    } else {
        result = postfix_expression(primary());
    }
    unary_depth = unary_depth - 1;
    return result;
}

int fp_constant_type;
unsigned long long fp_constant_bits;
int floating_bits_constant(void);

int inline_reference(int id) {
    if (inline_body >= 0 && (symbol_flags[id] & 32) &&
        (!local_offset[id] || local_offset[id] == -2 || local_offset[id] == -4) &&
        !alias_type[id] && !enum_visible(id))
        inline_invalid[inline_body] = 1;
    return 0;
}

int generic_math_selector(void) {
    int n, i, t, p, rank = 1, domain = 0, id, chosen = -1, target;
    int start, calls, space, locals, peak;
    c89_extension("type-generic math selector");
    next();
    expect('(');
    if (token != 144 || value < 1 || value > 3)
        return error_message("invalid generic argument count");
    n = value;
    next();
    expect(',');
    start = position;
    calls = call_count;
    space = allocation;
    locals = local_bytes;
    peak = frame_peak;
    unevaluated_depth++;
    for (i = 0; i < n; i++) {
        expression(1);
        t = unqualified(expression_type);
        p = complex_component(t);
        if (p) {
            domain = 1;
            t = p;
        }
        if (t < 1 || t > 15 || expression_array_type)
            return error_message("generic math requires arithmetic arguments");
        p = t == 15 ? 3 : t == 13 ? 1 : 2;
        if (p > rank)
            rank = p;
        expect(',');
    }
    unevaluated_depth--;
    allocation = space;
    local_bytes = locals;
    frame_peak = peak;
    rewind_code(start);
    call_count = calls;
    target = domain * 3 + rank - 1;
    for (i = 0; i < 6; i++) {
        if (token == 144 && value == 0) {
            id = -1;
            next();
        } else {
            id = name();
            next();
        }
        if (i == target)
            chosen = id;
        if (i < 5)
            expect(',');
        else
            expect(')');
    }
    if (chosen < 0)
        return error_message("unsupported generic math type domain");
    if (!function_exists(chosen))
        return error_message("generic math function is not declared");
    sequence(0x058d48, 3);
    function_reference(chosen);
    expression_bit = 0;
    return function_value(function_signature(chosen));
}

int primary(void) {
    int id, result, t, n;
    if (failed)
        return 0;
    expression_array_bytes = 0;
    expression_array_type = 0;
    expression_register = 0;
    if (token == 301)
        return generic_math_selector();
    if (token == 300) {
        c89_extension("complex imaginary unit");
        next();
        t = complex_type(13);
        n = temporary(8, 4);
        complex_real_bits(13, 0, 0);
        store(n, 13);
        complex_real_bits(13, 0x3f800000, 0);
        store(n - 4, 13);
        sequence(0x858d48, 3);
        emit32(-n);
        record_value(qualify_type(t, 1));
        return 0;
    }
    if (token == 299) {
        floating_bits_constant();
        if (wide_type(fp_constant_type)) {
            sequence(0xb848, 2);
            emit32((int)fp_constant_bits);
            emit32((int)(fp_constant_bits >> 32));
        } else
            constant((int)fp_constant_bits);
        expression_type = fp_constant_type;
        expression_ice = 4;
        expression_value = fp_constant_bits;
        expression_zero = 0;
        return 0;
    }
    if (token == 145) {
        join_strings();
        return emit_string();
    }
    if (token == 144) {
        if (wide_type(literal_type)) {
            sequence(0xb848, 2);
            emit32(value);
            emit32(literal_high);
        } else
            constant(value);
        expression_type = literal_type;
        expression_value = ce_normalize(((unsigned long long)(unsigned int)literal_high << 32) |
                                            (unsigned int)value,
                                        literal_type);
        expression_ice = floating_type(literal_type) ? 4 : 1;
        expression_zero = expression_ice == 1 && expression_value == 0;
        next();
        return 0;
    }
    if (token == '(') {
        next();
        if (type_start(token)) {
            t = declarator(specifiers(), 1);
            expect(')');
            if (token == '{')
                return compound_literal(t);
            if (is_array(t))
                return error();
            unary();
            cast_to(t);
            expression_array_bytes = 0;
            expression_array_type = 0;
            return 0;
        }
        result = full_expression();
        expect(')');
        return result;
    }
    id = name();
    inline_reference(id);
    if (alias_type[id])
        return error();
    if (enum_visible(id)) {
        constant(enum_value(id));
        next();
        return 0;
    }
    next();
    if (token == '(' &&
        (local_offset[id] == -4 || (!local_offset[id] && !(symbol_flags[id] & 8)))) {
        if (!function_exists(id))
            implicit_function(id);
        return arguments(id, -1);
    }
    if (function_exists(id)) {
        sequence(0x058d48, 3);
        function_reference(id);
        return function_value(function_signature(id));
    }
    return load(id);
}

int arguments(int id, int signature) {
    int n = 0, i, saved_stack, result, output = 0, hidden = 0, t, copy, target = 0, expected = -1;
    int actual_types[128];
    if (id >= 0) {
        if (!function_exists(id))
            return error();
        signature = (function_signature(id) - 0x30000f) / 256;
        result = function_result[signature];
        expected = function_arity[signature];
    } else {
        result = function_result[signature];
        expected = function_arity[signature];
        target = temporary(8, 8);
        store(target, expression_type);
    }
    if (is_record(result)) {
        output = temporary(type_size(result), 16);
        hidden = record_indirect(result);
        if (hidden) {
            sequence(0x858d48, 3);
            emit32(-output);
            emit(0x50);
            actual_types[0] = 11;
        }
    }
    expect('(');
    while (!failed && token != ')') {
        if (n == 127)
            return error();
        expression(1);
        if (expected >= 0 && n < fixed_arity(expected)) {
            convert(function_parameter[signature * 128 + n]);
        } else {
            if (expected >= 0 && expected < 128)
                return error();
            if (!expression_type)
                return error();
            convert(default_promoted(expression_type));
        }
        t = expression_type;
        actual_types[n + hidden] = t;
        if (is_record(t)) {
            if (record_indirect(t)) {
                copy = temporary(type_size(t), 16);
                store_record(copy, t);
                sequence(0x858d48, 3);
                emit32(-copy);
            } else
                load_opcode(record_word(t), 0);
        }
        emit(0x50);
        n = n + 1;
        if (token != ',')
            break;
        next();
        if (token == ')')
            return error();
    }
    expect(')');
    if (expected >= 0 && !arity_matches(expected, n))
        return error();
    if (id >= 0)
        symbol_flags[id] = symbol_flags[id] | 2;
    n += hidden;
    if (n > 4) {
        /* R11 addresses the evaluated arguments in reverse stack order.
           A separate aligned area contains shadow space and ABI stack slots. */
        sequence(0xe38949, 3);
        sequence(0xe48348, 3);
        emit(0xf0);
        saved_stack = n * 8;
        sequence(0xec8148, 3);
        emit32(((saved_stack + 8 + 15) / 16) * 16);
        sequence(0x838d49, 3);
        emit32(n * 8);
        sequence(0x24848948, 4);
        emit32(saved_stack);
        sequence(0x8b8b49, 3);
        emit32((n - 1) * 8);
        sequence(0x938b49, 3);
        emit32((n - 2) * 8);
        sequence(0x838b4d, 3);
        emit32((n - 3) * 8);
        sequence(0x8b8b4d, 3);
        emit32((n - 4) * 8);
        for (i = 4; i < n; i = i + 1) {
            sequence(0x838b49, 3);
            emit32((n - i - 1) * 8);
            sequence(0x24848948, 4);
            emit32(i * 8);
        }
        floating_arguments(actual_types, n);
        invoke(id, target, n - hidden);
        sequence(0x24a48b48, 4);
        emit32(saved_stack);
    } else {
        if (n == 4)
            sequence(0x5941, 2);
        if (n >= 3)
            sequence(0x5841, 2);
        if (n >= 2)
            emit(0x5a);
        if (n >= 1)
            emit(0x59);
        sequence(0xe38949, 3);
        sequence(0xe48348, 3);
        emit(0xf0);
        sequence(0x30ec8348, 4);
        sequence(0x245c894c, 4);
        emit(0x20);
        floating_arguments(actual_types, n);
        invoke(id, target, n - hidden);
        sequence(0x24648b48, 4);
        emit(0x20);
    }
    if (floating_type(result))
        floating_from_xmm(result, 0);
    if (output) {
        if (!hidden)
            store(output, record_word(result));
        sequence(0x858d48, 3);
        emit32(-output);
        record_value(result);
        expression_record_ready = 1;
    }
    expression_type = result;
    expression_register = 0;
    expression_array_bytes = 0;
    expression_array_type = 0;
    expression_zero = 0;
    expression_ice = 0;
    convert(expression_type);
    return 0;
}

int bind_variable_type(int t, int stack) {
    int node;
    if (prototype_depth || !variably_modified_type(t))
        return 0;
    if (vm_count == 4095)
        return error_message("too many variably modified declarations");
    node = ++vm_count;
    vm_parent[node] = vm_current;
    vm_binding[node] = local_count;
    vm_stack[node] = stack;
    vm_current = node;
    return 0;
}

int record_vm_goto(int label) {
    int n = vm_goto_count++;
    if (n == 4096)
        return error_message("too many goto statements");
    vm_goto_label[n] = label;
    vm_goto_from[n] = vm_current;
    vm_goto_code[n] = position;
    return 0;
}

int resolve_vm_gotos(void) {
    int i, node, target, stack, p;
    for (i = 0; i < vm_goto_count; i++) {
        node = vm_goto_from[i];
        target = label_vm[vm_goto_label[i]];
        stack = 0;
        while (node && node != target) {
            if (vm_stack[node])
                stack = vm_stack[node];
            node = vm_parent[node];
        }
        if (node != target)
            return error_message("goto enters scope of variably modified identifier");
        if (stack) {
            patch(vm_goto_code[i] + 1, position);
            sequence(0xa58b48, 3);
            emit32(8 - stack);
            p = jump(0);
            patch(p, label_address[vm_goto_label[i]]);
        }
    }
    return 0;
}

int runtime_array_type(int t) {
    return is_array(t) && array_bytes[(unqualified(t) - 4096) / 256] < -1;
}

int allocate_runtime_array(int t, int offset) {
    int start, end, back;
    sequence(0xa58948, 3);
    emit32(8 - offset);
    emit_extent(array_bytes[(unqualified(t) - 4096) / 256]);
    sequence(0xc08348, 3);
    emit(15);
    sequence(0xe08348, 3);
    emit(240);
    start = position;
    sequence(0xf88148, 3);
    emit32(4096);
    end = jump(0x82);
    sequence(0xec8148, 3);
    emit32(4096);
    sequence(0x2404f6, 3);
    emit(0);
    sequence(0xe88148, 3);
    emit32(4096);
    back = jump(0);
    patch(back, start);
    patch(end, position);
    sequence(0xc42948, 3);
    sequence(0x2404f6, 3);
    emit(0);
    sequence(0xa58948, 3);
    emit32(-offset);
    return 0;
}

int local(int t, int id) {
    int bytes;
    if (failed)
        return 0;
    if (!t)
        return error();
    if ((local_offset[id] != 0 || alias_type[id]) && local_depth[id] == block_depth)
        return error();
    if (local_count == 256)
        return error();
    bytes = runtime_array_type(t) ? 16 : type_size(t);
    if (failed)
        return 0;
    binding_name[local_count] = id;
    binding_offset[local_count] = local_offset[id];
    binding_depth[local_count] = local_depth[id];
    binding_type[local_count] = local_type[id];
    binding_bytes[local_count] = local_bytes;
    binding_alias[local_count] = alias_type[id];
    alias_type[id] = 0;
    binding_register[local_count] = register_object[id];
    register_object[id] = 0;
    binding_static_address[local_count] = local_static_address[id];
    local_static_address[id] = 0;
    local_bytes = local_bytes + ((bytes + 7) / 8) * 8;
    if (local_bytes > 65536)
        return error();
    if (local_bytes > frame_peak)
        frame_peak = local_bytes;
    local_count = local_count + 1;
    local_offset[id] = local_bytes;
    local_depth[id] = block_depth;
    local_type[id] = t;
    bind_variable_type(t, runtime_array_type(t) ? local_offset[id] : 0);
    return local_offset[id];
}

int declaration(void) {
    int offset, base = specifiers(), is_alias = decl_typedef, storage = decl_storage, t, id,
                inferred, original, staging;
    if (token == ';' && (is_enum(base) || is_record(base)) && !storage) {
        if (decl_inline)
            return error();
        if (for_declaration)
            return error();
        next();
        return 0;
    }
    if (is_alias) {
        if (decl_inline)
            return error();
        if (for_declaration)
            return error();
        return alias_declaration(base, 0);
    }
    if (storage == 4) {
        if (for_declaration)
            return error();
        return static_declaration(base);
    }
    if (storage == 5) {
        if (for_declaration)
            return error();
        return extern_declaration(base);
    }
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        if (is_function(t)) {
            if (for_declaration || (token != ',' && token != ';'))
                return error();
            block_function(id, t, storage);
        } else {
            if (decl_inline)
                return error();
            inferred = is_array(t) && array_length[(t - 4096) / 256] == 0;
            original = t;
            if (inferred)
                t = infer_initializer(t, id, 0);
            offset = local(t, id);
            if (runtime_array_type(t)) {
                if (token == '=')
                    return error_message("variable length array cannot have an initializer");
                allocate_runtime_array(t, offset);
            }
            if (inferred)
                local_type[id] = original;
            register_object[id] = storage == 3;
            if (token == '=') {
                next();
                staging = offset;
                if ((is_array(t) || is_record(t)) && contains_volatile(t))
                    staging = temporary(type_size(t), type_align(t));
                if (is_array(t) || is_record(t))
                    zero_local(staging, type_size(t));
                initialize_object(t, staging, 0, 0, 0);
                if (staging != offset) {
                    sequence(0x858d48, 3);
                    emit32(-staging);
                    store_record(offset, t);
                }
            }
            local_type[id] = t;
        }
        if (token != ',')
            break;
        next();
    }
    return expect(';');
}

int finish_breaks(int head, int target) {
    int previous;
    while (head != 0 && !failed) {
        previous = read32(head);
        patch(head, target);
        head = previous;
    }
    return 0;
}

int restore_locals(int saved) {
    int id;
    while (local_count > saved) {
        local_count = local_count - 1;
        id = binding_name[local_count];
        local_bytes = binding_bytes[local_count];
        if (id < 0)
            continue;
        if (local_offset[id] > 0 && runtime_array_type(local_type[id])) {
            sequence(0xa58b48, 3);
            emit32(8 - local_offset[id]);
        }
        local_offset[id] = binding_offset[local_count];
        local_depth[id] = binding_depth[local_count];
        local_type[id] = binding_type[local_count];
        alias_type[id] = binding_alias[local_count];
        register_object[id] = binding_register[local_count];
        local_static_address[id] = binding_static_address[local_count];
        local_bytes = binding_bytes[local_count];
    }
    while (vm_current && vm_binding[vm_current] > local_count)
        vm_current = vm_parent[vm_current];
    return 0;
}

int statement(void) {
    if (failed)
        return 0;
    statement_depth = statement_depth + 1;
    if (statement_depth > 128)
        return error();
    statement_body();
    statement_depth = statement_depth - 1;
    return 0;
}

int scalar_test(void) {
    if (complex_component(expression_type))
        complex_truth();
    if (!expression_type || is_record(expression_type))
        return error();
    if (floating_type(expression_type))
        floating_absolute_bits(expression_type);
    if (wide_type(expression_type))
        emit(0x48);
    return sequence(0xc085, 2);
}

int release_runtime_arrays(int saved) {
    int i, j, id, t, offset;
    for (i = local_count - 1; i >= saved; i--) {
        id = binding_name[i];
        if (id < 0)
            continue;
        t = local_type[id];
        offset = local_offset[id];
        for (j = i + 1; j < local_count; j++)
            if (binding_name[j] == id) {
                t = binding_type[j];
                offset = binding_offset[j];
                break;
            }
        if (offset > 0 && runtime_array_type(t)) {
            sequence(0xa58b48, 3);
            emit32(8 - offset);
        }
    }
    return 0;
}

int statement_body(void) {
    int target, end, start, old_break, old_continue, saved_break_locals, saved_continue_locals;
    int saved_tags;
    int condition, increment, body, saved_locals, for_scope = 0, seen_statement = 0;
    if (token == 286)
        return switch_statement();
    if (token == 269 || token == 272)
        return switch_label();
    if (label_ahead()) {
        target = name();
        next();
        expect(':');
        if (label_address[target])
            return error();
        label_address[target] = position;
        label_vm[target] = vm_current;
        finish_breaks(label_head[target], position);
        label_head[target] = 0;
        return statement();
    }
    if (token == 278) {
        next();
        start = name();
        next();
        expect(';');
        if (failed)
            return 0;
        record_vm_goto(start);
        target = jump(0);
        if (label_address[start])
            patch(target, label_address[start]);
        else {
            write32(target, label_head[start]);
            label_head[start] = target;
        }
        return 0;
    }
    if (token == '{') {
        saved_locals = local_count;
        saved_tags = tag_count;
        block_depth = block_depth + 1;
        next();
        while (!failed && token != '}') {
            if (token == 0)
                return error();
            if (type_start(token) && !label_ahead()) {
                if (seen_statement)
                    c89_extension("declaration after statement");
                declaration();
            } else {
                seen_statement = 1;
                statement();
            }
        }
        expect('}');
        restore_locals(saved_locals);
        restore_tags(saved_tags);
        block_depth = block_depth - 1;
        return 0;
    }
    if (type_start(token))
        return error();
    if (token == 257) {
        next();
        if (token == ';') {
            if (!c89_identifiers && unqualified(return_type))
                return error();
            next();
            return epilogue();
        }
        if (!unqualified(return_type))
            return error();
        full_expression();
        convert(return_type);
        if (floating_type(return_type))
            floating_to_xmm(return_type, 0, 0);
        if (is_record(return_type)) {
            if (record_indirect(return_type)) {
                sequence(0x8d8b48, 3);
                emit32(-return_address_offset);
                emit(0x51);
                store_record(-1, return_type);
                sequence(0x858b48, 3);
                emit32(-return_address_offset);
            } else
                load_opcode(record_word(return_type), 0);
        }
        expect(';');
        return epilogue();
    }
    if (token == 258) {
        next();
        expect('(');
        full_expression();
        expect(')');
        scalar_test();
        target = jump(0x84);
        statement();
        end = jump(0);
        patch(target, position);
        if (token == 259) {
            next();
            statement();
        }
        patch(end, position);
        return 0;
    }
    if (token == 273) {
        old_break = break_head;
        old_continue = continue_target;
        saved_break_locals = break_local_count;
        saved_continue_locals = continue_local_count;
        break_local_count = local_count;
        continue_local_count = local_count;
        break_head = 0;
        loop_depth = loop_depth + 1;
        /* First entry skips the continue trampoline. Its forward jump is
           patched to the condition after the body has been generated. */
        body = jump(0);
        continue_target = position;
        condition = jump(0);
        patch(body, position);
        start = position;
        next();
        statement();
        expect(260);
        expect('(');
        patch(condition, position);
        full_expression();
        expect(')');
        expect(';');
        scalar_test();
        target = jump(0x85);
        patch(target, start);
        finish_breaks(break_head, position);
        break_head = old_break;
        continue_target = old_continue;
        break_local_count = saved_break_locals;
        continue_local_count = saved_continue_locals;
        loop_depth = loop_depth - 1;
        return 0;
    }
    if (token == 260 || token == 265) {
        old_break = break_head;
        old_continue = continue_target;
        saved_break_locals = break_local_count;
        saved_continue_locals = continue_local_count;
        break_local_count = local_count;
        continue_local_count = local_count;
        break_head = 0;
        loop_depth = loop_depth + 1;
        if (token == 260) {
            start = position;
            continue_target = start;
            next();
            expect('(');
            full_expression();
            expect(')');
            scalar_test();
            end = jump(0x84);
            statement();
            target = jump(0);
            patch(target, start);
        } else {
            saved_locals = local_count;
            saved_tags = tag_count;
            block_depth = block_depth + 1;
            for_scope = 1;
            next();
            expect('(');
            if (type_start(token)) {
                c89_extension("declaration in for initializer");
                for_declaration = 1;
                declaration();
                for_declaration = 0;
            } else {
                if (token != ';') {
                    full_expression();
                    record_read();
                }
                expect(';');
            }
            condition = position;
            end = 0;
            if (token != ';') {
                full_expression();
                scalar_test();
                end = jump(0x84);
            }
            expect(';');
            body = jump(0);
            continue_local_count = local_count;
            increment = position;
            continue_target = increment;
            if (token != ')') {
                full_expression();
                record_read();
            }
            expect(')');
            target = jump(0);
            patch(target, condition);
            patch(body, position);
            statement();
            target = jump(0);
            patch(target, increment);
        }
        if (end != 0)
            patch(end, position);
        finish_breaks(break_head, position);
        break_head = old_break;
        continue_target = old_continue;
        break_local_count = saved_break_locals;
        continue_local_count = saved_continue_locals;
        if (for_scope) {
            restore_locals(saved_locals);
            restore_tags(saved_tags);
            block_depth = block_depth - 1;
        }
        loop_depth = loop_depth - 1;
        return 0;
    }
    if (token == 263 || token == 264) {
        if (token == 264 && loop_depth == 0)
            return error();
        if (token == 263 && loop_depth == 0 && switch_depth == 0)
            return error();
        release_runtime_arrays(token == 263 ? break_local_count : continue_local_count);
        target = jump(0);
        if (token == 263) {
            write32(target, break_head);
            break_head = target;
        } else
            patch(target, continue_target);
        next();
        return expect(';');
    }
    if (token == ';') {
        next();
        return 0;
    }
    full_expression();
    record_read();
    return expect(';');
}

int initializer_address(int base, int offset, int global) {
    if (global) {
        sequence(0x0d8d48, 3);
        relative(base + offset);
    } else {
        sequence(0x8d8d48, 3);
        emit32(offset - base);
    }
    emit(0x51);
    return 0;
}

int join_strings(void) {
    joined_string_start = literal_string_start;
    joined_string_length = 0;
    joined_string_wide = 0;
    if (token != 145)
        return error();
    while (!failed && token == 145) {
        joined_string_length = joined_string_length + literal_string_length;
        joined_string_wide |= literal_string_wide;
        next();
    }
    return 0;
}

int emit_string(void) {
    int skip, data, i;
    if (joined_string_wide && !(position & 1))
        emit(0x90);
    skip = jump(0);
    data = position;
    emitted_string_address = data + 0x400e00;
    for (i = 0; i < joined_string_length && !failed; i = i + 1) {
        emit(string_pool[joined_string_start + i] & 255);
        if (joined_string_wide)
            emit(string_pool[joined_string_start + i] >> 8);
    }
    emit(0);
    if (joined_string_wide)
        emit(0);
    patch(skip, position);
    sequence(0x058d48, 3);
    emit32(data - position - 4);
    sequence(0xc28948, 3);
    expression_load = position;
    array_value(make_array(joined_string_wide ? 8 : 9, joined_string_length + 1));
    return -1;
}

int character_array(int t) {
    if (!is_array(t))
        return 0;
    t = unqualified(array_base[(t - 4096) / 256]);
    return t == 8 ? 2 : (t == 5 || t == 6 || t == 9);
}

int initializer_scalar_shape(void) {
    int closes[128], conditions[128], depth = 0, conditional = 0, seen = 0;
    while (!failed) {
        if (!depth && !conditional && (token == ',' || token == '}'))
            break;
        /* Braces after a cast belong to a compound literal. This pass only
           counts initializer shape; the semantic pass validates the syntax. */
        if (!token || (!depth && token == ';'))
            return error();
        if (token == '(' || token == '[' || token == '{') {
            if (depth == 128)
                return error();
            conditions[depth] = conditional;
            conditional = 0;
            closes[depth++] = token == '(' ? ')' : token == '[' ? ']' : '}';
        } else if (token == ')' || token == ']' || token == '}') {
            if (!depth || conditional || closes[--depth] != token)
                return error();
            conditional = conditions[depth];
        } else if (token == '?')
            conditional++;
        else if (token == ':') {
            if (conditional)
                conditional--;
            else if (!depth)
                return error();
        }
        seen = 1;
        next();
    }
    if (!seen || depth || conditional)
        return error();
    return 0;
}

int initializer_more(int braces) {
    int saved_cursor = cursor, saved_line = line, saved_strings = string_used, next_token;
    if (token != ',')
        return 0;
    if (braces)
        return 1;
    next_token = lex();
    cursor = saved_cursor;
    line = saved_line;
    string_used = saved_strings;
    return next_token != '.' && next_token != '[' && next_token != '}';
}

int initializer_shape(int t) {
    int braces = 0, i = 0, n, element, node, maximum = 0, designated = initializer_designated;
    initializer_designated = 0;
    if (failed)
        return 0;
    initializer_depth++;
    if (initializer_depth > 64)
        return error();
    if (token == '{') {
        braces = 1;
        next();
        if (token == '}')
            return error();
    }
    if (character_array(t) && token == 145) {
        join_strings();
        i = joined_string_length;
        if (character_array(t) != (joined_string_wide ? 2 : 1))
            return error();
        i++;
    } else if (complex_component(t)) {
        initializer_scalar_shape();
    } else if (is_record(t)) {
        type_size(t);
        n = (unqualified(t) - 0x20000e) / 256;
        node = named_member(record_first[n]);
        while ((node || (braces && token == '.')) && !failed && token != '}') {
            if ((braces || designated) && token == '.')
                node = initializer_member(t);
            if (failed)
                break;
            designated = 0;
            initializer_shape(member_type[node]);
            node = record_kind[n] == 2 ? 0 : named_member(member_next[node]);
            if ((braces || node) && initializer_more(braces))
                next();
            else
                break;
        }
    } else if (is_array(t)) {
        if (!braces && initializer_depth == 1)
            return error();
        n = array_length[(t - 4096) / 256];
        element = array_base[(t - 4096) / 256];
        while (!failed && (!n || i < n || (braces && token == '['))) {
            if (token == '}')
                break;
            if ((braces || designated) && token == '[')
                i = initializer_index(n);
            designated = 0;
            initializer_shape(element);
            i++;
            if (i > maximum)
                maximum = i;
            if ((braces || !n || i < n) && initializer_more(braces))
                next();
            else
                break;
        }
    } else if (token == '{')
        initializer_shape(t);
    else
        initializer_scalar_shape();
    if (braces) {
        if (token == ',')
            next();
        expect('}');
    }
    initializer_depth--;
    return maximum > i ? maximum : i;
}

int infer_initializer(int t, int id, int global) {
    int saved_cursor = cursor, saved_line = line, saved_strings = string_used, n, element = t;
    while (is_array(element))
        element = array_base[(element - 4096) / 256];
    if (!global && is_record(element))
        return infer_aggregate_array(t, id);
    expect('=');
    n = initializer_shape(t);
    cursor = saved_cursor;
    line = saved_line;
    string_used = saved_strings;
    token = '=';
    if (failed || !n)
        return error();
    return make_array(array_base[(t - 4096) / 256], n);
}

int copy_string(int t, int base, int offset, int global) {
    int n = array_length[(t - 4096) / 256], bytes = joined_string_length + 1;
    if (character_array(t) != (joined_string_wide ? 2 : 1))
        return error();
    if (joined_string_length > n)
        return error();
    if (bytes > n)
        bytes = n;
    bytes *= joined_string_wide ? 2 : 1;
    emit_string();
    emit(0x56);
    emit(0x57);
    sequence(0xc68948, 3);
    if (global) {
        sequence(0x3d8d48, 3);
        relative(base + offset);
    } else {
        sequence(0xbd8d48, 3);
        emit32(offset - base);
    }
    emit(0xb9);
    emit32(bytes);
    emit(0xfc);
    sequence(0xa4f3, 2);
    emit(0x5f);
    emit(0x5e);
    return 0;
}

int zero_local(int offset, int bytes) {
    emit(0x57);
    sequence(0xbd8d48, 3);
    emit32(-offset);
    sequence(0xc031, 2);
    emit(0xb9);
    emit32(bytes);
    emit(0xfc);
    sequence(0xaaf3, 2);
    emit(0x5f);
    return 0;
}

unsigned long long ce_value;
int ce_type;
int ce_constant_ok;
int ce_depth;

unsigned long long ce_normalize(unsigned long long x, int t) {
    t = unqualified(t);
    if (t == 12)
        return x != 0;
    if (t == 1 || t == 3 || is_enum(t))
        return (int)x;
    if (t == 2 || t == 4)
        return (unsigned int)x;
    if (t == 5 || t == 9)
        return (signed char)x;
    if (t == 6)
        return (unsigned char)x;
    if (t == 7)
        return (short)x;
    if (t == 8)
        return (unsigned short)x;
    return x;
}

int ce_expression(int minimum, int active);
int ce_inner(int minimum, int active);

int ce_unary(int active) {
    int op = token, t, saved_position, saved_calls;
    unsigned long long sign;
    if (failed)
        return 0;
    ce_depth++;
    if (ce_depth > 128)
        return error();
    if (op == 299) {
        floating_bits_constant();
        ce_type = fp_constant_type;
        ce_value = fp_constant_bits;
    } else if (op == 144) {
        ce_type = literal_type;
        ce_value = ce_normalize(
            ((unsigned long long)(unsigned int)literal_high << 32) | (unsigned int)value, ce_type);
        next();
    } else if (op >= 256 && enum_visible(op - 256)) {
        ce_type = pp_condition ? 10 : 1;
        ce_value = (long long)enum_value(op - 256);
        next();
    } else if (op == 296) {
        ce_value = (unsigned int)offset_expression();
        ce_type = 11;
    } else if (op == 283) {
        saved_position = position;
        saved_calls = call_count;
        unary();
        if (expression_ice != 1)
            return error_message("sizeof VLA is not an integer constant expression");
        ce_value = sizeof_value;
        ce_type = 11;
        rewind_code(saved_position);
        call_count = saved_calls;
    } else if (op == '(') {
        next();
        if (type_start(token)) {
            t = declarator(specifiers(), 1);
            expect(')');
            if (!t || floating_type(t) || is_pointer(t) || is_array(t) || is_record(t) ||
                is_function(t))
                return error();
            type_size(t);
            ce_unary(active);
            if (floating_type(ce_type)) {
                if (fpe_convert(ce_type, is_enum(t) ? 1 : unqualified(t), ce_value)) {
                    if (active)
                        return error_message("floating constant conversion out of range");
                    fpe_bits = 0;
                }
                ce_value = fpe_bits;
            }
            ce_value = ce_normalize(ce_value, t);
            ce_type = t;
        } else {
            ce_sequence(active);
            expect(')');
        }
    } else if (op == '+' || op == '-' || op == '~' || op == '!') {
        next();
        ce_unary(active);
        if (floating_type(ce_type))
            return error_message("floating operand is not immediately cast");
        ce_type = promoted(ce_type);
        sign = 1ULL << (type_size(ce_type) * 8 - 1);
        if (op == '-') {
            if (active && !unsigned_type(ce_type) && ce_value == ce_normalize(sign, ce_type))
                return error();
            ce_value = 0ULL - ce_value;
        }
        if (op == '~')
            ce_value = ~ce_value;
        if (op == '!') {
            ce_value = !ce_value;
            ce_type = pp_condition ? 10 : 1;
        }
        ce_value = ce_normalize(ce_value, ce_type);
    } else
        return error();
    ce_depth--;
    return 0;
}

int ce_invalid(int active) {
    ce_constant_ok = 0;
    if (active < 0)
        return 0;
    return error();
}

int ce_binary(int op, unsigned long long a, unsigned long long b, int t, int active) {
    unsigned long long result = 0, sign = 1ULL << (type_size(t) * 8 - 1), x, y, limit;
    int negative, comparison = 0;
    ce_constant_ok = 1;
    a = ce_normalize(a, t);
    b = ce_normalize(b, t);
    if (op == '+') {
        result = a + b;
        if (active && !unsigned_type(t) && ((~(a ^ b) & (a ^ result)) & sign))
            return ce_invalid(active);
    } else if (op == '-') {
        result = a - b;
        if (active && !unsigned_type(t) && (((a ^ b) & (a ^ result)) & sign))
            return ce_invalid(active);
    } else if (op == '*') {
        if (active && !unsigned_type(t)) {
            negative = ((a & sign) != 0) ^ ((b & sign) != 0);
            x = (a & sign) ? 0ULL - a : a;
            y = (b & sign) ? 0ULL - b : b;
            limit = negative ? sign : sign - 1;
            if (y && x > limit / y)
                return ce_invalid(active);
        }
        result = a * b;
    } else if (op == '/' || op == '%') {
        if (!active)
            result = 0;
        else {
            if (!b)
                return ce_invalid(active);
            if (!unsigned_type(t) && a == ce_normalize(sign, t) && b == ~0ULL)
                return ce_invalid(active);
            if (unsigned_type(t)) {
                if (op == '/')
                    result = a / b;
                else
                    result = a % b;
            } else {
                if (op == '/')
                    result = (long long)a / (long long)b;
                else
                    result = (long long)a % (long long)b;
            }
        }
    } else if (op == '&')
        result = a & b;
    else if (op == '|')
        result = a | b;
    else if (op == '^')
        result = a ^ b;
    else if (op == 128) {
        result = a == b;
        comparison = 1;
    } else if (op == 129) {
        result = a != b;
        comparison = 1;
    } else {
        comparison = 1;
        if (unsigned_type(t)) {
            if (op == '<')
                result = a < b;
            if (op == '>')
                result = a > b;
            if (op == 130)
                result = a <= b;
            if (op == 131)
                result = a >= b;
        } else {
            if (op == '<')
                result = (long long)a < (long long)b;
            if (op == '>')
                result = (long long)a > (long long)b;
            if (op == 130)
                result = (long long)a <= (long long)b;
            if (op == 131)
                result = (long long)a >= (long long)b;
        }
    }
    ce_type = comparison ? (pp_condition ? 10 : 1) : t;
    ce_value = ce_normalize(result, ce_type);
    return 0;
}

/* Classify a runtime expression without suppressing code or publishing a diagnostic. */
int ice_set(int kind, unsigned long long bits) {
    expression_ice = kind;
    expression_value = bits;
    expression_zero = kind == 1 && !bits;
    return 0;
}

int ice_convert(int t) {
    if (!t || floating_type(t) || expression_ice == 4 || is_pointer(t) || is_record(t) ||
        is_array(t) || is_function(t))
        return ice_set(0, 0);
    return ice_set(expression_ice, ce_normalize(expression_value, t));
}

int ice_unary(int op, int kind, unsigned long long bits, int t) {
    unsigned long long sign;
    if (!kind || kind == 4 || is_pointer(t) || floating_type(t))
        return ice_set(0, 0);
    if (kind != 1)
        return ice_set(2, 0);
    bits = ce_normalize(bits, t);
    sign = 1ULL << (type_size(t) * 8 - 1);
    if (op == '-') {
        if (!unsigned_type(t) && bits == ce_normalize(sign, t))
            return ice_set(2, 0);
        bits = 0ULL - bits;
    }
    if (op == '~')
        bits = ~bits;
    if (op == '!')
        bits = !bits;
    return ice_set(1, ce_normalize(bits, op == '!' ? 1 : t));
}

int ice_binary(int op, int ak, unsigned long long a, int at, int bk, unsigned long long b, int bt) {
    int t, width, old_type = ce_type, valid;
    unsigned long long sign, old_value = ce_value, bits;
    if (!ak || !bk || ak == 4 || bk == 4 || is_pointer(at) || is_pointer(bt) || floating_type(at) ||
        floating_type(bt))
        return ice_set(0, 0);
    if (ak != 1 || bk != 1)
        return ice_set(2, 0);
    if (op == 132 || op == 133) {
        t = promoted(at);
        width = type_size(t) * 8;
        a = ce_normalize(a, t);
        b = ce_normalize(b, promoted(bt));
        sign = 1ULL << (width - 1);
        if (b >= (unsigned int)width)
            return ice_set(2, 0);
        if (op == 132) {
            if (!c89_identifiers && !unsigned_type(t) && ((a & sign) || a > ((sign - 1) >> b)))
                return ice_set(2, 0);
            bits = a << b;
        } else if (unsigned_type(t))
            bits = a >> b;
        else
            bits = (long long)a >> b;
        return ice_set(1, ce_normalize(bits, t));
    }
    t = common_type(at, bt);
    ce_binary(op, a, b, t, -1);
    valid = ce_constant_ok;
    bits = ce_value;
    ce_type = old_type;
    ce_value = old_value;
    return ice_set(valid ? 1 : 2, bits);
}

int ice_logical(int op, int ak, unsigned long long a, int bk, unsigned long long b) {
    if (!ak || !bk || ak == 4 || bk == 4)
        return ice_set(0, 0);
    if (ak != 1)
        return ice_set(2, 0);
    if (op == 134 && !a)
        return ice_set(1, 0);
    if (op == 135 && a)
        return ice_set(1, 1);
    if (bk != 1)
        return ice_set(2, 0);
    return ice_set(1, b != 0);
}

int ice_conditional(int ck, unsigned long long c, int ak, unsigned long long a, int bk,
                    unsigned long long b, int t) {
    if (!ck || !ak || !bk || ck == 4 || ak == 4 || bk == 4 || !t || is_pointer(t) || is_record(t) ||
        floating_type(t))
        return ice_set(0, 0);
    if (ck != 1 || (c ? ak : bk) != 1)
        return ice_set(2, 0);
    return ice_set(1, ce_normalize(c ? a : b, t));
}

int ce_sequence(int active) {
    int comma = 0;
    ce_inner(2, active);
    while (!failed && token == ',') {
        if (floating_type(ce_type))
            return error_message("floating operand is not immediately cast");
        if (active)
            return error_message("evaluated comma is not constant expression");
        c89_extension("unevaluated comma in constant expression");
        comma = 1;
        next();
        ce_inner(2, 0);
    }
    if (comma && floating_type(ce_type))
        return error_message("floating operand is not immediately cast");
    return 0;
}

int ce_expression(int minimum, int active) {
    ce_inner(minimum, active);
    if (floating_type(ce_type))
        return error_message("floating operand in integer constant expression");
    return 0;
}

int ce_inner(int minimum, int active) {
    int op, prec, left_type, middle_type, selected, width, t;
    unsigned long long left, middle, sign;
    if (failed)
        return 0;
    ce_depth++;
    if (ce_depth > 128)
        return error();
    ce_unary(active);
    while (!failed) {
        op = token;
        prec = precedence(op);
        if (prec < minimum)
            break;
        if (floating_type(ce_type))
            return error_message("floating operand is not immediately cast");
        left = ce_value;
        left_type = ce_type;
        next();
        if (op == '?') {
            selected = left != 0;
            ce_sequence(active && selected);
            middle = ce_value;
            middle_type = ce_type;
            expect(':');
            ce_expression(2, active && !selected);
            if (floating_type(middle_type))
                return error_message("floating operand is not immediately cast");
            t = common_type(middle_type, ce_type);
            ce_value = ce_normalize(selected ? middle : ce_value, t);
            ce_type = t;
        } else {
            selected = active;
            if (op == 134 && !left)
                selected = 0;
            if (op == 135 && left)
                selected = 0;
            ce_expression(prec + 1, selected);
            if (op == 134 || op == 135) {
                if (op == 134)
                    ce_value = (left != 0) && (ce_value != 0);
                else
                    ce_value = (left != 0) || (ce_value != 0);
                ce_type = pp_condition ? 10 : 1;
            } else if (op == 132 || op == 133) {
                t = promoted(left_type);
                width = type_size(t) * 8;
                left = ce_normalize(left, t);
                sign = 1ULL << (width - 1);
                if (active && ce_value >= (unsigned int)width)
                    return error();
                if (!active)
                    ce_value = 0;
                if (op == 132) {
                    if (active && !c89_identifiers && !unsigned_type(t) &&
                        ((left & sign) || left > ((sign - 1) >> ce_value)))
                        return error();
                    ce_value = left << ce_value;
                } else if (unsigned_type(t))
                    ce_value = left >> ce_value;
                else
                    ce_value = (long long)left >> ce_value;
                ce_type = t;
                ce_value = ce_normalize(ce_value, t);
            } else
                ce_binary(op, left, ce_value, common_type(left_type, ce_type), active);
        }
    }
    ce_depth--;
    return 0;
}

int constant_bound(void) {
    ce_expression(2, 1);
    if (!ce_value || ce_value > 0x1a00000ULL)
        return error();
    return (int)ce_value;
}

/* Implementation expression used by math.h for constant IEEE encodings.
   It produces a floating constant, never an address or a memory load. */
int floating_bits_constant(void) {
    int t, saved_storage = decl_storage, saved_typedef = decl_typedef, saved_inline = decl_inline;
    unsigned long long bits;
    next();
    expect('(');
    t = unqualified(specifiers());
    if (decl_storage || decl_inline)
        return error_message("__fpbits requires a type without declaration specifiers");
    if (!floating_type(t))
        return error_message("__fpbits requires floating type");
    expect(',');
    ce_expression(2, 1);
    bits = ce_value;
    expect(')');
    if (t == 13 && bits > 0xffffffffULL)
        return error_message("__fpbits float encoding exceeds 32 bits");
    fp_constant_type = t;
    fp_constant_bits = bits;
    decl_storage = saved_storage;
    decl_typedef = saved_typedef;
    decl_inline = saved_inline;
    return 0;
}

int enum_visible(int id) {
    if (alias_type[id])
        return 0;
    if (local_offset[id])
        return local_offset[id] == -3;
    return enum_global[id];
}

int enum_value(int id) {
    if (local_offset[id] == -3)
        return local_static_address[id];
    return enum_global_value[id];
}

int enum_binding(int id, int v) {
    if (!block_depth) {
        if (global_visible[id] || file_function_type[id] || alias_type[id] || enum_global[id])
            return error();
        enum_global[id] = 1;
        enum_global_value[id] = v;
        return 0;
    }
    if ((local_offset[id] || alias_type[id]) && local_depth[id] == block_depth)
        return error();
    if (local_count == 256)
        return error();
    binding_name[local_count] = id;
    binding_offset[local_count] = local_offset[id];
    binding_depth[local_count] = local_depth[id];
    binding_type[local_count] = local_type[id];
    binding_bytes[local_count] = local_bytes;
    binding_alias[local_count] = alias_type[id];
    binding_register[local_count] = register_object[id];
    binding_static_address[local_count] = local_static_address[id];
    local_count++;
    local_offset[id] = -3;
    local_type[id] = 1;
    local_depth[id] = block_depth;
    alias_type[id] = 0;
    register_object[id] = 0;
    local_static_address[id] = v;
    return 0;
}

int restore_tags(int saved) {
    int id;
    while (tag_count > saved) {
        tag_count--;
        id = tag_binding_name[tag_count];
        tag_type[id] = tag_binding_type[tag_count];
        tag_depth[id] = tag_binding_depth[tag_count];
    }
    return 0;
}

int enum_specifier(void) {
    int tag = -1, t, id, index;
    long long v = 0;
    next();
    if (token != '{') {
        tag = name();
        next();
    }
    if (token != '{') {
        if (tag < 0 || !is_enum(tag_type[tag]))
            return error();
        return tag_type[tag];
    }
    if (enum_count == 1024)
        return error();
    index = enum_count++;
    t = 0x10000d + index * 256;
    if (tag >= 0) {
        if (tag_type[tag] && tag_depth[tag] == block_depth)
            return error();
        if (block_depth) {
            if (tag_count == 1024)
                return error();
            tag_binding_name[tag_count] = tag;
            tag_binding_type[tag_count] = tag_type[tag];
            tag_binding_depth[tag_count] = tag_depth[tag];
            tag_count++;
        }
        tag_type[tag] = t;
        tag_depth[tag] = block_depth;
    }
    next();
    if (token == '}')
        return error();
    while (!failed) {
        id = name();
        next();
        if (token == '=') {
            next();
            ce_expression(2, 1);
            if (unsigned_type(ce_type) && ce_value > 2147483647ULL)
                return error();
            v = (long long)ce_value;
        }
        if (v < -2147483647LL - 1 || v > 2147483647LL)
            return error();
        enum_binding(id, (int)v);
        v++;
        if (token != ',')
            break;
        next();
        if (token == '}') {
            c89_extension("trailing comma in enumeration");
            break;
        }
    }
    expect('}');
    enum_complete[index] = 1;
    return t;
}

int record_specifier(void) {
    int kind = token == 285 ? 1 : 2, tag = -1, t = 0, index, base, mt, id, node, last;
    int width, align, offset = 0, total = 0, maximum = 1, create = 1, flexible = 0, bits, shift,
                      unit = 0, used = 0, start = 0, previous = 0, named = 0;
    record_depth++;
    if (record_depth > 64)
        return error();
    next();
    if (token != '{') {
        tag = name();
        next();
    }
    if (tag >= 0 && tag_type[tag]) {
        if (token != '{' && !(token == ';' && tag_depth[tag] != block_depth))
            create = 0;
        if (tag_depth[tag] == block_depth)
            create = 0;
        if (!create) {
            t = tag_type[tag];
            if (!is_record(t))
                return error();
            index = (unqualified(t) - 0x20000e) / 256;
            if (record_kind[index] != kind)
                return error();
        }
    }
    if (create) {
        if (record_count == 1024)
            return error();
        index = record_count++;
        t = 0x20000e + index * 256;
        record_kind[index] = kind;
        if (tag >= 0) {
            if (block_depth) {
                if (tag_count == 1024)
                    return error();
                tag_binding_name[tag_count] = tag;
                tag_binding_type[tag_count] = tag_type[tag];
                tag_binding_depth[tag_count] = tag_depth[tag];
                tag_count++;
            }
            tag_type[tag] = t;
            tag_depth[tag] = block_depth;
        }
    }
    if (token != '{') {
        record_depth--;
        return t;
    }
    if (record_size[index] || record_open[index])
        return error();
    record_open[index] = 1;
    next();
    while (!failed && token != '}') {
        if (flexible)
            return error_message("flexible array member must be last");
        base = specifiers();
        if (decl_storage || decl_inline)
            return error();
        while (!failed) {
            bits = 0;
            shift = 0;
            if (token == ':') {
                mt = base;
                id = -1;
            } else {
                mt = declarator(base, 0);
                id = decl_name;
            }
            if (variably_modified_type(mt))
                return error_message("structure or union member has variably modified type");
            if (token == '=' || token == '(')
                return error();
            if (token == ':') {
                width = unqualified(mt);
                if (width != 1 && width != 2 && width != 12)
                    return error_message("unsupported bit-field base type");
                next();
                ce_expression(2, 1);
                if (ce_value > (unsigned int)(width == 12 ? 1 : 32))
                    return error_message("invalid bit-field width");
                bits = (int)ce_value;
                if (!bits && id >= 0)
                    return error_message("named bit-field has zero width");
                width = type_size(mt);
                align = type_align(mt);
                if (!bits) {
                    if (previous) {
                        if (align > maximum)
                            maximum = align;
                        if (kind == 1)
                            total = ((total + align - 1) / align) * align;
                    }
                    unit = 0;
                    used = 0;
                    previous = 0;
                    goto member_done;
                }
            } else {
                if (id < 0)
                    return error();
                if (kind == 1 && record_has_flexible(mt))
                    return error_message("structure member contains flexible array member");
                if (incomplete_array(mt)) {
                    if (kind != 1 || !named)
                        return error_message(
                            "flexible array requires preceding named structure member");
                    c89_extension("flexible array member");
                    flexible = 1;
                    width = 0;
                } else
                    width = type_size(mt);
                align = type_align(mt);
            }
            if (failed)
                return 0;
            if (id >= 0) {
                for (node = record_first[index]; node; node = member_next[node])
                    if (member_name[node] == id)
                        return error();
                named++;
            }
            if (member_count == 8191)
                return error();
            node = ++member_count;
            member_name[node] = id;
            member_type[node] = mt;
            if (align > maximum)
                maximum = align;
            if (kind == 1) {
                if (bits && unit == width && used + bits <= width * 8) {
                    offset = start;
                    shift = used;
                    used += bits;
                } else {
                    offset = ((total + align - 1) / align) * align;
                    if (width > 0x1a00000 - offset)
                        return error();
                    total = offset + width;
                    if (bits) {
                        unit = width;
                        used = bits;
                        start = offset;
                    }
                }
            } else {
                offset = 0;
                if (width > total)
                    total = width;
            }
            if (!bits) {
                unit = 0;
                used = 0;
            }
            previous = bits != 0;
            member_offset[node] = offset;
            member_bits[node] = bits;
            member_shift[node] = shift;
            last = record_last[index];
            if (last)
                member_next[last] = node;
            else
                record_first[index] = node;
            record_last[index] = node;
        member_done:
            if (token != ',')
                break;
            if (flexible)
                return error_message("flexible array member must be last");
            next();
        }
        expect(';');
    }
    if (!named)
        return error_message("record requires named member");
    expect('}');
    record_size[index] = ((total + maximum - 1) / maximum) * maximum;
    record_align[index] = maximum;
    record_open[index] = 0;
    record_depth--;
    return t;
}

int record_value(int t) {
    expression_type = t;
    expression_zero = 0;
    expression_ice = 0;
    expression_load = position;
    expression_record_ready = 0;
    expression_bit = 0;
    expression_array_type = 0;
    expression_array_bytes = 0;
    return 0;
}

int named_member(int node) {
    while (node && member_name[node] < 0)
        node = member_next[node];
    return node;
}

int find_member(int t, int id) {
    int node;
    if (!is_record(t))
        return error();
    type_size(t);
    for (node = record_first[(unqualified(t) - 0x20000e) / 256]; node; node = member_next[node]) {
        if (member_name[node] == id)
            return node;
    }
    return error();
}

int offset_expression(void) {
    int t, node, width, offset = 0;
    unsigned long long index;
    next();
    expect('(');
    t = declarator(specifiers(), 1);
    type_size(t);
    expect(',');
    node = find_member(t, name());
    next();
    if (failed)
        return 0;
    if (member_bits[node])
        return error_message("offsetof cannot name bit-field");
    offset = member_offset[node];
    t = member_type[node];
    while (!failed) {
        if (token == '.') {
            next();
            node = find_member(t, name());
            next();
            if (failed)
                return 0;
            if (member_bits[node])
                return error_message("offsetof cannot name bit-field");
            offset += member_offset[node];
            t = member_type[node];
        } else if (token == '[') {
            if (!is_array(t))
                return error_message("offsetof subscript requires array member");
            next();
            ce_expression(2, 1);
            index = ce_value;
            expect(']');
            if (failed)
                return 0;
            t = array_base[(t - 4096) / 256];
            width = type_size(t);
            if (failed || !width)
                return 0;
            if (index > (unsigned int)(0x1a00000 - offset) / (unsigned int)width)
                return error_message("offsetof subscript exceeds object capacity");
            offset += (int)index * width;
        } else
            break;
    }
    expect(')');
    return offset;
}

int member_access(int left, int arrow) {
    int t = expression_type, node, reg = expression_register, q;
    if (arrow) {
        if (!is_pointer(t))
            return error();
        t = pointed_type(t);
        reg = 0;
    }
    next();
    node = find_member(t, name());
    next();
    if (failed)
        return 0;
    if (member_offset[node]) {
        sequence(0x0548, 2);
        emit32(member_offset[node]);
    }
    q = type_qualifiers(t);
    t = qualify_type(member_type[node], q);
    if (reg && is_array(t) && !unevaluated_depth)
        return error();
    if (member_bits[node])
        bitfield_load(node, type_qualifiers(t));
    else
        indirect_value(t);
    expression_register = reg;
    return arrow || left ? -1 : 0;
}

int bitfield_load(int node, int q) {
    int n = member_bits[node], shift = member_shift[node], base = unqualified(member_type[node]),
        saved;
    sequence(0xc28948, 3);
    saved = position;
    load_opcode(base == 12 ? 6 : 2, 0);
    if (shift) {
        sequence(0xe8c1, 2);
        emit(shift);
    }
    if (n < 32) {
        sequence(0xe0c1, 2);
        emit(32 - n);
        sequence(base == 1 ? 0xf8c1 : 0xe8c1, 2);
        emit(32 - n);
    }
    expression_type = qualify_type(0x50000a + node * 256, q);
    expression_load = saved;
    expression_bit = node;
    expression_zero = 0;
    expression_ice = 0;
    return 0;
}

int bitfield_store(int node) {
    int n = member_bits[node], shift = member_shift[node], base = unqualified(member_type[node]);
    unsigned int mask;
    convert(base);
    if (n == 32) {
        emit(0x59);
        sequence(0x0189, 2);
    } else {
        mask = (1U << n) - 1;
        sequence(0xe0c1, 2);
        emit(32 - n);
        sequence(base == 1 ? 0xf8c1 : 0xe8c1, 2);
        emit(32 - n);
        sequence(0xc28941, 3);
        emit(0x25);
        emit32(mask);
        if (shift) {
            sequence(0xe0c1, 2);
            emit(shift);
        }
        emit(0x59);
        if (base == 12)
            sequence(0x11b60f, 3);
        else
            sequence(0x118b, 2);
        sequence(0xe281, 2);
        emit32(~(mask << shift));
        sequence(0xc209, 2);
        sequence(base == 12 ? 0x1188 : 0x1189, 2);
        sequence(0xd08944, 3);
    }
    expression_type = 0x50000a + node * 256;
    expression_bit = 0;
    expression_zero = 0;
    expression_ice = 0;
    return 0;
}

int store_record(int left, int t) {
    int n = type_size(t);
    if (left == -1)
        emit(0x59);
    emit(0x56);
    emit(0x57);
    sequence(0xc68948, 3);
    if (left == -1)
        sequence(0xcf8948, 3);
    else if (left < -1) {
        sequence(0x3d8d48, 3);
        object_reference(-left - 2);
    } else {
        sequence(0xbd8d48, 3);
        emit32(-left);
    }
    emit(0xb9);
    emit32(n);
    emit(0xfc);
    sequence(0xa4f3, 2);
    emit(0x5f);
    emit(0x5e);
    return 0;
}

/* Materialize a volatile aggregate only when its value is consumed.
   Member selection, address formation and assignment destinations stay lazy. */
int record_read(void) {
    int t = expression_type, copy;
    if (failed || !is_record(t) || expression_record_ready || !contains_volatile(t))
        return 0;
    copy = temporary(type_size(t), type_align(t));
    store_record(copy, t);
    sequence(0x858d48, 3);
    emit32(-copy);
    record_value(unqualified(t));
    expression_record_ready = 1;
    return 0;
}

int record_indirect(int t) {
    int n;
    if (!is_record(t))
        return 0;
    n = type_size(t);
    return n != 1 && n != 2 && n != 4 && n != 8;
}

int record_word(int t) {
    int n = type_size(t);
    if (n == 1)
        return 6;
    if (n == 2)
        return 8;
    if (n == 4)
        return 2;
    if (n == 8)
        return 11;
    return error();
}

int temporary(int bytes, int align) {
    if (unevaluated_depth)
        return 16;
    if (bytes > 65536 - local_bytes)
        return error();
    local_bytes = ((local_bytes + bytes + align - 1) / align) * align;
    if (local_bytes > 65536)
        return error();
    if (local_bytes > frame_peak)
        frame_peak = local_bytes;
    return local_bytes;
}

int aggregate_parameters(int n, int hidden) {
    int i, spill = temporary((n + hidden) * 8, 8), offset, t;
    return_address_offset = spill;
    for (i = 0; i < n + hidden; i++) {
        offset = spill - i * 8;
        t = 0;
        if (i >= hidden) {
            t = parameter_decl_type[i - hidden];
            if (current_oldstyle)
                t = default_promoted(t);
        }
        if (i < 4 && floating_type(t)) {
            emit(unqualified(t) == 13 ? 0xf3 : 0xf2);
            sequence(0x110f, 2);
            emit(0x85 + i * 8);
            emit32(-offset);
            continue;
        }
        if (i == 0)
            sequence(0x8d8948, 3);
        else if (i == 1)
            sequence(0x958948, 3);
        else if (i == 2)
            sequence(0x85894c, 3);
        else if (i == 3)
            sequence(0x8d894c, 3);
        else {
            sequence(0x858b48, 3);
            emit32(16 + i * 8);
            sequence(0x858948, 3);
        }
        emit32(-offset);
    }
    for (i = 0; i < n; i++) {
        t = parameter_decl_type[i];
        sequence(0x858b48, 3);
        emit32(-(spill - (i + hidden) * 8));
        if (record_indirect(t))
            store_record(parameter_offset[i], t);
        else {
            if (is_record(t))
                t = record_word(t);
            else {
                expression_type = current_oldstyle ? default_promoted(t) : promoted(t);
                convert(t);
            }
            store(parameter_offset[i], t);
        }
    }
    return 0;
}

int switch_label(void) {
    int node;
    unsigned long long value_bits;
    if (!switch_depth)
        return error();
    if (vm_current != switch_vm)
        return error_message("switch label enters scope of variably modified identifier");
    if (token == 272) {
        if (switch_default)
            return error();
        next();
        expect(':');
        switch_default = position;
    } else {
        next();
        ce_expression(2, 1);
        expect(':');
        value_bits = ce_normalize(ce_value, switch_type);
        for (node = switch_head; node; node = case_next[node - 1]) {
            if (case_value[node - 1] == value_bits)
                return error();
        }
        if (case_count == 2048)
            return error();
        node = case_count;
        case_count++;
        case_value[node] = value_bits;
        case_address[node] = position;
        case_next[node] = switch_head;
        switch_head = node + 1;
    }
    return statement();
}

int switch_statement(void) {
    int old_head = switch_head, old_default = switch_default, old_type = switch_type,
        old_vm = switch_vm;
    int old_break = break_head, saved_break_locals = break_local_count, saved_cases = case_count,
        dispatch, end, branch, node;
    unsigned long long bits;
    next();
    expect('(');
    full_expression();
    expect(')');
    if (!expression_type || is_pointer(expression_type) || is_record(expression_type) ||
        floating_type(expression_type))
        return error();
    switch_type = promoted(expression_type);
    convert(switch_type);
    if (!wide_type(switch_type)) {
        if (unsigned_type(switch_type))
            sequence(0xc089, 2);
        else
            sequence(0xc06348, 3);
    }
    dispatch = jump(0);
    switch_vm = vm_current;
    switch_head = 0;
    switch_default = 0;
    break_head = 0;
    break_local_count = local_count;
    switch_depth++;
    statement();
    end = jump(0);
    patch(dispatch, position);
    sequence(0xc28949, 3);
    for (node = switch_head; node && !failed; node = case_next[node - 1]) {
        bits = case_value[node - 1];
        sequence(0xb848, 2);
        emit32((int)bits);
        emit32((int)(bits >> 32));
        sequence(0xc23949, 3);
        branch = jump(0x84);
        patch(branch, case_address[node - 1]);
    }
    if (switch_default) {
        branch = jump(0);
        patch(branch, switch_default);
    }
    patch(end, position);
    finish_breaks(break_head, position);
    switch_vm = old_vm;
    switch_head = old_head;
    switch_default = old_default;
    switch_type = old_type;
    break_head = old_break;
    break_local_count = saved_break_locals;
    case_count = saved_cases;
    switch_depth--;
    return 0;
}

/* Static constants keep relocatable object identity separate from the offset.
   kind: 0 integer, 1 pointer value, 2 object designator (never loaded). */
int sc_kind;
int sc_null;
int sc_ice;
int sc_type;
int sc_base;
int sc_depth;
unsigned long long sc_bits;

unsigned long long sc_imaginary;

int sc_truth(int t, unsigned long long real, unsigned long long imaginary) {
    int p = complex_component(t);
    return p ? (fpe_truth(p, real) || fpe_truth(p, imaginary)) : fpe_truth(t, real);
}

int sc_complex_convert(int from, int to, unsigned long long bits, int active) {
    int a = complex_component(from), b = complex_component(to);
    unsigned long long imaginary = a ? sc_imaginary : 0, real;
    if (to == 12) {
        sc_bits = sc_truth(from, bits, imaginary);
        sc_imaginary = 0;
        sc_type = 12;
        return 0;
    }
    sc_convert(a ? a : from, b ? b : to, bits, active);
    real = sc_bits;
    if (b) {
        sc_convert(a ? a : b, b, imaginary, active);
        sc_imaginary = sc_bits;
    } else
        sc_imaginary = 0;
    sc_bits = real;
    sc_type = to;
    sc_ice = 0;
    return 0;
}

unsigned long long sc_complex_op(int t, int op, unsigned long long a, unsigned long long b,
                                 int active) {
    unsigned long long r = fpe_arithmetic(t, op, a, b);
    if (active && !fpe_finite(t, r) && fpe_finite(t, a) && fpe_finite(t, b))
        error_message("complex constant arithmetic out of range");
    return r;
}

int sc_complex_wide(int t, int op, unsigned long long a, unsigned long long b, unsigned long long c,
                    unsigned long long d, int active) {
    unsigned long long v[6];
    void (*leaf)(unsigned long long *);
    v[0] = a;
    v[1] = b;
    v[2] = c;
    v[3] = d;
    v[4] = 0;
    v[5] = 0;
    /* FNSAVE starts a clean, nearest-even, 64-significand-bit x87
       environment. FRSTOR restores the compiler's complete saved state;
       MXCSR is untouched. Only the final component stores narrow range. */
    if (op == '/' && t == 13)
        leaf = (void (*)(
            unsigned long long
                *)) "\x48\x81\xec\x80\x00\x00\x00\xdd\x34\x24"
                    "\xd9\x41\x10\xd8\x49\x10\xd9\x41\x18\xd8\x49\x18\xde\xc1\xdb\x7c\x24\x70"
                    "\xd9\x01\xd8\x49\x10\xd9\x41\x08\xd8\x49\x18\xde\xc1\xdb\x6c\x24\x70\xde\xf9\xd9\x59\x20"
                    "\xd9\x41\x08\xd8\x49\x10\xd9\x01\xd8\x49\x18\xde\xe9\xdb\x6c\x24\x70\xde\xf9\xd9\x59\x28"
                    "\xdd\x24\x24\x48\x81\xc4\x80\x00\x00\x00\xc3";
    else if (op == '/')
        leaf = (void (*)(
            unsigned long long
                *)) "\x48\x81\xec\x80\x00\x00\x00\xdd\x34\x24"
                    "\xdd\x41\x10\xdc\x49\x10\xdd\x41\x18\xdc\x49\x18\xde\xc1\xdb\x7c\x24\x70"
                    "\xdd\x01\xdc\x49\x10\xdd\x41\x08\xdc\x49\x18\xde\xc1\xdb\x6c\x24\x70\xde\xf9\xdd\x59\x20"
                    "\xdd\x41\x08\xdc\x49\x10\xdd\x01\xdc\x49\x18\xde\xe9\xdb\x6c\x24\x70\xde\xf9\xdd\x59\x28"
                    "\xdd\x24\x24\x48\x81\xc4\x80\x00\x00\x00\xc3";
    else if (t == 13)
        leaf = (void (*)(unsigned long long
                             *)) "\x48\x81\xec\x80\x00\x00\x00\xdd\x34\x24"
                                 "\xd9\x01\xd8\x49\x10\xd9\x41\x08\xd8\x49\x18\xde\xe9\xd9\x59\x20"
                                 "\xd9\x01\xd8\x49\x18\xd9\x41\x08\xd8\x49\x10\xde\xc1\xd9\x59\x28"
                                 "\xdd\x24\x24\x48\x81\xc4\x80\x00\x00\x00\xc3";
    else
        leaf = (void (*)(unsigned long long
                             *)) "\x48\x81\xec\x80\x00\x00\x00\xdd\x34\x24"
                                 "\xdd\x01\xdc\x49\x10\xdd\x41\x08\xdc\x49\x18\xde\xe9\xdd\x59\x20"
                                 "\xdd\x01\xdc\x49\x18\xdd\x41\x08\xdc\x49\x10\xde\xc1\xdd\x59\x28"
                                 "\xdd\x24\x24\x48\x81\xc4\x80\x00\x00\x00\xc3";
    leaf(v);
    if (active && (!fpe_finite(t, v[4]) || !fpe_finite(t, v[5])))
        error_message("complex constant arithmetic out of range");
    sc_bits = v[4];
    sc_imaginary = v[5];
    return 0;
}

int sc_complex_binary(int op, int lt, unsigned long long lr, unsigned long long li, int active) {
    int rt = sc_type, t, p, lc = complex_component(lt), rc = complex_component(rt), truth;
    unsigned long long rr = sc_bits, ri = sc_imaginary, x, y, den;
    if (op == 134 || op == 135) {
        truth = sc_truth(lt, lr, li);
        sc_bits = op == 134 ? (truth && sc_truth(rt, rr, ri)) : (truth || sc_truth(rt, rr, ri));
        sc_imaginary = 0;
        sc_type = 1;
        return 0;
    }
    if (op != '+' && op != '-' && op != '*' && op != '/' && op != 128 && op != 129)
        return error();
    t = complex_common(lt, rt);
    p = complex_component(t);
    if (failed)
        return 0;
    sc_convert(rt, t, rr, active);
    rr = sc_bits;
    ri = sc_imaginary;
    sc_imaginary = li;
    sc_convert(lt, t, lr, active);
    lr = sc_bits;
    li = sc_imaginary;
    if (op == 128 || op == 129) {
        truth = fpe_compare(p, lr, rr) == 0 && fpe_compare(p, li, ri) == 0;
        sc_bits = op == 128 ? truth : !truth;
        sc_imaginary = 0;
        sc_type = 1;
        return 0;
    }
    if (op == '+' || op == '-') {
        x = sc_complex_op(p, op, lr, rr, active);
        if (lc && rc)
            y = sc_complex_op(p, op, li, ri, active);
        else {
            y = lc ? li : ri;
            if (op == '-' && !lc)
                y ^= p == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
        }
    } else if (op == '*') {
        if (!lc) {
            x = sc_complex_op(p, '*', lr, rr, active);
            y = sc_complex_op(p, '*', lr, ri, active);
        } else if (!rc) {
            x = sc_complex_op(p, '*', lr, rr, active);
            y = sc_complex_op(p, '*', li, rr, active);
        } else if (fpe_finite(p, lr) && fpe_finite(p, li) && fpe_finite(p, rr) &&
                   fpe_finite(p, ri)) {
            sc_complex_wide(p, '*', lr, li, rr, ri, active);
            x = sc_bits;
            y = sc_imaginary;
        } else {
            x = sc_complex_op(p, '-', sc_complex_op(p, '*', lr, rr, active),
                              sc_complex_op(p, '*', li, ri, active), active);
            y = sc_complex_op(p, '+', sc_complex_op(p, '*', lr, ri, active),
                              sc_complex_op(p, '*', li, rr, active), active);
        }
    } else if (!fpe_truth(p, ri)) {
        x = sc_complex_op(p, '/', lr, rr, active);
        y = sc_complex_op(p, '/', li, rr, active);
    } else if (!fpe_truth(p, rr)) {
        x = sc_complex_op(p, '/', li, ri, active);
        y = sc_complex_op(p, '/', lr, ri, active);
        y ^= p == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
    } else if (fpe_finite(p, lr) && fpe_finite(p, li) && fpe_finite(p, rr) && fpe_finite(p, ri)) {
        sc_complex_wide(p, '/', lr, li, rr, ri, active);
        x = sc_bits;
        y = sc_imaginary;
    } else {
        den = sc_complex_op(p, '+', sc_complex_op(p, '*', rr, rr, active),
                            sc_complex_op(p, '*', ri, ri, active), active);
        x = sc_complex_op(p, '/',
                          sc_complex_op(p, '+', sc_complex_op(p, '*', lr, rr, active),
                                        sc_complex_op(p, '*', li, ri, active), active),
                          den, active);
        y = sc_complex_op(p, '/',
                          sc_complex_op(p, '-', sc_complex_op(p, '*', li, rr, active),
                                        sc_complex_op(p, '*', lr, ri, active), active),
                          den, active);
    }
    sc_bits = x;
    sc_imaginary = y;
    sc_type = t;
    return 0;
}

int sc_convert(int from, int to, unsigned long long bits, int active) {
    if (complex_component(from) || complex_component(to))
        return sc_complex_convert(from, to, bits, active);
    from = unqualified(from);
    to = unqualified(to);
    if (is_enum(from))
        from = 1;
    if (is_enum(to))
        to = 1;
    if (!from || !to || from > 15 || to > 15)
        return error();
    if (fpe_convert(from, to, bits)) {
        if (active)
            return error_message("floating constant conversion out of range");
        fpe_bits = 0;
    }
    sc_bits = fpe_bits;
    sc_type = to;
    return 0;
}

int sc_floating_binary(int op, int left_type, unsigned long long left, int active) {
    int t, cmp;
    unsigned long long right;
    if (op == 134 || op == 135) {
        if (op == 134)
            sc_bits = fpe_truth(left_type, left) && fpe_truth(sc_type, sc_bits);
        else
            sc_bits = fpe_truth(left_type, left) || fpe_truth(sc_type, sc_bits);
        sc_type = 1;
        return 0;
    }
    if (op != '+' && op != '-' && op != '*' && op != '/' && op != '<' && op != '>' && op != 128 &&
        op != 129 && op != 130 && op != 131)
        return error();
    t = common_type(left_type, sc_type);
    sc_convert(sc_type, t, sc_bits, active);
    right = sc_bits;
    sc_convert(left_type, t, left, active);
    left = sc_bits;
    if (op == '+' || op == '-' || op == '*' || op == '/') {
        sc_bits = fpe_arithmetic(t, op, left, right);
        sc_type = t;
        if (!fpe_finite(t, sc_bits) && fpe_finite(t, left) && fpe_finite(t, right)) {
            if (active)
                return error_message("floating constant arithmetic out of range");
            sc_bits = 0;
        }
    } else {
        cmp = fpe_compare(t, left, right);
        sc_type = 1;
        if (op == 128)
            sc_bits = cmp == 0;
        if (op == 129)
            sc_bits = cmp != 0;
        if (op == '<')
            sc_bits = cmp == -1;
        if (op == '>')
            sc_bits = cmp == 1;
        if (op == 130)
            sc_bits = cmp == -1 || cmp == 0;
        if (op == 131)
            sc_bits = cmp == 1 || cmp == 0;
    }
    return 0;
}

int sc_decay(void) {
    if (sc_kind == 2) {
        if (is_function(sc_type))
            sc_type = make_pointer(sc_type);
        else {
            if (!is_array(sc_type))
                return error();
            sc_type = make_pointer(array_base[(sc_type - 4096) / 256]);
        }
        sc_kind = 1;
    }
    return 0;
}

int sc_add(int kind, int t, int base, unsigned long long bits, int subtract, int left_ice) {
    int width;
    sc_decay();
    if (!kind && sc_kind == 1 && !subtract) {
        if (left_ice != 1)
            return error_message("address offset requires integer constant expression");
        width = type_size(pointed_type(sc_type));
        sc_bits += bits * (unsigned int)width;
        return 0;
    }
    if (kind != 1 || sc_kind)
        return error();
    if (sc_ice != 1)
        return error_message("address offset requires integer constant expression");
    width = type_size(pointed_type(t));
    if (subtract)
        bits -= sc_bits * (unsigned int)width;
    else
        bits += sc_bits * (unsigned int)width;
    sc_kind = 1;
    sc_type = t;
    sc_base = base;
    sc_bits = bits;
    return 0;
}

int sc_expression(int minimum, int active);

int sc_sequence(int active) {
    sc_expression(2, active);
    while (!failed && token == ',') {
        if (active)
            return error_message("evaluated comma is not constant expression");
        c89_extension("unevaluated comma in constant expression");
        next();
        sc_expression(2, 0);
    }
    return 0;
}

int sc_unary(int active) {
    int op = token, t, id, node, start, k, b, ice;
    unsigned long long bits, sign;
    sc_null = 0;
    sc_ice = 1;
    sc_imaginary = 0;
    if (failed)
        return 0;
    sc_depth++;
    if (sc_depth > 128)
        return error();
    if (op == '&' || op == '*' || op == '+' || op == '-' || op == '~' || op == '!') {
        next();
        sc_unary(active);
        if (op == '&') {
            if (sc_kind != 2)
                return error();
            sc_kind = 1;
            sc_type = make_pointer(sc_type);
        } else if (op == '*') {
            sc_decay();
            if (sc_kind != 1 || sc_type == 16)
                return error();
            sc_kind = 2;
            sc_type = pointed_type(sc_type);
        } else {
            sc_decay();
            if (sc_kind)
                return error();
            if (complex_component(sc_type)) {
                if (op == '~')
                    return error();
                if (op == '-') {
                    sign = complex_component(sc_type) == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
                    sc_bits ^= sign;
                    sc_imaginary ^= sign;
                }
                if (op == '!') {
                    sc_bits = !sc_truth(sc_type, sc_bits, sc_imaginary);
                    sc_imaginary = 0;
                    sc_type = 1;
                }
                sc_ice = 0;
                sc_depth--;
                return 0;
            }
            if (floating_type(sc_type)) {
                sc_ice = 0;
                if (op == '~')
                    return error();
                if (op == '-')
                    sc_bits ^= sc_type == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
                if (op == '!') {
                    sc_bits =
                        (sc_bits & (sc_type == 13 ? 0x7fffffffULL : 0x7fffffffffffffffULL)) == 0;
                    sc_type = 1;
                }
                sc_depth--;
                return 0;
            }
            sc_type = promoted(sc_type);
            sign = 1ULL << (type_size(sc_type) * 8 - 1);
            if (op == '-') {
                if (active && !unsigned_type(sc_type) && sc_bits == ce_normalize(sign, sc_type))
                    return error();
                sc_bits = 0ULL - sc_bits;
            }
            if (op == '~')
                sc_bits = ~sc_bits;
            if (op == '!') {
                sc_bits = !sc_bits;
                sc_type = 1;
            }
            sc_bits = ce_normalize(sc_bits, sc_type);
        }
        sc_depth--;
        return 0;
    }
    if (op == '(') {
        next();
        if (type_start(token)) {
            t = declarator(specifiers(), 1);
            expect(')');
            if (token == '{') {
                start = static_compound_literal(t);
                sc_kind = 2;
                sc_type = compound_static_type;
                sc_base = -2147483647;
                sc_bits = (unsigned int)start;
                sc_ice = 0;
                goto static_postfix;
            }
            sc_unary(active);
            sc_decay();
            if (complex_component(t) || complex_component(sc_type)) {
                if (sc_kind)
                    return error();
                sc_convert(sc_type, t, sc_bits, active);
                sc_ice = 0;
                sc_depth--;
                return 0;
            }
            if (floating_type(t) || floating_type(sc_type)) {
                if (sc_kind)
                    return error();
                sc_ice = sc_ice == 4 && !floating_type(t) ? 1 : 0;
                sc_convert(sc_type, t, sc_bits, active);
                sc_depth--;
                return 0;
            }
            if (!t || is_array(t) || is_record(t) || is_function(t))
                return error();
            if (is_pointer(t)) {
                sc_null = t == 16 && !sc_kind && sc_ice == 1 && !sc_bits;
                if (!sc_kind)
                    sc_base = -1;
                sc_kind = 1;
                sc_ice = 0;
            } else {
                if (sc_kind)
                    return error();
                type_size(t);
                sc_bits = ce_normalize(sc_bits, t);
            }
            sc_type = t;
            sc_depth--;
            return 0;
        }
        sc_sequence(active);
        expect(')');
    } else if (op == 300) {
        c89_extension("complex imaginary unit");
        next();
        sc_type = qualify_type(complex_type(13), 1);
        sc_bits = 0;
        sc_imaginary = 0x3f800000ULL;
        sc_kind = 0;
        sc_base = -1;
        sc_ice = 0;
    } else if (op == 299) {
        floating_bits_constant();
        sc_bits = fp_constant_bits;
        sc_type = fp_constant_type;
        sc_kind = 0;
        sc_base = -1;
        sc_ice = 4;
    } else if (op == 144 || op == 283 || op == 296 || (op >= 256 && enum_visible(op - 256))) {
        if (op == 144 && floating_type(literal_type)) {
            sc_bits = ((unsigned long long)(unsigned int)literal_high << 32) | (unsigned int)value;
            sc_type = literal_type;
            sc_ice = 4;
            next();
        } else {
            ce_unary(active);
            sc_bits = ce_value;
            sc_type = ce_type;
        }
        sc_kind = 0;
        sc_base = -1;
    } else if (op == 145) {
        sc_ice = 0;
        join_strings();
        start = position;
        emit_string();
        sc_kind = 2;
        sc_type = expression_array_type;
        sc_base = -2147483647;
        sc_bits = emitted_string_address;
    } else {
        sc_ice = 0;
        id = name();
        inline_reference(id);
        next();
        sc_kind = 2;
        sc_bits = 0;
        if (function_exists(id)) {
            sc_type = function_signature(id);
            sc_base = -id - 2;
        } else {
            if (!object_exists(id))
                return error();
            sc_type = object_type(id);
            sc_base = id;
        }
    }
static_postfix:
    while (!failed && (token == '[' || token == '.' || token == 173)) {
        op = token;
        next();
        if (op == '[') {
            sc_decay();
            k = sc_kind;
            t = sc_type;
            b = sc_base;
            bits = sc_bits;
            ice = sc_ice;
            sc_expression(2, active);
            expect(']');
            sc_add(k, t, b, bits, 0, ice);
            sc_kind = 2;
            sc_type = pointed_type(sc_type);
            sc_ice = 0;
        } else {
            if (op == 173) {
                sc_decay();
                if (sc_kind != 1)
                    return error();
                sc_kind = 2;
                sc_type = pointed_type(sc_type);
            }
            if (sc_kind != 2)
                return error();
            node = find_member(sc_type, name());
            next();
            if (member_bits[node])
                return error_message("bit-field has no address");
            sc_bits += (unsigned int)member_offset[node];
            sc_type = qualify_type(member_type[node], type_qualifiers(sc_type));
        }
    }
    sc_depth--;
    return 0;
}

int sc_expression(int minimum, int active) {
    int op, prec, k, t, b, selected, mk, mt, mb, mn, width, ice, mi;
    unsigned long long bits, middle, sign, imaginary, middle_imaginary;
    if (failed)
        return 0;
    sc_depth++;
    if (sc_depth > 128)
        return error();
    sc_unary(active);
    while (!failed) {
        op = token;
        prec = precedence(op);
        if (prec < minimum)
            break;
        sc_decay();
        k = sc_kind;
        t = sc_type;
        b = sc_base;
        bits = sc_bits;
        imaginary = sc_imaginary;
        ice = sc_ice;
        next();
        if (op == '?') {
            if (k)
                return error();
            selected = sc_truth(t, bits, imaginary);
            sc_sequence(active && selected);
            sc_decay();
            mk = sc_kind;
            mt = sc_type;
            mb = sc_base;
            middle = sc_bits;
            middle_imaginary = sc_imaginary;
            mn = sc_null;
            mi = sc_ice;
            expect(':');
            sc_expression(2, active && !selected);
            sc_decay();
            if (complex_component(mt) || complex_component(sc_type)) {
                if (mk || sc_kind)
                    return error();
                t = common_type(mt, sc_type);
                if (selected) {
                    sc_imaginary = middle_imaginary;
                    sc_convert(mt, t, middle, active);
                } else
                    sc_convert(sc_type, t, sc_bits, active);
                sc_null = 0;
                sc_ice = 0;
                continue;
            }
            if (floating_type(mt) || floating_type(sc_type)) {
                if (mk || sc_kind)
                    return error();
                t = common_type(mt, sc_type);
                if (selected)
                    sc_convert(mt, t, middle, active);
                else
                    sc_convert(sc_type, t, sc_bits, active);
                sc_null = 0;
                sc_ice = 0;
                continue;
            }
            if (is_pointer(mt) && (sc_null || (!sc_kind && sc_ice == 1 && !sc_bits)))
                sc_type = mt;
            else if (is_pointer(sc_type) && (mn || (!mk && mi == 1 && !middle)))
                mt = sc_type;
            sc_ice = ice == 1 && mi == 1 && sc_ice == 1 ? 1 : 0;
            t = common_type(mt, sc_type);
            if (selected) {
                sc_kind = mk;
                sc_base = mb;
                sc_bits = middle;
            }
            sc_type = t;
            if (is_pointer(t)) {
                sc_null = t == 16 && !sc_kind && !sc_bits;
                if (!sc_kind)
                    sc_base = -1;
                sc_kind = 1;
            } else
                sc_bits = ce_normalize(sc_bits, t);
        } else {
            selected = active;
            if (op == 134 && !sc_truth(t, bits, imaginary))
                selected = 0;
            if (op == 135 && sc_truth(t, bits, imaginary))
                selected = 0;
            sc_expression(prec + 1, selected);
            sc_decay();
            if (complex_component(t) || complex_component(sc_type)) {
                if (k || sc_kind)
                    return error();
                sc_complex_binary(op, t, bits, imaginary, active);
                sc_null = 0;
                sc_ice = 0;
                continue;
            }
            if (floating_type(t) || floating_type(sc_type)) {
                if (k || sc_kind)
                    return error();
                sc_floating_binary(op, t, bits, active);
                sc_null = 0;
                sc_ice = 0;
                continue;
            }
            if (k || sc_kind) {
                if (op != '+' && op != '-')
                    return error();
                sc_add(k, t, b, bits, op == '-', ice);
                sc_ice = 0;
            } else if (op == 134 || op == 135) {
                if (op == 134)
                    sc_bits = (bits != 0) && (sc_bits != 0);
                else
                    sc_bits = (bits != 0) || (sc_bits != 0);
                sc_type = 1;
            } else if (op == 132 || op == 133) {
                t = promoted(t);
                width = type_size(t) * 8;
                bits = ce_normalize(bits, t);
                sign = 1ULL << (width - 1);
                if (active && sc_bits >= (unsigned int)width)
                    return error();
                if (!active)
                    sc_bits = 0;
                if (op == 132) {
                    if (active && !c89_identifiers && !unsigned_type(t) &&
                        ((bits & sign) || bits > ((sign - 1) >> sc_bits)))
                        return error();
                    sc_bits = bits << sc_bits;
                } else if (unsigned_type(t))
                    sc_bits = bits >> sc_bits;
                else
                    sc_bits = (long long)bits >> sc_bits;
                sc_type = t;
                sc_bits = ce_normalize(sc_bits, t);
            } else {
                ce_binary(op, bits, sc_bits, common_type(t, sc_type), active);
                sc_type = ce_type;
                sc_bits = ce_value;
            }
            sc_ice = ice == 1 && sc_ice == 1 ? 1 : 0;
        }
        sc_null = 0;
    }
    sc_depth--;
    return 0;
}

int static_value(int target) {
    sc_expression(2, 1);
    sc_decay();
    if (floating_type(target) || floating_type(sc_type) || complex_component(sc_type)) {
        if (sc_kind)
            return error();
        sc_convert(sc_type, target, sc_bits, 1);
    }
    if (object_mode && sc_kind == 1 && sc_base == -2147483647) {
        sequence(0x058d48, 3);
        relative((int)sc_bits);
    } else if (sc_kind == 1 && sc_base < -1 && sc_base != -2147483647) {
        sequence(0x058d48, 3);
        function_reference(-sc_base - 2);
        if (sc_bits)
            return error();
    } else if (sc_kind == 1 && sc_base >= 0) {
        sequence(0x058d48, 3);
        object_reference(sc_base);
        if (sc_bits) {
            sequence(0xb948, 2);
            emit32((int)sc_bits);
            emit32((int)(sc_bits >> 32));
            sequence(0xc80148, 3);
        }
    } else if (wide_type(sc_type)) {
        sequence(0xb848, 2);
        emit32((int)sc_bits);
        emit32((int)(sc_bits >> 32));
    } else
        constant((int)sc_bits);
    expression_type = sc_type;
    expression_ice = sc_kind ? 0 : sc_ice;
    expression_value = sc_bits;
    expression_zero = (!sc_kind && sc_ice == 1 && !floating_type(sc_type) && !sc_bits) || sc_null;
    expression_array_bytes = 0;
    expression_array_type = 0;
    return 0;
}

int initializer_member(int t) {
    int node;
    c89_extension("designated initializer");
    expect('.');
    node = find_member(t, name());
    next();
    if (failed)
        return 0;
    if (incomplete_array(member_type[node]))
        return error_message("cannot initialize flexible array member");
    initializer_designated = token == '.' || token == '[';
    if (!initializer_designated)
        expect('=');
    return node;
}

int initializer_index(int n) {
    unsigned long long index;
    c89_extension("designated initializer");
    expect('[');
    ce_expression(2, 1);
    index = ce_value;
    expect(']');
    if (index > 2147483647ULL || (n && index >= (unsigned int)n))
        return error_message("initializer index outside array bounds");
    initializer_designated = token == '.' || token == '[';
    if (!initializer_designated)
        expect('=');
    return (int)index;
}

int initializer_clear(int t, int base, int offset, int global) {
    if (!is_record(t) && !is_array(t))
        return 0;
    emit(0x57);
    if (global) {
        sequence(0x3d8d48, 3);
        relative(base + offset);
    } else {
        sequence(0xbd8d48, 3);
        emit32(offset - base);
    }
    sequence(0xc031, 2);
    emit(0xb9);
    emit32(type_size(t));
    emit(0xfc);
    sequence(0xaaf3, 2);
    emit(0x5f);
    return 0;
}

/* Compound literals have block lifetime, unlike expression intermediates. */
int initializer_literal_epoch;

int initialize_bitfield(int node, int base, int offset, int global, int constant_list) {
    int braces = 0, saved_bytes = local_bytes, saved_epoch = initializer_literal_epoch;
    while (!initializer_pending && token == '{') {
        next();
        if (++braces + initializer_depth > 64)
            return error();
    }
    if (!initializer_pending) {
        if (global || constant_list)
            static_value(member_type[node]);
        else
            expression(1);
    }
    initializer_pending = 0;
    initializer_address(base, offset, global);
    bitfield_store(node);
    while (braces--) {
        if (token == ',')
            next();
        expect('}');
    }
    if (!global && saved_epoch == initializer_literal_epoch)
        local_bytes = saved_bytes;
    return 0;
}

int initialize_object(int t, int base, int offset, int global, int constant_list) {
    int saved_bytes = local_bytes, saved_epoch = initializer_literal_epoch;
    int braces = 0, i, n, element, width, node, copied = 0, maximum = 0,
        designated = initializer_designated;
    initializer_designated = 0;
    if (failed)
        return 0;
    initializer_depth = initializer_depth + 1;
    if (initializer_depth > 64)
        return error();
    if (!initializer_pending && token == '{') {
        braces = 1;
        next();
        if (token == '}')
            return error();
    }
    if (c89_identifiers && braces && (is_array(t) || is_record(t)))
        constant_list = 1;
    if (!initializer_pending && character_array(t) && token == 145) {
        join_strings();
        copy_string(t, base, offset, global);
    } else if (complex_component(t)) {
        if (global || constant_list) {
            element = complex_component(t);
            if (initializer_pending)
                return error_message("pending complex static initializer");
            sc_expression(2, 1);
            sc_decay();
            if (sc_kind)
                return error();
            sc_convert(sc_type, t, sc_bits, 1);
            complex_real_bits(element, (int)sc_bits, (int)(sc_bits >> 32));
            initializer_address(base, offset, global);
            store(-1, element);
            complex_real_bits(element, (int)sc_imaginary, (int)(sc_imaginary >> 32));
            initializer_address(base, offset + type_size(element), global);
            store(-1, element);
        } else {
            if (!initializer_pending)
                expression(1);
            initializer_pending = 0;
            initializer_address(base, offset, global);
            convert(t);
            store(-1, t);
        }
    } else if (is_record(t)) {
        type_size(t);
        n = (unqualified(t) - 0x20000e) / 256;
        if (!braces && !designated && !global && (initializer_pending || token != 145)) {
            if (!initializer_pending) {
                if (constant_list)
                    static_value(t);
                else
                    expression(1);
                initializer_pending = 1;
            }
            if (unqualified(expression_type) == unqualified(t)) {
                initializer_address(base, offset, global);
                convert(t);
                store(-1, t);
                initializer_pending = 0;
                copied = 1;
            }
        }
        if (!copied) {
            if (!braces && initializer_depth == 1)
                return error();
            node = named_member(record_first[n]);
            while ((node || (braces && token == '.')) && !failed &&
                   (initializer_pending || token != '}')) {
                if ((braces || designated) && token == '.') {
                    node = initializer_member(t);
                    if (!failed && !initializer_designated)
                        initializer_clear(member_type[node], base, offset + member_offset[node],
                                          global);
                }
                if (failed)
                    break;
                designated = 0;
                if (incomplete_array(member_type[node]))
                    break;
                if (member_bits[node])
                    initialize_bitfield(node, base, offset + member_offset[node], global,
                                        constant_list);
                else
                    initialize_object(member_type[node], base, offset + member_offset[node], global,
                                      constant_list);
                node = record_kind[n] == 2 ? 0 : named_member(member_next[node]);
                if ((braces || node) && initializer_more(braces))
                    next();
                else
                    break;
            }
        }
    } else if (is_array(t)) {
        if (!braces && initializer_depth == 1)
            return error();
        i = (t - 4096) / 256;
        n = array_length[i];
        element = array_base[i];
        width = type_size(element);
        i = 0;
        while ((!n && initializer_probe || i < n || (braces && token == '[')) && !failed) {
            if (!initializer_pending && token == '}')
                break;
            if ((braces || designated) && token == '[') {
                i = initializer_index(n);
                if (!failed && !initializer_designated)
                    initializer_clear(element, base, offset + i * width, global);
            }
            if (!n && i >= 0x1a00000 / width)
                return error();
            designated = 0;
            initialize_object(element, base, offset + i * width, global, constant_list);
            i++;
            if (i > maximum)
                maximum = i;
            if ((braces || !n || i < n) && initializer_more(braces))
                next();
            else
                break;
        }
        if (initializer_probe && initializer_depth == 1)
            inferred_elements = maximum;
    } else if (!initializer_pending && token == '{') {
        initialize_object(t, base, offset, global, constant_list);
    } else {
        if (!initializer_pending) {
            if (global || constant_list)
                static_value(t);
            else
                expression(1);
        }
        initializer_pending = 0;
        initializer_address(base, offset, global);
        convert(t);
        store(-1, t);
    }
    if (braces) {
        if (token == ',')
            next();
        expect('}');
    }
    initializer_depth = initializer_depth - 1;
    /* The value has been stored. Preserve the peak for the prologue, but
       recycle intermediates unless a block-lifetime literal was allocated. */
    if (!global && saved_epoch == initializer_literal_epoch)
        local_bytes = saved_bytes;
    return 0;
}

int compound_literal(int t) {
    int offset, n, element = t, saved_cursor = cursor, saved_line = line,
                   saved_strings = string_used;
    int saved_depth = initializer_depth, saved_pending = initializer_pending,
        saved_designated = initializer_designated;
    if (!block_depth) {
        offset = static_compound_literal(t);
        sequence(0x058d48, 3);
        relative(offset);
        return indirect_value(compound_static_type);
    }
    initializer_literal_epoch++;
    c89_extension("compound literal");
    if (variably_modified_type(t))
        return error_message("compound literal has variably modified type");
    if (!unqualified(t) || is_function(t))
        return error_message("invalid automatic compound literal type");
    initializer_depth = 0;
    initializer_pending = 0;
    initializer_designated = 0;
    while (is_array(element))
        element = array_base[(element - 4096) / 256];
    if (incomplete_array(t) && is_record(element))
        t = infer_aggregate_array(t, -1);
    if (incomplete_array(t)) {
        n = initializer_shape(t);
        cursor = saved_cursor;
        line = saved_line;
        string_used = saved_strings;
        token = '{';
        initializer_depth = 0;
        if (failed || n <= 0)
            return error();
        t = make_array(array_base[(t - 4096) / 256], n);
    }
    offset = temporary(type_size(t), type_align(t));
    zero_local(offset, type_size(t));
    initialize_object(t, offset, 0, 0, 0);
    initializer_depth = saved_depth;
    initializer_pending = saved_pending;
    initializer_designated = saved_designated;
    sequence(0x858d48, 3);
    emit32(-offset);
    return indirect_value(t);
}

int static_compound_literal(int t) {
    int address, n, align, width, saved_cursor = cursor, saved_line = line,
                                  saved_strings = string_used;
    int saved_depth = initializer_depth, saved_pending = initializer_pending,
        saved_designated = initializer_designated;
    c89_extension("compound literal");
    if (variably_modified_type(t))
        return error_message("compound literal has variably modified type");
    if (block_depth || !unqualified(t) || is_function(t))
        return error_message("compound literal is not a static initializer address");
    initializer_depth = 0;
    initializer_pending = 0;
    initializer_designated = 0;
    if (incomplete_array(t)) {
        n = initializer_shape(t);
        cursor = saved_cursor;
        line = saved_line;
        string_used = saved_strings;
        token = '{';
        initializer_depth = 0;
        if (failed || n <= 0)
            return error();
        t = make_array(array_base[(t - 4096) / 256], n);
    }
    width = type_size(t);
    align = type_align(t);
    allocation = ((allocation + align - 1) / align) * align;
    if (width > 0x2000000 - allocation)
        return error();
    address = allocation;
    allocation += width;
    initialize_object(t, address, 0, 1, 0);
    initializer_depth = saved_depth;
    initializer_pending = saved_pending;
    initializer_designated = saved_designated;
    compound_static_type = t;
    return address;
}

int static_declaration(int base) {
    int id, t, width, align, element, address, inferred, skip, original;
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        if (failed)
            return 0;
        if ((local_offset[id] || alias_type[id]) && local_depth[id] == block_depth)
            return error();
        if (local_count == 256)
            return error();
        if (decl_inline)
            return error();
        inferred = is_array(t) && array_length[(t - 4096) / 256] == 0;
        original = t;
        if (inferred)
            t = infer_initializer(t, id, 1);
        width = type_size(t);
        if (failed)
            return 0;
        element = t;
        while (is_array(element))
            element = array_base[(element - 4096) / 256];
        if (inline_body >= 0 && !(type_qualifiers(element) & 1))
            inline_invalid[inline_body] = 1;
        align = type_align(t);
        allocation = ((allocation + align - 1) / align) * align;
        if (allocation + width > 0x2000000)
            return error();
        address = allocation;
        allocation += width;
        binding_name[local_count] = id;
        binding_offset[local_count] = local_offset[id];
        binding_depth[local_count] = local_depth[id];
        binding_type[local_count] = local_type[id];
        binding_bytes[local_count] = local_bytes;
        binding_alias[local_count] = alias_type[id];
        binding_register[local_count] = register_object[id];
        binding_static_address[local_count] = local_static_address[id];
        local_count++;
        local_offset[id] = -1;
        local_static_address[id] = address;
        local_type[id] = t;
        local_depth[id] = block_depth;
        alias_type[id] = 0;
        register_object[id] = 0;
        bind_variable_type(t, 0);
        if (inferred)
            local_type[id] = original;
        if (inferred || token == '=') {
            skip = jump(0);
            if (initializer_tail)
                patch(initializer_tail, position);
            else
                initializer_start = position;
            next();
            initialize_object(t, address, 0, 1, 0);
            initializer_tail = jump(0);
            patch(skip, position);
        }
        local_type[id] = t;
        if (token != ',')
            break;
        next();
    }
    return expect(';');
}

int composite_type(int a, int b) {
    int na, nb, element, i, n, parameters[128];
    if (a == b)
        return a;
    if (type_qualifiers(a) != type_qualifiers(b))
        return error_message("incompatible type qualification");
    if (type_qualifiers(a))
        return qualify_type(composite_type(unqualified(a), unqualified(b)), type_qualifiers(a));
    if (is_enum(a) && b == 1)
        return a;
    if (is_enum(b) && a == 1)
        return b;
    if (is_function(a) && is_function(b)) {
        a = (a - 0x30000f) / 256;
        b = (b - 0x30000f) / 256;
        element = composite_type(function_result[a], function_result[b]);
        na = function_arity[a];
        nb = function_arity[b];
        if (na >= 0 && nb >= 0 && na != nb)
            return error();
        if ((na >= 128 && nb < 0) || (nb >= 128 && na < 0))
            return error();
        n = na >= 0 ? na : nb;
        for (i = 0; i < fixed_arity(n); i++) {
            if (na < 0)
                parameters[i] = composite_type(function_parameter[b * 128 + i],
                                               default_promoted(function_parameter[b * 128 + i]));
            else if (nb < 0)
                parameters[i] = composite_type(function_parameter[a * 128 + i],
                                               default_promoted(function_parameter[a * 128 + i]));
            else
                parameters[i] = composite_type(function_parameter[a * 128 + i],
                                               function_parameter[b * 128 + i]);
        }
        return make_function(element, n, parameters);
    }
    if (is_array(a) && is_array(b)) {
        na = array_length[(a - 4096) / 256];
        nb = array_length[(b - 4096) / 256];
        if (na > 0 && nb > 0 && na != nb)
            return error();
        element = composite_type(array_base[(a - 4096) / 256], array_base[(b - 4096) / 256]);
        if (array_bytes[(a - 4096) / 256] < -1)
            return rebase_array(a, element);
        if (array_bytes[(b - 4096) / 256] < -1)
            return rebase_array(b, element);
        return make_array(element, na > 0 ? na : nb > 0 ? nb : na < 0 || nb < 0 ? -1 : 0);
    }
    if (is_pointer(a) && is_pointer(b) && !is_array(a) && !is_array(b))
        return make_pointer(composite_type(pointed_type(a), pointed_type(b)));
    return error();
}

int allocate_global(int id) {
    int t = global_type[id], width = type_size(t), element = t, align;
    if (failed)
        return 0;
    while (is_array(element))
        element = array_base[(element - 4096) / 256];
    align = type_align(t);
    allocation = ((allocation + align - 1) / align) * align;
    if (allocation + width > 0x2000000)
        return error();
    global_address[id] = allocation;
    allocation += width;
    global_extent[id] = width;
    return 0;
}

int extern_declaration(int base) {
    int id, t, visible, same;
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        if (failed || !unqualified(t))
            return error();
        if (variably_modified_type(t))
            return error_message("variably modified identifier cannot have linkage");
        if (is_function(t)) {
            if (token != ',' && token != ';')
                return error();
            block_function(id, t, 5);
        } else {
            if (decl_inline || token == '=' || token == '(')
                return error();
            same = (local_offset[id] || alias_type[id]) && local_depth[id] == block_depth;
            if (same && local_offset[id] != -2)
                return error();
            visible = local_offset[id] == -2 ||
                      (!local_offset[id] && !alias_type[id] && global_visible[id]);
            if ((symbol_flags[id] & 71) || ((symbol_flags[id] & 32) && !visible))
                return error();
            if (visible)
                t = composite_type(object_type(id), t);
            if (entity_type[id])
                entity_type[id] = composite_type(entity_type[id], t);
            else
                entity_type[id] = t;
            symbol_flags[id] = symbol_flags[id] | 8;
            if (!same) {
                if (local_count == 256)
                    return error();
                binding_name[local_count] = id;
                binding_offset[local_count] = local_offset[id];
                binding_depth[local_count] = local_depth[id];
                binding_type[local_count] = local_type[id];
                binding_bytes[local_count] = local_bytes;
                binding_alias[local_count] = alias_type[id];
                binding_register[local_count] = register_object[id];
                binding_static_address[local_count] = local_static_address[id];
                local_count++;
            }
            local_offset[id] = -2;
            local_type[id] = t;
            local_depth[id] = block_depth;
            alias_type[id] = 0;
            register_object[id] = 0;
            local_static_address[id] = 0;
        }
        if (token != ',')
            break;
        next();
    }
    return expect(';');
}

int finish_objects(void) {
    int id, t, p;
    for (id = 0; id < count && !failed; id++) {
        if (global_tentative[id] && !global_address[id]) {
            t = global_type[id];
            if (is_array(t) && !array_length[(t - 4096) / 256])
                global_type[id] = make_array(array_base[(t - 4096) / 256], 1);
            allocate_global(id);
        }
    }
    for (id = 0; id < object_ref_count && !failed; id++) {
        t = object_ref_symbol[id];
        p = object_ref_position[id];
        if (!global_address[t] && !object_mode)
            return error();
        write32(p, global_address[t] - (p + 0x400e00 + 4));
    }
    return 0;
}

int merge_function_entity(int id, int t, int storage) {
    int i, signature = (t - 0x30000f) / 256, n = function_arity[signature];
    if (symbol_flags[id] & 8)
        return error();
    if (storage == 4) {
        if (symbol_flags[id] && !(symbol_flags[id] & 32))
            return error();
        symbol_flags[id] = symbol_flags[id] | 32;
    }
    if ((symbol_flags[id] & 1) && !(symbol_flags[id] & 4) && n >= 0) {
        if (arity[id] != n)
            return error();
        for (i = 0; i < fixed_arity(n); i++)
            composite_type(parameter_type[id * 128 + i], function_parameter[signature * 128 + i]);
    }
    if (symbol_flags[id] & 69)
        t = composite_type(function_entity_signature(id), t);
    signature = (t - 0x30000f) / 256;
    n = function_arity[signature];
    global_type[id] = function_result[signature];
    if (id == 6 && ((n >= 0 && n != 0 && n != 2) || unqualified(global_type[id]) != 1))
        return error();
    if (n >= 0) {
        symbol_flags[id] = symbol_flags[id] | 4;
        arity[id] = n;
        for (i = 0; i < fixed_arity(n); i++)
            parameter_type[id * 128 + i] = function_parameter[signature * 128 + i];
    } else
        symbol_flags[id] = symbol_flags[id] | 64;
    return 0;
}

int expose_file_function(int id, int t) {
    if (file_function_type[id])
        t = composite_type(file_function_type[id], t);
    file_function_type[id] = t;
    return 0;
}

int typed_function_declaration(int id, int t, int storage) {
    int i;
    if (token != ',' && token != ';' && !(token == '{' && definition_ready))
        return error();
    if (definition_ready) {
        for (i = 0; i < local_count; i++)
            if (binding_name[i] == id && binding_alias[i])
                return error();
    }
    if (alias_type[id] || enum_global[id])
        return error();
    if (decl_inline) {
        if (id == 6)
            return error();
        inline_seen[id] = 1;
    }
    if (!decl_inline || storage == 5)
        inline_external[id] = 1;
    merge_function_entity(id, t, storage);
    expose_file_function(id, t);
    return 0;
}

int block_function(int id, int t, int storage) {
    int same = (local_offset[id] || alias_type[id]) && local_depth[id] == block_depth;
    int visible = function_exists(id);
    if (variably_modified_type(t))
        return error_message("variably modified function cannot have linkage");
    if (decl_inline && id == 6)
        return error();
    if (storage && storage != 5)
        return error();
    if (same && local_offset[id] != -4)
        return error();
    if ((symbol_flags[id] & 32) && !visible)
        return error();
    if (visible)
        t = composite_type(function_signature(id), t);
    merge_function_entity(id, t, 0);
    if (!same) {
        if (local_count == 256)
            return error();
        binding_name[local_count] = id;
        binding_offset[local_count] = local_offset[id];
        binding_depth[local_count] = local_depth[id];
        binding_type[local_count] = local_type[id];
        binding_bytes[local_count] = local_bytes;
        binding_alias[local_count] = alias_type[id];
        binding_register[local_count] = register_object[id];
        binding_static_address[local_count] = local_static_address[id];
        local_count++;
    }
    local_offset[id] = -4;
    local_type[id] = t;
    local_depth[id] = block_depth;
    alias_type[id] = 0;
    register_object[id] = 0;
    local_static_address[id] = 0;
    return 0;
}

int implicit_function(int id) {
    int t = make_function(1, -1, &parameter_type[id * 128]);
    if (block_depth)
        return block_function(id, t, 0);
    merge_function_entity(id, t, 0);
    return expose_file_function(id, t);
}

int global(int id, int t, int base, int storage) {
    int inferred, incomplete, initialized, known, original, inline_spec = decl_inline;
    while (!failed) {
        if (is_function(t)) {
            decl_inline = inline_spec;
            typed_function_declaration(id, t, storage);
        } else {
            if (inline_spec)
                return error();
            known = symbol_flags[id] & 8;
            if (alias_type[id] || enum_global[id] || (symbol_flags[id] && !known))
                return error();
            if (known) {
                if (storage == 4 && !(symbol_flags[id] & 32))
                    return error();
                if (storage == 0 && (symbol_flags[id] & 32))
                    return error();
                if (global_visible[id])
                    t = composite_type(global_type[id], t);
            }
            incomplete = (is_array(t) && !array_length[(t - 4096) / 256]) ||
                         (is_record(t) && !record_size[(unqualified(t) - 0x20000e) / 256]);
            inferred = is_array(t) && incomplete && token == '=';
            original = t;
            if (inferred)
                t = infer_initializer(t, id, 1);
            incomplete = (is_array(t) && !array_length[(t - 4096) / 256]) ||
                         (is_record(t) && !record_size[(unqualified(t) - 0x20000e) / 256]);
            initialized = inferred || token == '=';
            if (!unqualified(t) || (incomplete && storage == 4))
                return error();
            if (initialized && global_initialized[id])
                return error();
            if (entity_type[id])
                entity_type[id] = composite_type(entity_type[id], t);
            else
                entity_type[id] = t;
            global_type[id] = t;
            global_visible[id] = 1;
            symbol_flags[id] = symbol_flags[id] | 8;
            if (storage == 4)
                symbol_flags[id] = symbol_flags[id] | 32;
            if (storage != 5 && !initialized)
                global_tentative[id] = 1;
            if (!global_address[id] && !incomplete && (initialized || global_tentative[id]))
                allocate_global(id);
            if (initialized) {
                global_initialized[id] = 1;
                if (initializer_tail)
                    patch(initializer_tail, position);
                else
                    initializer_start = position;
                if (inferred)
                    global_type[id] = original;
                next();
                initialize_object(t, global_address[id], 0, 1, 0);
                global_type[id] = t;
                initializer_tail = jump(0);
            }
        }
        if (token != ',')
            break;
        next();
        t = declarator(base, 0);
        id = decl_name;
    }
    return expect(';');
}

int parameter(int base, int function_id, int known) {
    int storage = decl_storage, pt, i;
    save_parameter_declarator(parameter_count, base);
    pt = declarator(base, 2);
    if (decl_inline || (storage && storage != 3))
        return error();
    if (is_array(pt))
        pt = make_pointer(array_base[(pt - 4096) / 256]);
    if (is_function(pt))
        pt = make_pointer(pt);
    if (parameter_count == 127 || local_count == 256 || !unqualified(pt))
        return error();
    i = function_id * 128 + parameter_count;
    parameter_count++;
    parameter_decl_type[parameter_count - 1] = pt;
    parameter_name[parameter_count - 1] = decl_name;
    if (known)
        parameter_type[i] = composite_type(parameter_type[i], unqualified(pt));
    else
        parameter_type[i] = unqualified(pt);
    if (decl_name == -1) {
        binding_name[local_count] = -1;
        binding_bytes[local_count] = local_bytes;
        local_count = local_count + 1;
        local_bytes = local_bytes + 8;
        return 1;
    }
    local(11, decl_name);
    local_type[decl_name] = pt;
    register_object[decl_name] = storage == 3;
    return 0;
}

int alias_declaration(int base, int global_scope) {
    int id, t;
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        if (failed)
            return 0;
        if (global_scope) {
            if (global_visible[id] || enum_global[id] || file_function_type[id] || alias_type[id])
                return error();
            alias_type[id] = t + 1;
            symbol_flags[id] = symbol_flags[id] | 16;
        } else {
            if ((local_offset[id] || alias_type[id]) && local_depth[id] == block_depth)
                return error();
            if (local_count == 256)
                return error();
            binding_name[local_count] = id;
            binding_offset[local_count] = local_offset[id];
            binding_depth[local_count] = local_depth[id];
            binding_type[local_count] = local_type[id];
            binding_bytes[local_count] = local_bytes;
            binding_alias[local_count] = alias_type[id];
            binding_register[local_count] = register_object[id];
            register_object[id] = 0;
            binding_static_address[local_count] = local_static_address[id];
            local_static_address[id] = 0;
            local_count++;
            local_offset[id] = 0;
            local_type[id] = 0;
            local_depth[id] = block_depth;
            alias_type[id] = t + 1;
            bind_variable_type(t, 0);
        }
        if (token != ',')
            break;
        next();
    }
    return expect(';');
}

int old_parameter_seen[128];

int old_parameters(int id, int known) {
    int i, p, base, pt, storage;
    while (!failed && token != ')') {
        if (parameter_count == 127)
            return error();
        p = name();
        next();
        i = parameter_count++;
        parameter_name[i] = p;
        parameter_decl_type[i] = 1;
        old_parameter_seen[i] = 0;
        local(11, p);
        local_type[p] = 1;
        if (token != ',')
            break;
        next();
        if (token == ')')
            return error();
    }
    expect(')');
    return old_parameter_declarations(id, known);
}

int old_parameter_declarations(int id, int known) {
    int i, p, base, pt, storage;
    prototype_depth++;
    prototype_star_seen = 0;
    while (!failed && type_start(token)) {
        base = specifiers();
        storage = decl_storage;
        if (decl_inline || (storage && storage != 3))
            return error();
        while (!failed) {
            save_parameter_declarator(127, base);
            pt = declarator(base, 2);
            p = decl_name;
            if (is_array(pt))
                pt = make_pointer(array_base[(pt - 4096) / 256]);
            if (is_function(pt))
                pt = make_pointer(pt);
            if (!unqualified(pt))
                return error();
            for (i = 0; i < parameter_count && parameter_name[i] != p; i++) {
            }
            if (i == parameter_count || old_parameter_seen[i])
                return error();
            parameter_replay_valid[i] = 1;
            parameter_replay_base[i] = parameter_replay_base[127];
            parameter_replay_token[i] = parameter_replay_token[127];
            parameter_replay_cursor[i] = parameter_replay_cursor[127];
            parameter_replay_line[i] = parameter_replay_line[127];
            parameter_replay_storage[i] = parameter_replay_storage[127];
            old_parameter_seen[i] = 1;
            parameter_decl_type[i] = pt;
            local_type[p] = pt;
            register_object[p] = storage == 3;
            if (token != ',')
                break;
            next();
        }
        expect(';');
    }
    if (token != '{')
        return error();
    definition_star_seen = prototype_star_seen;
    prototype_depth--;
    prototype_star_seen = 0;
    if (known && arity[id] != parameter_count)
        return error();
    for (i = 0; i < parameter_count; i++) {
        pt = default_promoted(parameter_decl_type[i]);
        if (known)
            composite_type(parameter_type[id * 128 + i], pt);
        else
            parameter_type[id * 128 + i] = pt;
    }
    return 0;
}

/* __func__ uses the execution encoding, unlike canonical object symbols. */
int function_name_unit(int id, int *at) {
    int p, c;
    if (!spelling_length[id])
        return names[id * 256 + (*at)++];
    p = spelling_start[id] + *at;
    c = source[p];
    if (c == 92) {
        c = identifier_ucn(source, spelling_start[id] + spelling_length[id], &p, *at == 0);
        *at = p - spelling_start[id];
        return c > 255 ? 63 : c;
    }
    *at = *at + 1;
    return c;
}

int predefined_function_name(int function_id) {
    int id, i, match, address, skip, t, full_length, source_length;
    char *spelling = "__func__";
    for (id = 0; id < count; id++) {
        match = lengths[id] == 8;
        for (i = 0; i < 8 && match; i++)
            if (names[id * 256 + i] != spelling[i])
                match = 0;
        if (match)
            break;
    }
    if (id == count) {
        if (count == 2048)
            return error_message("identifier capacity exceeded");
        seed(count, 0x75665f5f, 0x5f5f636e, 0);
        count++;
    }
    if ((local_offset[id] || alias_type[id]) && local_depth[id] == 1)
        return error();
    if (local_count == 256)
        return error();
    source_length = spelling_length[function_id];
    if (!source_length)
        source_length = lengths[function_id];
    full_length = 0;
    i = 0;
    while (i < source_length && !failed) {
        function_name_unit(function_id, &i);
        full_length++;
    }
    t = make_array(qualify_type(9, 1), full_length + 1);
    skip = jump(0);
    address = position + 0x400e00;
    i = 0;
    while (i < source_length && !failed)
        emit(function_name_unit(function_id, &i));

    emit(0);
    patch(skip, position);
    binding_name[local_count] = id;
    binding_offset[local_count] = local_offset[id];
    binding_depth[local_count] = local_depth[id];
    binding_type[local_count] = local_type[id];
    binding_bytes[local_count] = local_bytes;
    binding_alias[local_count] = alias_type[id];
    binding_register[local_count] = register_object[id];
    binding_static_address[local_count] = local_static_address[id];
    local_count++;
    local_offset[id] = -1;
    local_static_address[id] = address;
    local_type[id] = t;
    local_depth[id] = 1;
    alias_type[id] = 0;
    register_object[id] = 0;
    return 0;
}

int function(void) {
    int id, i, n, t, base, known, storage, unnamed = 0, hidden, aggregate, unspecified, old = 0,
                                           implicit = 0, declaration_base, declaration_result,
                                           variadic = 0;
    for (i = 0; i < 128; i++)
        parameter_replay_valid[i] = 0;
    vm_current = 0;
    vm_count = 0;
    vm_goto_count = 0;
    block_depth = 0;
    definition_star_seen = 0;
    prototype_depth = 0;
    prototype_star_seen = 0;
    if (type_start(token))
        base = specifiers();
    else {
        base = 1;
        decl_storage = 0;
        decl_typedef = 0;
        decl_inline = 0;
        implicit = 1;
    }
    declaration_base = base;
    storage = decl_storage;
    if (token == ';' && (is_enum(base) || is_record(base)) && !storage) {
        if (decl_inline)
            return error();
        next();
        return 0;
    }
    if (decl_storage == 2 || decl_storage == 3)
        return error();
    if (decl_typedef) {
        if (decl_inline)
            return error();
        return alias_declaration(base, 1);
    }
    definition_ready = 0;
    definition_old = 0;
    declarator_top_function = 1;
    t = declarator(base, 0);
    declarator_top_function = 0;
    id = decl_name;
    if (is_function(t) || token == '(') {
        if (decl_inline) {
            if (id == 6)
                return error();
            inline_seen[id] = 1;
        }
        if (!decl_inline || storage == 5)
            inline_external[id] = 1;
    }
    if (failed)
        return 0;
    if (is_function(t) && definition_ready && definition_old) {
        parameter_count = definition_parameter_count;
        for (i = 0; i < parameter_count; i++) {
            parameter_name[i] = definition_parameter_name[i];
            parameter_decl_type[i] = 1;
            old_parameter_seen[i] = 0;
        }
        old_parameter_declarations(id, (symbol_flags[id] & 4) != 0);
        for (i = 0; i < parameter_count; i++)
            definition_parameter_type[i] = parameter_decl_type[i];
    }
    if (is_function(t) && token == '{' && definition_ready) {
        n = definition_parameter_count;
        if ((symbol_flags[id] & 4) && function_arity[(t - 0x30000f) / 256] < 0 && arity[id] != n)
            return error();
        variadic = function_arity[(t - 0x30000f) / 256] >= 128;
        typed_function_declaration(id, t, storage);
        t = global_type[id];
        arity[id] = n + variadic * 128;
        parameter_count = n;
        for (i = 0; i < n; i++) {
            parameter_name[i] = definition_parameter_name[i];
            parameter_decl_type[i] = definition_parameter_type[i];
            if (parameter_name[i] < 0)
                unnamed = 1;
        }
        old = definition_old;
        definition_ready = 0;
        goto function_body;
    }
    if (definition_ready) {
        restore_locals(0);
        restore_tags(0);
        block_depth = 0;
        definition_ready = 0;
    }
    if (token != '(') {
        if (implicit)
            return error();
        return global(id, t, base, storage);
    }
    if (alias_type[id] || enum_global[id])
        return error();
    /* Simple definitions retain the existing parameter-list parser. */
    if (is_array(t) || is_function(t) || (decl_parentheses && t != base))
        return error();
    if (symbol_flags[id] & 8)
        return error();
    if (storage == 4) {
        if (symbol_flags[id] && !(symbol_flags[id] & 32))
            return error();
        symbol_flags[id] = symbol_flags[id] | 32;
    }
    declaration_result = t;
    known = (symbol_flags[id] & 4) != 0;
    if (symbol_flags[id] & 68)
        t = composite_type(global_type[id], t);
    global_type[id] = t;
    for (i = 0; i < count; i = i + 1) {
        local_offset[i] = 0;
        local_depth[i] = 0;
        local_type[i] = 0;
    }
    local_count = 0;
    local_bytes = 0;
    frame_peak = 0;
    block_depth = 1;
    parameter_count = 0;
    next();
    unspecified = token == ')';
    old = !unspecified && !type_start(token);
    if (old)
        old_parameters(id, known);
    else {
        prototype_depth++;
        prototype_star_seen = 0;
        while (!failed && token != ')') {
            base = specifiers();
            if (!base && token == ')' && !parameter_count && !decl_storage)
                break;
            unnamed = unnamed | parameter(base, id, known || ((symbol_flags[id] & 1) != 0));
            if (token != ',')
                break;
            next();
            if (token == ')')
                return error();
            if (token == 174) {
                variadic = 1;
                next();
                break;
            }
        }
        expect(')');
        definition_star_seen = prototype_star_seen;
        prototype_depth--;
        prototype_star_seen = 0;
    }
    n = parameter_count;
    if (id == 6 && ((n != 0 && n != 2) || variadic || unqualified(t) != 1))
        return error();
    if (old) {
        symbol_flags[id] = symbol_flags[id] | 64;
        arity[id] = n;
    } else if (unspecified) {
        if (known) {
            if (arity[id] >= 128)
                return error();
            for (i = 0; i < arity[id]; i++)
                composite_type(parameter_type[id * 128 + i],
                               default_promoted(parameter_type[id * 128 + i]));
            if (token != ';' && arity[id])
                return error();
        }
        symbol_flags[id] = symbol_flags[id] | 64;
        if (!known && token != ';')
            arity[id] = 0;
    } else {
        if (known && arity[id] != n + variadic * 128)
            return error();
        if (!known && (symbol_flags[id] & 64)) {
            if (variadic)
                return error();
            for (i = 0; i < n; i++)
                composite_type(parameter_type[id * 128 + i],
                               default_promoted(parameter_type[id * 128 + i]));
            if ((symbol_flags[id] & 1) && arity[id] != n)
                return error();
        }
        symbol_flags[id] = symbol_flags[id] | 4;
        arity[id] = n + variadic * 128;
    }
    expose_file_function(id, make_function(declaration_result,
                                           (old || unspecified) ? -1 : n + variadic * 128,
                                           parameter_decl_type));
    if (token == ',') {
        restore_locals(0);
        restore_tags(0);
        block_depth = 0;
        next();
        t = declarator(declaration_base, 0);
        id = decl_name;
        return global(id, t, declaration_base, storage);
    }
    if (token == ';') {
        next();
        restore_locals(0);
        restore_tags(0);
        block_depth = 0;
        return 0;
    }
function_body:
    if (definition_star_seen & 1)
        return error_message("star array is not allowed in a function definition");
    /* Definition bounds execute after all incoming arguments have been saved. */
    inline_body = id;
    if (id == 6 && (unqualified(t) != 1 || (n != 0 && n != 2) || variadic))
        return error();
    if (id == 6 && n == 2 &&
        (unqualified(parameter_decl_type[0]) != 1 ||
         unqualified(parameter_decl_type[1]) != make_pointer(make_pointer(9))))
        return error();
    if (unnamed || (symbol_flags[id] & 1) != 0)
        return error();
    symbol_flags[id] = symbol_flags[id] | 1;
    function_address[id] = position;
    for (i = 0; i < 2048; i = i + 1) {
        label_address[i] = 0;
        label_head[i] = 0;
        label_vm[i] = 0;
    }
    /* Prototype names need bindings, but only definitions need object storage. */
    local_bytes = 0;
    frame_peak = 0;
    for (i = 0; i < local_count; i++) {
        binding_bytes[i] = local_bytes;
        base = binding_name[i];
        if (base >= 0 && local_offset[base] > 0) {
            local_bytes += ((type_size(local_type[base]) + 7) / 8) * 8;
            if (local_bytes > 65536)
                return error();
            local_offset[base] = local_bytes;
        }
    }
    frame_peak = local_bytes;
    for (i = 0; i < n; i++)
        parameter_offset[i] = local_offset[parameter_name[i]];
    prologue();
    hidden = record_indirect(t);
    aggregate = hidden;
    current_variadic = variadic;
    current_oldstyle = old;
    current_va_offset = 16 + (n + hidden) * 8;
    if (variadic) {
        sequence(0x4d8948, 3);
        emit(16);
        sequence(0x558948, 3);
        emit(24);
        sequence(0x45894c, 3);
        emit(32);
        sequence(0x4d894c, 3);
        emit(40);
    }
    for (i = 0; i < n; i++)
        if (record_indirect(parameter_decl_type[i]) || floating_type(parameter_decl_type[i]))
            aggregate = 1;
    if (aggregate)
        aggregate_parameters(n, hidden);
    else {
        if (n >= 1) {
            emit(0x48);
            sequence(0xf84d89, 3);
        }
        if (n >= 2) {
            emit(0x48);
            sequence(0xf05589, 3);
        }
        if (n >= 3) {
            sequence(0x45894c, 3);
            emit(0xe8);
        }
        if (n >= 4) {
            sequence(0x4d894c, 3);
            emit(0xe0);
        }
        for (i = 4; i < n; i = i + 1) {
            sequence(0x858b48, 3);
            emit32(16 + i * 8);
            sequence(0x858948, 3);
            emit32(-(i + 1) * 8);
        }
    }
    for (i = 0; i < n; i++) {
        if (!aggregate && unqualified(parameter_decl_type[i]) == 12) {
            sequence(0x858b, 2);
            emit32(-parameter_offset[i]);
            expression_type = 1;
            convert(12);
            store(parameter_offset[i], 12);
        }
    }
    if (token != '{')
        return error();
    if (definition_star_seen & 2) {
        block_depth = 1;
        evaluate_parameter_declarators();
    }
    predefined_function_name(id);
    return_type = t;
    block_depth = 0;
    statement();
    for (i = 0; i < 2048; i = i + 1) {
        if (label_head[i])
            return error();
    }
    write32(frame_patch, ((frame_peak + 15) / 16) * 16);
    sequence(0xc031, 2);
    if (floating_type(t))
        floating_to_xmm(t, 0, 0);
    epilogue();
    resolve_vm_gotos();
    restore_locals(0);
    restore_tags(0);
    block_depth = 0;
    inline_body = -1;
    return 0;
}

int pe_name(int offset, char *name) {
    while (*name)
        image[offset++] = *name++;
    return 0;
}

int pe_import(int index, char *name, int cursor) {
    int n;
    write32(0x240 + index * 8, cursor + 0xe00);
    write32(0x480 + index * 8, cursor + 0xe00);
    pe_name(cursor + 2, name);
    n = 0;
    while (name[n])
        n++;
    return (cursor + n + 4) & -2;
}

int pe_header(void) {
    int i;
    write32(0, 0x5a4d);
    write32(0x3c, 0x80);
    write32(0x80, 0x4550);
    write32(0x84, 0x28664);
    write32(0x94, 0x2300f0);
    write32(0x98, 0x20b);
    write32(0xa4, 0x1a01000);
    write32(0xa8, 0x1e00);
    write32(0xac, 0x1000);
    write32(0xb0, 0x400000);
    write32(0xb8, 0x1000);
    write32(0xbc, 0x200);
    write32(0xc0, 6);
    write32(0xc8, 6);
    write32(0xd0, 0x1c01000);
    write32(0xd4, 0x200);
    write32(0xdc, 0x1000003);
    write32(0xe0, 0x100000);
    write32(0xe8, 0x1000);
    write32(0xf0, 0x100000);
    write32(0xf8, 0x1000);
    write32(0x104, 16);
    write32(0x110, 0x1000);
    write32(0x114, 40);
    write32(0x168, 0x1040);
    write32(0x16c, 312);
    write32(0x188, 0x7865742e);
    write32(0x18c, 0x74);
    write32(0x190, 0x1ff000);
    write32(0x194, 0x1000);
    write32(0x19c, 0x200);
    write32(0x1ac, -536870880);
    write32(0x1b0, 0x7373622e);
    write32(0x1b8, 0x1a01000);
    write32(0x1bc, 0x200000);
    write32(0x1d4, -1073741696);
    write32(0x200, 0x1280);
    write32(0x20c, 0x14a0);
    write32(0x210, 0x1040);
    pe_name(0x6a0, "KERNEL32.dll");
    i = 0x6c0;
    i = pe_import(0, "GetStdHandle", i);
    i = pe_import(1, "ReadFile", i);
    i = pe_import(2, "WriteFile", i);
    i = pe_import(3, "ExitProcess", i);
    i = pe_import(4, "CreateFileA", i);
    i = pe_import(5, "CloseHandle", i);
    i = pe_import(6, "GetEnvironmentVariableA", i);
    i = pe_import(7, "GetLocalTime", i);
    i = pe_import(8, "GetProcessHeap", i);
    i = pe_import(9, "HeapAlloc", i);
    i = pe_import(10, "HeapReAlloc", i);
    i = pe_import(11, "HeapFree", i);
    i = pe_import(12, "SetFilePointerEx", i);
    i = pe_import(13, "GetFileSizeEx", i);
    i = pe_import(14, "DeleteFileA", i);
    i = pe_import(15, "MoveFileExA", i);
    i = pe_import(16, "FlushFileBuffers", i);
    i = pe_import(17, "GetFileType", i);
    i = pe_import(18, "GetLastError", i);
    i = pe_import(19, "GetCommandLineA", i);
    i = pe_import(20, "GetSystemTimeAsFileTime", i);
    i = pe_import(21, "GetTimeZoneInformation", i);
    i = pe_import(22, "GetTickCount64", i);
    i = pe_import(23, "GetCurrentProcessId", i);
    i = pe_import(24, "GetTempPathA", i);
    i = pe_import(25, "GetTempFileNameA", i);
    i = pe_import(26, "GetConsoleMode", i);
    i = pe_import(27, "IsDBCSLeadByteEx", i);
    i = pe_import(28, "GetProcessTimes", i);
    i = pe_import(29, "GetCurrentProcess", i);
    i = pe_import(30, "GetDynamicTimeZoneInformation", i);
    i = pe_import(31, "GetTimeZoneInformationForYear", i);
    i = pe_import(32, "RemoveDirectoryA", i);
    i = pe_import(33, "GetFileAttributesA", i);
    i = pe_import(34, "GetSystemDirectoryA", i);
    i = pe_import(35, "CreateProcessA", i);
    i = pe_import(36, "WaitForSingleObject", i);
    i = pe_import(37, "GetExitCodeProcess", i);
    if (i > 0x1000)
        return error();
    position = 0x1000;
    sequence(0x28ec8348, 4);
    emit(0xe8);
    initializer_call = position;
    emit32(0);
    /* The reserved BSS callback runs after main, preserving its exit status.
       Libraries install a void(void) dispatcher at 0x2000010 when needed. */
    hosted_startup_call = call_count;
    call(6);
    sequence(0x20244489, 4);
    sequence(0x058b48, 3);
    relative(0x2000010);
    sequence(0xc08548, 3);
    i = jump(0x84);
    sequence(0xd0ff, 2);
    patch(i, position);
    sequence(0x20244c8b, 4);
    indirect(0x401058);
    emit(0xcc);
    return 0;
}

int runtime_function(int id) {
    int first, second;
    function_address[id] = position;
    sequence(0x38ec8348, 4);
    if (id == 11) {
        sequence(0x30244c89, 4);
        sequence(0x0d88, 2);
        relative(0x2000000);
        emit(0xb9);
        emit32(-11);
    } else {
        emit(0xb9);
        emit32(-10);
    }
    indirect(0x401040);
    sequence(0xc18948, 3);
    sequence(0x158d48, 3);
    relative(0x2000000);
    sequence(0xb841, 2);
    emit32(1);
    sequence(0x0d8d4c, 3);
    relative(0x2000004);
    sequence(0x2444c748, 4);
    emit(0x20);
    emit32(0);
    if (id == 11)
        indirect(0x401050);
    else
        indirect(0x401048);
    sequence(0xc085, 2);
    first = jump(0x84);
    sequence(0x058b, 2);
    relative(0x2000004);
    sequence(0x01f883, 3);
    second = jump(0x85);
    if (id == 11) {
        sequence(0x3024448b, 4);
        emit(0x25);
        emit32(255);
    } else {
        sequence(0x05b60f, 3);
        relative(0x2000000);
    }
    sequence(0x38c48348, 4);
    emit(0xc3);
    patch(first, position);
    patch(second, position);
    constant(-1);
    sequence(0x38c48348, 4);
    emit(0xc3);
    return 0;
}

/* Transactional type-aware pass for automatic inferred aggregate arrays.
   Interned identifier text is retained; semantic bindings and emitted code are restored. */
int initializer_backup[309248];
int initializer_state[58];
int initializer_checkpoint_depth;
int *initializer_checkpoint_stack[128];

int initializer_checkpoint_push(void) {
    typedef void *Heap(void);
    typedef void *Allocate(void *, unsigned int, unsigned long long);
    Heap *heap = (Heap *)__c1_win_GetProcessHeap;
    Allocate *allocate = (Allocate *)__c1_win_HeapAlloc;
    int *p, i;
    if (initializer_checkpoint_depth == 128)
        return error_message("initializer checkpoint nesting exceeded");
    if (initializer_checkpoint_depth) {
        p = allocate(heap(), 0, (309248ULL + 58) * 4);
        if (!p)
            return error_message("initializer checkpoint allocation failed");
        for (i = 0; i < 309248; i++)
            p[i] = initializer_backup[i];
        for (i = 0; i < 58; i++)
            p[309248 + i] = initializer_state[i];
        initializer_checkpoint_stack[initializer_checkpoint_depth] = p;
    }
    initializer_checkpoint_depth++;
    return 0;
}

int initializer_checkpoint_pop(void) {
    typedef void *Heap(void);
    typedef int Release(void *, unsigned int, void *);
    Heap *heap = (Heap *)__c1_win_GetProcessHeap;
    Release *release = (Release *)__c1_win_HeapFree;
    int *p, i;
    initializer_checkpoint_depth--;
    if (initializer_checkpoint_depth) {
        p = initializer_checkpoint_stack[initializer_checkpoint_depth];
        for (i = 0; i < 309248; i++)
            initializer_backup[i] = p[i];
        for (i = 0; i < 58; i++)
            initializer_state[i] = p[309248 + i];
        release(heap(), 0, p);
    }
    return 0;
}

int initializer_checkpoint(int save) {
    int i;
    if (save) {
        initializer_checkpoint_push();
        if (failed)
            return 0;
    }
    for (i = 0; i < 2048; i++) {
        if (save)
            initializer_backup[0 + i] = local_offset[i];
        else
            local_offset[i] = initializer_backup[0 + i];
        if (save)
            initializer_backup[2048 + i] = local_depth[i];
        else
            local_depth[i] = initializer_backup[2048 + i];
        if (save)
            initializer_backup[4096 + i] = local_type[i];
        else
            local_type[i] = initializer_backup[4096 + i];
        if (save)
            initializer_backup[6144 + i] = alias_type[i];
        else
            alias_type[i] = initializer_backup[6144 + i];
        if (save)
            initializer_backup[8192 + i] = register_object[i];
        else
            register_object[i] = initializer_backup[8192 + i];
        if (save)
            initializer_backup[10240 + i] = local_static_address[i];
        else
            local_static_address[i] = initializer_backup[10240 + i];
        if (save)
            initializer_backup[12288 + i] = global_type[i];
        else
            global_type[i] = initializer_backup[12288 + i];
        if (save)
            initializer_backup[14336 + i] = symbol_flags[i];
        else
            symbol_flags[i] = initializer_backup[14336 + i];
        if (save)
            initializer_backup[16384 + i] = global_visible[i];
        else
            global_visible[i] = initializer_backup[16384 + i];
        if (save)
            initializer_backup[18432 + i] = entity_type[i];
        else
            entity_type[i] = initializer_backup[18432 + i];
        if (save)
            initializer_backup[20480 + i] = enum_complete[i];
        else
            enum_complete[i] = initializer_backup[20480 + i];
        if (save)
            initializer_backup[22528 + i] = enum_global[i];
        else
            enum_global[i] = initializer_backup[22528 + i];
        if (save)
            initializer_backup[24576 + i] = enum_global_value[i];
        else
            enum_global_value[i] = initializer_backup[24576 + i];
        if (save)
            initializer_backup[26624 + i] = tag_type[i];
        else
            tag_type[i] = initializer_backup[26624 + i];
        if (save)
            initializer_backup[28672 + i] = tag_depth[i];
        else
            tag_depth[i] = initializer_backup[28672 + i];
        if (save)
            initializer_backup[30720 + i] = record_size[i];
        else
            record_size[i] = initializer_backup[30720 + i];
        if (save)
            initializer_backup[32768 + i] = record_align[i];
        else
            record_align[i] = initializer_backup[32768 + i];
        if (save)
            initializer_backup[34816 + i] = record_kind[i];
        else
            record_kind[i] = initializer_backup[34816 + i];
        if (save)
            initializer_backup[36864 + i] = record_open[i];
        else
            record_open[i] = initializer_backup[36864 + i];
        if (save)
            initializer_backup[38912 + i] = record_first[i];
        else
            record_first[i] = initializer_backup[38912 + i];
        if (save)
            initializer_backup[40960 + i] = record_last[i];
        else
            record_last[i] = initializer_backup[40960 + i];
        if (save)
            initializer_backup[43008 + i] = arity[i];
        else
            arity[i] = initializer_backup[43008 + i];
    }
    for (i = 0; i < 262144; i++) {
        if (save)
            initializer_backup[45056 + i] = parameter_type[i];
        else
            parameter_type[i] = initializer_backup[45056 + i];
    }
    for (i = 0; i < 2048; i++) {
        if (save)
            initializer_backup[307200 + i] = file_function_type[i];
        else
            file_function_type[i] = initializer_backup[307200 + i];
    }
    if (save)
        initializer_state[56] = literal_string_wide;
    else
        literal_string_wide = initializer_state[56];
    if (save)
        initializer_state[57] = joined_string_wide;
    else
        joined_string_wide = initializer_state[57];
    if (save)
        initializer_state[0] = cursor;
    else
        cursor = initializer_state[0];
    if (save)
        initializer_state[1] = line;
    else
        line = initializer_state[1];
    if (save)
        initializer_state[2] = string_used;
    else
        string_used = initializer_state[2];
    if (save)
        initializer_state[3] = token;
    else
        token = initializer_state[3];
    if (save)
        initializer_state[4] = value;
    else
        value = initializer_state[4];
    if (save)
        initializer_state[5] = literal_type;
    else
        literal_type = initializer_state[5];
    if (save)
        initializer_state[6] = literal_high;
    else
        literal_high = initializer_state[6];
    if (save)
        initializer_state[7] = literal_string_start;
    else
        literal_string_start = initializer_state[7];
    if (save)
        initializer_state[8] = literal_string_length;
    else
        literal_string_length = initializer_state[8];
    if (save)
        initializer_state[9] = joined_string_start;
    else
        joined_string_start = initializer_state[9];
    if (save)
        initializer_state[10] = joined_string_length;
    else
        joined_string_length = initializer_state[10];
    if (save)
        initializer_state[11] = local_count;
    else
        local_count = initializer_state[11];
    if (save)
        initializer_state[12] = tag_count;
    else
        tag_count = initializer_state[12];
    if (save)
        initializer_state[13] = array_count;
    else
        array_count = initializer_state[13];
    if (save)
        initializer_state[14] = enum_count;
    else
        enum_count = initializer_state[14];
    if (save)
        initializer_state[15] = record_count;
    else
        record_count = initializer_state[15];
    if (save)
        initializer_state[16] = member_count;
    else
        member_count = initializer_state[16];
    if (save)
        initializer_state[17] = local_bytes;
    else
        local_bytes = initializer_state[17];
    if (save)
        initializer_state[18] = frame_peak;
    else
        frame_peak = initializer_state[18];
    if (save)
        initializer_state[19] = decl_storage;
    else
        decl_storage = initializer_state[19];
    if (save)
        initializer_state[20] = decl_typedef;
    else
        decl_typedef = initializer_state[20];
    if (save)
        initializer_state[21] = decl_name;
    else
        decl_name = initializer_state[21];
    if (save)
        initializer_state[22] = decl_count;
    else
        decl_count = initializer_state[22];
    if (save)
        initializer_state[23] = decl_depth;
    else
        decl_depth = initializer_state[23];
    if (save)
        initializer_state[24] = decl_parentheses;
    else
        decl_parentheses = initializer_state[24];
    if (save)
        initializer_state[25] = declarator_active;
    else
        declarator_active = initializer_state[25];
    if (save)
        initializer_state[26] = initializer_depth;
    else
        initializer_depth = initializer_state[26];
    if (save)
        initializer_state[27] = initializer_pending;
    else
        initializer_pending = initializer_state[27];
    if (save)
        initializer_state[28] = initializer_probe;
    else
        initializer_probe = initializer_state[28];
    if (save)
        initializer_state[29] = inferred_elements;
    else
        inferred_elements = initializer_state[29];
    if (save)
        initializer_state[30] = record_depth;
    else
        record_depth = initializer_state[30];
    if (save)
        initializer_state[31] = expression_depth;
    else
        expression_depth = initializer_state[31];
    if (save)
        initializer_state[32] = unary_depth;
    else
        unary_depth = initializer_state[32];
    if (save)
        initializer_state[33] = unevaluated_depth;
    else
        unevaluated_depth = initializer_state[33];
    if (save)
        initializer_state[34] = ce_depth;
    else
        ce_depth = initializer_state[34];
    if (save)
        initializer_state[35] = ce_type;
    else
        ce_type = initializer_state[35];
    if (save)
        initializer_state[36] = expression_type;
    else
        expression_type = initializer_state[36];
    if (save)
        initializer_state[37] = sizeof_value;
    else
        sizeof_value = initializer_state[37];
    if (save)
        initializer_state[38] = expression_zero;
    else
        expression_zero = initializer_state[38];
    if (save)
        initializer_state[39] = expression_load;
    else
        expression_load = initializer_state[39];
    if (save)
        initializer_state[40] = expression_array_bytes;
    else
        expression_array_bytes = initializer_state[40];
    if (save)
        initializer_state[41] = expression_array_type;
    else
        expression_array_type = initializer_state[41];
    if (save)
        initializer_state[42] = expression_register;
    else
        expression_register = initializer_state[42];
    if (save)
        initializer_state[43] = call_count;
    else
        call_count = initializer_state[43];
    if (save)
        initializer_state[44] = object_ref_count;
    else
        object_ref_count = initializer_state[44];
    if (save)
        initializer_state[45] = position;
    else
        position = initializer_state[45];
    if (save)
        initializer_state[46] = decl_result_name;
    else
        decl_result_name = initializer_state[46];
    if (save)
        initializer_state[47] = function_type_count;
    else
        function_type_count = initializer_state[47];
    if (save)
        initializer_state[48] = qualified_count;
    else
        qualified_count = initializer_state[48];
    if (save)
        initializer_state[55] = initializer_designated;
    else
        initializer_designated = initializer_state[55];
    if (save)
        initializer_state[54] = decl_inline;
    else
        decl_inline = initializer_state[54];
    if (save)
        initializer_state[49] = expression_record_ready;
    else
        expression_record_ready = initializer_state[49];
    if (save)
        initializer_state[50] = expression_bit;
    else
        expression_bit = initializer_state[50];
    if (save)
        initializer_state[51] = expression_ice;
    else
        expression_ice = initializer_state[51];
    if (save)
        initializer_state[52] = (int)expression_value;
    else
        expression_value = (unsigned int)initializer_state[52];
    if (save)
        initializer_state[53] = (int)(expression_value >> 32);
    else
        expression_value |= (unsigned long long)(unsigned int)initializer_state[53] << 32;
    if (!save)
        initializer_checkpoint_pop();
    return 0;
}

int infer_aggregate_array(int t, int id) {
    int start = position, old_members = member_count, n, i;
    unsigned long long saved_value = ce_value;
    initializer_checkpoint(1);
    if (failed)
        return 0;
    initializer_probe = 1;
    initializer_pending = 0;
    inferred_elements = 0;
    initializer_depth = 0;
    initializer_designated = 0;
    if (id >= 0) {
        local_offset[id] = -2;
        local_type[id] = t;
        local_depth[id] = block_depth;
        alias_type[id] = 0;
        register_object[id] = 0;
        expect('=');
    }
    initialize_object(t, 0, 0, 0, 0);
    n = inferred_elements;
    for (i = old_members + 1; i <= member_count; i++)
        member_next[i] = 0;
    rewind_code(start);
    initializer_checkpoint(0);
    ce_value = saved_value;
    if (failed || !n)
        return error();
    return make_array(array_base[(t - 4096) / 256], n);
}

/* Preprocessing tokens retain their spelling and whitespace boundaries.
   The input arena is immutable while expanded tokens are written to source. */
int pp_input[1048576];
int pp_input_size;
int pp_cursor;
int pp_output;
int pp_start;
int pp_length;
int pp_kind;
int pp_space;
int pp_tokens;
int pp_token_start[65536];
int pp_token_length[65536];
int pp_token_kind[65536];
int pp_token_space[65536];
int pp_macros;
int pp_name_start[1024];
int pp_name_length[1024];
int pp_body_start[1024];
int pp_body_length[1024];
int pp_defined[1024];

int pp_depth;
int pp_enabled;
int pp_parent[128];
int pp_taken[128];
int pp_else[128];

int pp_arity[1024];
int pp_variadic[1024];
int pp_allow_va;
int pp_parameter_start[1024];
int pp_nodes;
int pp_free;
int pp_node_start[65536];
int pp_node_length[65536];
int pp_node_kind[65536];
int pp_node_space[65536];
int pp_node_hide[65536];
int pp_node_next[65536];
int pp_node_line[65536];
int pp_node_file[65536];
int pp_node_file_length[65536];
int pp_hides;
int pp_hide_name[131072];
int pp_hide_parent[131072];
int pp_hide_next[131072];
int pp_hide_bucket[65536];
int pp_beginning;
int pp_arena_used;
int pp_file_depth;
int pp_file_eof;
int pp_file_cursor[64];
int pp_file_end[64];
int pp_file_condition[64];
int pp_file_physical_end[64];
int pp_file_line_delta[64];
int pp_file_display[64];
int pp_file_display_length[64];
int pp_current_line;
int pp_clock_ready;
int pp_date_start;
int pp_date_length;
int pp_time_start;
int pp_time_length;
char pp_file_path[65536];
char pp_include_name[1024];
char pp_candidate[1024];
char pp_read_buffer[4096];
char pp_include_env[4096];

/* Generated spellings temporarily use image, before PE generation begins. */
int pp_generated;
int *pp_text;

int pp_char(int start, int offset) {
    if (start >= 0)
        return pp_input[start + offset] & 255;
    if (start == -1)
        return '0';
    if (start == -2)
        return '1';
    return image[-start - 3 + offset];
}

int pp_generate(int c) {
    if (pp_generated == 1049088)
        return error();
    image[pp_generated++] = c;
    return 0;
}

int pp_display_name(char *name) {
    int start = pp_generated, c;
    pp_generate(34);
    while (*name && !failed) {
        c = (unsigned char)*name++;
        if (c == 34 || c == 92)
            pp_generate(92);
        pp_generate(c);
    }
    pp_generate(34);
    pp_file_display[pp_file_depth] = -start - 3;
    pp_file_display_length[pp_file_depth] = pp_generated - start;
    return 0;
}

int pp_spelling(int start, int length, char *word) {
    int i = 0;
    while (word[i]) {
        if (i == length || pp_char(start, i) != word[i])
            return 0;
        i++;
    }
    return i == length;
}

int pp_builtin(int start, int length) {
    if (pp_spelling(start, length, "__LINE__"))
        return 1;
    if (pp_spelling(start, length, "__FILE__"))
        return 2;
    if (pp_spelling(start, length, "__DATE__"))
        return 3;
    if (pp_spelling(start, length, "__TIME__"))
        return 4;
    if (pp_spelling(start, length, "__STDC__"))
        return 5;
    if (pp_spelling(start, length, "__STDC_HOSTED__"))
        return 6;
    if (c99_mode && pp_spelling(start, length, "__STDC_VERSION__"))
        return 7;
    if (c89_identifiers && pp_spelling(start, length, "__C1_C89__"))
        return 8;
    return 0;
}

typedef void PP_LocalTime(unsigned short *);

int pp_clock_digits(int value, int width) {
    int digits[12], count = 0;
    do {
        digits[count++] = value % 10;
        value /= 10;
    } while (value);
    while (width > count) {
        pp_generate('0');
        width--;
    }
    while (count)
        pp_generate('0' + digits[--count]);
    return 0;
}

int pp_clock_valid(unsigned short *v) {
    int days = 31, year = v[0], month = v[1];
    if (!year || year > 9999 || month < 1 || month > 12 || v[4] > 23 || v[5] > 59 || v[6] > 59)
        return 0;
    if (month == 4 || month == 6 || month == 9 || month == 11)
        days = 30;
    if (month == 2)
        days = 28 + (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    return v[3] >= 1 && v[3] <= days;
}

int pp_clock_format(unsigned short *clock_value) {
    char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int i, start = pp_generated;
    if (!pp_clock_valid(clock_value))
        return error();
    pp_generate(34);
    for (i = 0; i < 3; i++)
        pp_generate(months[(clock_value[1] - 1) * 3 + i]);
    pp_generate(' ');
    if (clock_value[3] < 10) {
        pp_generate(' ');
        pp_clock_digits(clock_value[3], 1);
    } else
        pp_clock_digits(clock_value[3], 2);
    pp_generate(' ');
    pp_clock_digits(clock_value[0], 4);
    pp_generate(34);
    pp_date_start = -start - 3;
    pp_date_length = pp_generated - start;
    start = pp_generated;
    pp_generate(34);
    pp_clock_digits(clock_value[4], 2);
    pp_generate(':');
    pp_clock_digits(clock_value[5], 2);
    pp_generate(':');
    pp_clock_digits(clock_value[6], 2);
    pp_generate(34);
    pp_time_start = -start - 3;
    pp_time_length = pp_generated - start;
    return 0;
}

int pp_clock_read(PP_LocalTime *local_time) {
    unsigned short clock_value[8];
    int i;
    if (pp_clock_ready)
        return 0;
    for (i = 0; i < 8; i++)
        clock_value[i] = 0;
    local_time(clock_value);
    if (!pp_clock_valid(clock_value)) {
        for (i = 0; i < 8; i++)
            clock_value[i] = 0;
        clock_value[0] = 1970;
        clock_value[1] = 1;
        clock_value[3] = 1;
    }
    pp_clock_format(clock_value);
    if (!failed)
        pp_clock_ready = 1;
    return 0;
}

int pp_clock(void) {
    return pp_clock_read((PP_LocalTime *)__c1_win_GetLocalTime);
}

int pp_expand_builtin(int n, int kind) {
    unsigned int value = pp_node_line[n];
    int start = pp_generated, digits[12], count = 0;
    if (kind == 3 || kind == 4) {
        pp_clock();
        pp_node_start[n] = kind == 3 ? pp_date_start : pp_time_start;
        pp_node_length[n] = kind == 3 ? pp_date_length : pp_time_length;
        pp_node_kind[n] = 3;
    } else if (kind == 2) {
        pp_node_start[n] = pp_node_file[n];
        pp_node_length[n] = pp_node_file_length[n];
        pp_node_kind[n] = 3;
    } else {
        if (kind == 5 || kind == 6 || kind == 8)
            value = 1;
        if (kind == 7)
            value = 199901;
        if (!value)
            return error();
        do {
            digits[count++] = value % 10;
            value /= 10;
        } while (value);
        while (count)
            pp_generate('0' + digits[--count]);
        if (kind == 7)
            pp_generate('L');
        pp_node_start[n] = -start - 3;
        pp_node_length[n] = pp_generated - start;
        pp_node_kind[n] = 2;
    }
    return 0;
}

int pp_token_word(int index, char *word) {
    int i = 0;
    while (word[i]) {
        if (i >= pp_token_length[index] || pp_char(pp_token_start[index], i) != word[i])
            return 0;
        i++;
    }
    return i == pp_token_length[index];
}

int pp_paste_token(int index) {
    return pp_token_word(index, "##") || pp_token_word(index, "%:%:");
}

int pp_parameter(int id, int index) {
    int j, p;
    if (pp_token_kind[index] != 1)
        return -1;
    if (pp_variadic[id] && pp_token_word(index, "__VA_ARGS__"))
        return pp_arity[id] - 1;
    for (j = 0; j < pp_arity[id]; j++) {
        p = pp_parameter_start[id] + j;
        if (pp_equal(pp_token_start[index], pp_token_length[index], pp_token_start[p],
                     pp_token_length[p]))
            return j;
    }
    return -1;
}

int pp_clone_list(int head, int hide) {
    int result = 0, last = 0, copy;
    while (head && !failed) {
        copy = pp_node(pp_node_start[head], pp_node_length[head], pp_node_kind[head],
                       pp_node_space[head], pp_hide_union(pp_node_hide[head], hide));
        pp_origin(copy, head);
        if (last)
            pp_node_next[last] = copy;
        else
            result = copy;
        last = copy;
        head = pp_node_next[head];
    }
    return result;
}

int pp_stringize(int head, int space, int hide) {
    int start = pp_generated, first = 1, i, c;
    pp_generate(34);
    while (head && !failed) {
        if (!first && pp_node_space[head])
            pp_generate(32);
        for (i = 0; i < pp_node_length[head] && !failed; i++) {
            c = pp_char(pp_node_start[head], i);
            if ((pp_node_kind[head] == 3 || pp_node_kind[head] == 4) && (c == 34 || c == 92))
                pp_generate(92);
            pp_generate(c);
        }
        first = 0;
        head = pp_node_next[head];
    }
    pp_generate(34);
    return pp_node(-start - 3, pp_generated - start, 3, space, hide);
}

int pp_retoken(int start) {
    int *old_text = pp_text;
    int old_size = pp_input_size, old_cursor = pp_cursor, old_start = pp_start,
        old_length = pp_length, old_kind = pp_kind, old_space = pp_space, result;
    pp_text = image;
    pp_input_size = pp_generated;
    pp_cursor = start;
    pp_scan();
    result = pp_kind;
    if (!result || result == 6 || pp_start != start || pp_cursor != pp_generated ||
        pp_length != pp_generated - start)
        error();
    pp_text = old_text;
    pp_input_size = old_size;
    pp_cursor = old_cursor;
    pp_start = old_start;
    pp_length = old_length;
    pp_kind = old_kind;
    pp_space = old_space;
    return result;
}

int pp_paste(int left, int right) {
    int start = pp_generated, i, kind;
    if (pp_node_kind[left] == 7) {
        pp_node_start[left] = pp_node_start[right];
        pp_node_length[left] = pp_node_length[right];
        pp_node_kind[left] = pp_node_kind[right];
        pp_node_hide[left] = pp_node_hide[right];
        return 0;
    }
    if (pp_node_kind[right] == 7)
        return 0;
    for (i = 0; i < pp_node_length[left] && !failed; i++)
        pp_generate(pp_char(pp_node_start[left], i));
    for (i = 0; i < pp_node_length[right] && !failed; i++)
        pp_generate(pp_char(pp_node_start[right], i));
    if (failed)
        return 0;
    kind = pp_retoken(start);
    pp_node_start[left] = -start - 3;
    pp_node_length[left] = pp_generated - start;
    pp_node_kind[left] = kind;
    pp_node_hide[left] = pp_hide_intersection(pp_node_hide[left], pp_node_hide[right]);
    return 0;
}

int pp_finish_pastes(int head) {
    int n = head, previous = 0, right, following;
    while (n && !failed) {
        if (pp_node_kind[n] == 8) {
            right = pp_node_next[n];
            if (!previous || !right || pp_node_kind[right] == 8)
                return error();
            following = pp_node_next[right];
            pp_paste(previous, right);
            pp_origin(previous, n);
            pp_node_next[previous] = following;
            pp_release(n);
            pp_release(right);
            n = following;
        } else {
            previous = n;
            n = pp_node_next[n];
        }
    }
    n = head;
    previous = 0;
    while (n && !failed) {
        following = pp_node_next[n];
        if (pp_node_kind[n] == 7) {
            if (following && pp_node_space[n])
                pp_node_space[following] = 1;
            if (previous)
                pp_node_next[previous] = following;
            else
                head = following;
            pp_release(n);
        } else
            previous = n;
        n = following;
    }
    return head;
}

int pp_hidden(int set, int id) {
    while (set) {
        if (pp_hide_name[set] == id)
            return 1;
        set = pp_hide_parent[set];
    }
    return 0;
}

int pp_hide_add(int set, int id) {
    int bucket, i;
    if (pp_hidden(set, id))
        return set;
    bucket = (id * 131 + set * 17) & 65535;
    for (i = pp_hide_bucket[bucket]; i; i = pp_hide_next[i])
        if (pp_hide_name[i] == id && pp_hide_parent[i] == set)
            return i;
    if (pp_hides == 131071)
        return error();
    i = ++pp_hides;
    pp_hide_name[i] = id;
    pp_hide_parent[i] = set;
    pp_hide_next[i] = pp_hide_bucket[bucket];
    pp_hide_bucket[bucket] = i;
    return i;
}

int pp_hide_union(int a, int b) {
    while (b && !failed) {
        a = pp_hide_add(a, pp_hide_name[b]);
        b = pp_hide_parent[b];
    }
    return a;
}

int pp_hide_intersection(int a, int b) {
    int result = 0;
    while (a && !failed) {
        if (pp_hidden(b, pp_hide_name[a]))
            result = pp_hide_add(result, pp_hide_name[a]);
        a = pp_hide_parent[a];
    }
    return result;
}

int pp_hide_reset(void) {
    int i;
    for (i = 1; i <= pp_hides; i++)
        pp_hide_bucket[(pp_hide_name[i] * 131 + pp_hide_parent[i] * 17) & 65535] = 0;
    pp_hides = 0;
    return 0;
}

int pp_origin(int target, int from) {
    pp_node_line[target] = pp_node_line[from];
    pp_node_file[target] = pp_node_file[from];
    pp_node_file_length[target] = pp_node_file_length[from];
    return 0;
}

int pp_node(int start, int length, int kind, int space, int hide) {
    int n;
    if (pp_free) {
        n = pp_free;
        pp_free = pp_node_next[n];
    } else {
        if (pp_nodes == 65535)
            return error();
        n = ++pp_nodes;
    }
    pp_node_start[n] = start;
    pp_node_length[n] = length;
    pp_node_kind[n] = kind;
    pp_node_line[n] = pp_current_line;
    pp_node_file[n] = pp_file_display[pp_file_depth];
    pp_node_file_length[n] = pp_file_display_length[pp_file_depth];
    pp_node_space[n] = space;
    pp_node_hide[n] = hide;
    pp_node_next[n] = 0;
    return n;
}

int pp_release(int n) {
    pp_node_next[n] = pp_free;
    pp_free = n;
    return 0;
}

int pp_release_list(int n) {
    int following;
    while (n) {
        following = pp_node_next[n];
        pp_release(n);
        n = following;
    }
    return 0;
}

int pp_node_word(int n, char *word) {
    int i = 0, start = pp_node_start[n];

    while (word[i]) {
        if (i >= pp_node_length[n] || pp_char(start, i) != word[i])
            return 0;
        i++;
    }
    return i == pp_node_length[n];
}

int pp_stream_take(int *head, int source_ok) {
    int n, newline = 0;
    if (*head) {
        n = *head;
        *head = pp_node_next[n];
        pp_node_next[n] = 0;
        return n;
    }
    if (!source_ok || failed)
        return 0;
    while (pp_kind == 6 && !failed) {
        pp_beginning = 1;
        newline = 1;
        pp_scan();
    }
    if (!pp_kind || (pp_beginning && (pp_word("#") || pp_word("%:"))))
        return 0;
    pp_beginning = 0;
    n = pp_node(pp_start, pp_length, pp_kind, pp_space || newline, 0);
    pp_scan();
    return n;
}

int pp_expand_stream(int head, int source_ok, int depth, int emit_output);

int pp_replace(int id, int invocation, int *queue, int source_ok, int depth) {
    int args[128], expanded[128], argc = 0, tail = 0, n, nesting = 0, hide, i, j, parameter, end;
    int result = 0, last = 0, copy, p, closing = 0;
    for (i = 0; i < 128; i++) {
        args[i] = 0;
        expanded[i] = -1;
    }
    if (pp_arity[id] >= 0) {
        argc = 1;
        while (!failed) {
            n = pp_stream_take(queue, source_ok);
            if (!n)
                return error();
            if (pp_node_word(n, ")") && !nesting) {
                closing = n;
                break;
            }
            if (pp_node_word(n, ",") && !nesting && (!pp_variadic[id] || argc < pp_arity[id])) {
                pp_release(n);
                if (argc == 128 || (!pp_variadic[id] && argc == 127))
                    return error();
                argc++;
                tail = 0;
                continue;
            }
            if (pp_node_word(n, "("))
                nesting++;
            if (pp_node_word(n, ")"))
                nesting--;
            if (tail)
                pp_node_next[tail] = n;
            else
                args[argc - 1] = n;
            tail = n;
        }
        if (argc == 1 && !args[0] && !pp_arity[id])
            argc = 0;
        if (argc != pp_arity[id])
            return error();
        hide = pp_hide_intersection(pp_node_hide[invocation], pp_node_hide[closing]);
        pp_release(closing);
    } else
        hide = pp_node_hide[invocation];
    hide = pp_hide_add(hide, id);
    end = pp_body_start[id] + pp_body_length[id];
    for (i = pp_body_start[id]; i < end && !failed; i++) {
        parameter = pp_parameter(id, i);
        copy = 0;
        if (argc >= 0 && pp_arity[id] >= 0 && (pp_token_word(i, "#") || pp_token_word(i, "%:"))) {
            parameter = pp_parameter(id, ++i);
            copy = pp_stringize(args[parameter], pp_token_space[i - 1], hide);
            pp_origin(copy, invocation);
        } else if (pp_paste_token(i)) {
            copy = pp_node(pp_token_start[i], pp_token_length[i], 8, pp_token_space[i], hide);
            pp_origin(copy, invocation);
        } else if (parameter >= 0) {
            if ((i > pp_body_start[id] && pp_paste_token(i - 1)) ||
                (i + 1 < end && pp_paste_token(i + 1)))
                n = args[parameter];
            else {
                if (expanded[parameter] < 0)
                    expanded[parameter] =
                        pp_expand_stream(pp_clone_list(args[parameter], 0), 0, depth + 1, 0);
                n = expanded[parameter];
            }
            copy = pp_clone_list(n, hide);
            if (copy)
                pp_node_space[copy] = pp_token_space[i];
            else {
                copy = pp_node(0, 0, 7, pp_token_space[i], hide);
                pp_origin(copy, invocation);
            }
        } else {
            copy = pp_node(pp_token_start[i], pp_token_length[i], pp_token_kind[i],
                           pp_token_space[i], hide);
            pp_origin(copy, invocation);
        }
        if (last)
            pp_node_next[last] = copy;
        else
            result = copy;
        while (copy) {
            last = copy;
            copy = pp_node_next[copy];
        }
    }
    if (result)
        pp_node_space[result] = pp_node_space[invocation];
    result = pp_finish_pastes(result);
    last = 0;
    for (n = result; n; n = pp_node_next[n])
        last = n;
    if (result)
        pp_node_space[result] = pp_node_space[invocation];
    if (!result && pp_node_space[invocation]) {
        n = pp_stream_take(queue, source_ok);
        if (n) {
            pp_node_space[n] = 1;
            pp_node_next[n] = *queue;
            *queue = n;
        }
    }
    for (i = 0; i < argc; i++) {
        pp_release_list(args[i]);
        if (expanded[i] >= 0)
            pp_release_list(expanded[i]);
    }
    if (last)
        pp_node_next[last] = *queue;
    else
        result = *queue;
    return result;
}

int pp_pragma_apply(int head) {
    int n;
    if (!head || !pp_node_word(head, "STDC"))
        return 0;
    n = pp_node_next[head];
    if (!n || (!pp_node_word(n, "FP_CONTRACT") && !pp_node_word(n, "FENV_ACCESS") &&
               !pp_node_word(n, "CX_LIMITED_RANGE")))
        return error();
    n = pp_node_next[n];
    if (!n || (!pp_node_word(n, "ON") && !pp_node_word(n, "OFF") && !pp_node_word(n, "DEFAULT")) ||
        pp_node_next[n])
        return error();
    /* Floating semantics are not implemented yet. This validates the form. */
    return 0;
}

int pp_pragma_payload(int literal, int invocation) {
    int i = 1, c, start = pp_generated, head = 0, tail = 0, n;
    int *old_text = pp_text;
    int old_size = pp_input_size, old_cursor = pp_cursor, old_start = pp_start;
    int old_length = pp_length, old_kind = pp_kind, old_space = pp_space;
    if (pp_char(pp_node_start[literal], 0) == 'L')
        i = 2;
    for (; i < pp_node_length[literal] - 1 && !failed; i++) {
        c = pp_char(pp_node_start[literal], i);
        if (c == 92 && i + 1 < pp_node_length[literal] - 1 &&
            (pp_char(pp_node_start[literal], i + 1) == 92 ||
             pp_char(pp_node_start[literal], i + 1) == 34))
            c = pp_char(pp_node_start[literal], ++i);
        pp_generate(c);
    }
    pp_text = image;
    pp_input_size = pp_generated;
    pp_cursor = start;
    while (!failed) {
        pp_scan_raw();
        if (!pp_kind)
            break;
        if (pp_kind == 6)
            continue;
        n = pp_node(-pp_start - 3, pp_length, pp_kind, pp_space, 0);
        pp_origin(n, invocation);
        if (tail)
            pp_node_next[tail] = n;
        else
            head = n;
        tail = n;
    }
    pp_text = old_text;
    pp_input_size = old_size;
    pp_cursor = old_cursor;
    pp_start = old_start;
    pp_length = old_length;
    pp_kind = old_kind;
    pp_space = old_space;
    if (!failed)
        pp_pragma_apply(head);
    pp_release_list(head);
    return 0;
}

int pp_expanded_take(int *queue, int source_ok, int depth);

int pp_pragma_operator(int *queue, int source_ok, int depth, int invocation) {
    int opening, literal, closing, next_node, space = pp_node_space[invocation];
    opening = pp_expanded_take(queue, source_ok, depth);
    if (!opening || !pp_node_word(opening, "("))
        return error_message("expected opening parenthesis after _Pragma");
    space = space || pp_node_space[opening];
    pp_release(opening);
    literal = pp_expanded_take(queue, source_ok, depth);
    if (!literal || pp_node_kind[literal] != 3)
        return error_message("expected string literal in _Pragma");
    space = space || pp_node_space[literal];
    closing = pp_expanded_take(queue, source_ok, depth);
    if (!closing || !pp_node_word(closing, ")"))
        return error_message("expected closing parenthesis after _Pragma string");
    space = space || pp_node_space[closing];
    pp_release(closing);
    pp_pragma_payload(literal, invocation);
    pp_release(literal);
    if (space && !failed) {
        next_node = pp_stream_take(queue, source_ok);
        if (next_node) {
            pp_node_space[next_node] = 1;
            pp_node_next[next_node] = *queue;
            *queue = next_node;
        }
    }
    return 0;
}

int pp_expanded_take(int *queue, int source_ok, int depth) {
    int n, id, look, builtin;
    if (depth > 128)
        return error();
    while (!failed) {
        n = pp_stream_take(queue, source_ok);
        if (!n)
            break;
        if (pp_node_kind[n] == 1 && pp_node_word(n, "__VA_ARGS__"))
            return error();
        if (pp_node_kind[n] == 1 && pp_node_word(n, "_Pragma")) {
            pp_pragma_operator(queue, source_ok, depth + 1, n);
            pp_release(n);
            continue;
        }
        if (pp_node_kind[n] == 1) {
            builtin = pp_builtin(pp_node_start[n], pp_node_length[n]);
            if (builtin)
                pp_expand_builtin(n, builtin);
        }
        id = -1;
        if (pp_node_kind[n] == 1)
            id = pp_find(pp_node_start[n], pp_node_length[n]);
        if (id >= 0 && pp_defined[id] && !pp_hidden(pp_node_hide[n], id)) {
            if (pp_arity[id] >= 0) {
                look = pp_stream_take(queue, source_ok);
                if (look && pp_node_word(look, "("))
                    pp_release(look);
                else {
                    if (look) {
                        pp_node_next[look] = *queue;
                        *queue = look;
                    }
                    id = -1;
                }
            }
            if (id >= 0) {
                *queue = pp_replace(id, n, queue, source_ok, depth);
                pp_release(n);
                continue;
            }
        }
        return n;
    }
    return 0;
}

int pp_expand_stream(int head, int source_ok, int depth, int emit_output) {
    int n, result = 0, last = 0;
    while (!failed) {
        n = pp_expanded_take(&head, source_ok, depth);
        if (!n)
            break;
        if (emit_output) {
            pp_emit(pp_node_start[n], pp_node_length[n], pp_node_kind[n]);
            pp_release(n);
        } else {
            if (last)
                pp_node_next[last] = n;
            else
                result = n;
            last = n;
        }
    }
    return result;
}

int pp_unit_length;

int pp_identifier_unit(int start, int length, int at) {
    unsigned int code = 0;
    int c, n, i, d;
    if (at == length)
        return -2;
    c = pp_char(start, at);
    pp_unit_length = 1;
    if (alpha(c) || (c >= '0' && c <= '9'))
        return c;
    if (c != 92 || at + 1 >= length)
        return -1;
    c = pp_char(start, at + 1);
    if (c != 'u' && c != 'U')
        return -1;
    n = c == 'u' ? 4 : 8;
    if (at + 2 + n > length)
        return -1;
    for (i = 0; i < n; i++) {
        d = digit(pp_char(start, at + 2 + i));
        if (d < 0)
            return -1;
        code = code * 16 + (unsigned int)d;
    }
    if (!ucn_identifier_class(code))
        return -1;
    pp_unit_length = n + 2;
    return (int)code;
}

int pp_equal(int a, int an, int b, int bn) {
    int i, raw = an == bn, ai = 0, bi = 0, ac, bc;
    for (i = 0; i < an && raw; i++)
        if (pp_char(a, i) != pp_char(b, i))
            raw = 0;
    if (raw)
        return 1;
    /* Only complete identifier spellings compare by character value.
       Literal and punctuator spellings keep their original byte equality. */
    while (ai < an && bi < bn) {
        ac = pp_identifier_unit(a, an, ai);
        ai += pp_unit_length;
        bc = pp_identifier_unit(b, bn, bi);
        bi += pp_unit_length;
        if (ac < 0 || bc < 0 || ac != bc)
            return 0;
    }
    return ai == an && bi == bn;
}

int pp_word(char *word) {
    int i = 0;
    while (word[i]) {
        if (i >= pp_length || word[i] != (pp_input[pp_start + i] & 255))
            return 0;
        i++;
    }
    return i == pp_length;
}

int pp_at(int at) {
    return pp_text[at] & 255;
}

int pp_match(int at, char *word) {
    int i = 0;
    while (word[i]) {
        if (at + i >= pp_input_size || pp_at(at + i) != word[i])
            return 0;
        i++;
    }
    return i;
}

int pp_directive_space;

int pp_scan_raw(void) {
    int c, d, quote, n;
    pp_kind = 0;
    pp_space = 0;
    while (pp_cursor < pp_input_size && !failed) {
        c = pp_at(pp_cursor);
        if (c == ' ' || c == 9 || c == 11 || c == 12) {
            if (pp_directive_space && pp_text == pp_input && (c == 11 || c == 12))
                return error_message("invalid whitespace in preprocessing directive");
            pp_cursor++;
            pp_space = 1;
            continue;
        }
        if (!c89_identifiers && pp_match(pp_cursor, "//")) {
            while (pp_cursor < pp_input_size && pp_at(pp_cursor) != 10)
                pp_cursor++;
            pp_space = 1;
            continue;
        }
        if (pp_match(pp_cursor, "/*")) {
            pp_cursor += 2;
            while (pp_cursor < pp_input_size && !pp_match(pp_cursor, "*/"))
                pp_cursor++;
            if (pp_cursor == pp_input_size)
                return error();
            pp_cursor += 2;
            pp_space = 1;
            continue;
        }
        break;
    }
    if (failed || pp_cursor == pp_input_size)
        return 0;
    pp_start = pp_cursor;
    c = pp_at(pp_cursor++);
    pp_kind = 5;
    if (c == 10)
        pp_kind = 6;
    else if (c < 32)
        return error();
    else if (c == 39 || c == 34 ||
             (c == 'L' && pp_cursor < pp_input_size &&
              (pp_at(pp_cursor) == 39 || pp_at(pp_cursor) == 34))) {
        if (c == 'L')
            c = pp_at(pp_cursor++);
        quote = c;
        pp_kind = c == 34 ? 3 : 4;
        while (1) {
            if (pp_cursor == pp_input_size)
                return error();
            c = pp_at(pp_cursor++);
            if (c == 10)
                return error();
            if (c == quote)
                break;
            if (c == 92) {
                if (pp_cursor == pp_input_size)
                    return error();
                pp_cursor++;
            }
        }
    } else if ((c >= '0' && c <= '9') || (c == '.' && pp_cursor < pp_input_size &&
                                          pp_at(pp_cursor) >= '0' && pp_at(pp_cursor) <= '9')) {
        pp_kind = 2;
        while (pp_cursor < pp_input_size) {
            d = pp_at(pp_cursor);
            if (d == 92 && pp_cursor + 1 < pp_input_size &&
                (pp_at(pp_cursor + 1) == 'u' || pp_at(pp_cursor + 1) == 'U')) {
                identifier_ucn(pp_text, pp_input_size, &pp_cursor, 0);
                if (failed)
                    return error_message(
                        "invalid universal character name in preprocessing number");
                c = 0;
                continue;
            }
            if (alpha(d) || (d >= '0' && d <= '9') || d == '.' ||
                ((d == '+' || d == '-') && (c == 'e' || c == 'E' || c == 'p' || c == 'P'))) {
                c = d;
                pp_cursor++;
            } else
                break;
        }
    } else if (alpha(c) || (c == 92 && pp_cursor < pp_input_size &&
                            (pp_at(pp_cursor) == 'u' || pp_at(pp_cursor) == 'U'))) {
        pp_kind = 1;
        pp_cursor = pp_start;
        while (pp_cursor < pp_input_size) {
            d = pp_at(pp_cursor);
            if (d == 92 && pp_cursor + 1 < pp_input_size &&
                (pp_at(pp_cursor + 1) == 'u' || pp_at(pp_cursor + 1) == 'U')) {
                identifier_ucn(pp_text, pp_input_size, &pp_cursor, pp_cursor == pp_start);
                if (failed)
                    return error_message("invalid universal character name in identifier");
                c89_extension("universal character name in identifier");
            } else if (alpha(d) || (d >= '0' && d <= '9'))
                pp_cursor++;
            else
                break;
        }
    } else {
        n = 0;
        if (pp_match(pp_start, "%:%:"))
            n = 4;
        else if (pp_match(pp_start, "<<=") || pp_match(pp_start, ">>=") ||
                 pp_match(pp_start, "..."))
            n = 3;
        else if (pp_match(pp_start, "->") || pp_match(pp_start, "++") || pp_match(pp_start, "--") ||
                 pp_match(pp_start, "<<") || pp_match(pp_start, ">>") || pp_match(pp_start, "<=") ||
                 pp_match(pp_start, ">=") || pp_match(pp_start, "==") || pp_match(pp_start, "!=") ||
                 pp_match(pp_start, "&&") || pp_match(pp_start, "||") || pp_match(pp_start, "*=") ||
                 pp_match(pp_start, "/=") || pp_match(pp_start, "%=") || pp_match(pp_start, "+=") ||
                 pp_match(pp_start, "-=") || pp_match(pp_start, "&=") || pp_match(pp_start, "^=") ||
                 pp_match(pp_start, "|=") || pp_match(pp_start, "##") || pp_match(pp_start, "<:") ||
                 pp_match(pp_start, ":>") || pp_match(pp_start, "<%") || pp_match(pp_start, "%>") ||
                 pp_match(pp_start, "%:"))
            n = 2;
        if (n)
            pp_cursor = pp_start + n;
    }
    pp_length = pp_cursor - pp_start;
    return pp_kind;
}

int pp_scan(void) {
    long long presumed;
    int physical;
    if (pp_text != pp_input)
        return pp_scan_raw();
    if (pp_file_eof) {
        if (pp_depth != pp_file_condition[pp_file_depth])
            return error();
        pp_file_depth--;
        pp_cursor = pp_file_cursor[pp_file_depth];
        pp_input_size = pp_file_end[pp_file_depth];
        pp_file_eof = 0;
        pp_beginning = 1;
    }
    pp_scan_raw();
    if (pp_enabled && !pp_allow_va && pp_kind == 1 && pp_word("__VA_ARGS__"))
        return error();
    if (!failed && !pp_kind && pp_file_depth) {
        pp_kind = 6;
        pp_start = pp_cursor;
        pp_length = 0;
        pp_space = 0;
        pp_file_eof = 1;
    }
    if (pp_kind) {
        physical = pp_file_physical_end[pp_file_depth];
        if (pp_start < pp_input_size)
            physical = pp_input[pp_start] >> 8;
        presumed = (long long)physical + pp_file_line_delta[pp_file_depth];
        if (pp_kind != 6 && (presumed < 1 || presumed > 4294967295LL))
            return error();
        pp_current_line = (int)presumed;
    }
    return pp_kind;
}

int pp_find(int start, int length) {
    int i;
    for (i = 0; i < pp_macros; i++)
        if (pp_equal(start, length, pp_name_start[i], pp_name_length[i]))
            return i;
    return -1;
}

int pp_put(int c) {
    if (pp_output == 1048576)
        return error();
    source[pp_output++] = c;
    return 0;
}

int pp_emit(int start, int length, int kind) {
    int i, c, hex = 0, floating = 0;

    if (pp_condition && kind == 1) {
        pp_put('0');
        pp_put(' ');
        return 0;
    }
    if (kind == 2) {
        hex = length > 1 && pp_char(start, 0) == '0' &&
              (pp_char(start, 1) == 'x' || pp_char(start, 1) == 'X');
        for (i = 0; i < length; i++) {
            c = pp_char(start, i);
            if (c == '.' || (hex ? (c == 'p' || c == 'P') : (c == 'e' || c == 'E')))
                floating = 1;
        }
    }
    for (i = 0; i < length && !failed; i++) {
        c = pp_char(start, i);
        /* A sign in an integer pp-number must not split into separate tokens. */
        if (kind == 2 && !floating && (c == '+' || c == '-'))
            return error();
        pp_put(c);
    }
    pp_put(' ');
    return 0;
}

int pp_skip_line(void) {
    while (pp_kind && pp_kind != 6 && !failed)
        pp_scan();
    return 0;
}

int pp_end_line(void) {
    if (pp_kind && pp_kind != 6)
        return error();
    return 0;
}

int pp_save_token(void) {
    if (pp_tokens == 65536)
        return error();
    pp_token_start[pp_tokens] = pp_start;
    pp_token_length[pp_tokens] = pp_length;
    pp_token_kind[pp_tokens] = pp_kind;
    pp_token_space[pp_tokens] = pp_space;
    pp_tokens++;
    return 0;
}

int pp_define(void) {
    int id, first, body, n, arity = -1, i, old, p, variadic = 0;
    pp_scan();
    if (pp_kind != 1)
        return error_message("expected macro name after define");
    if (pp_builtin(pp_start, pp_length) || pp_word("defined") || pp_word("_Pragma"))
        return error_message("cannot define or undefine reserved preprocessing name");
    id = pp_find(pp_start, pp_length);
    if (id < 0) {
        if (pp_macros == 1024)
            return error_message("macro name capacity exceeded");
        id = pp_macros++;
        pp_name_start[id] = pp_start;
        pp_name_length[id] = pp_length;
    }
    first = pp_tokens;
    pp_scan();
    if (!pp_space && pp_word("(")) {
        arity = 0;
        pp_scan();
        if (!pp_word(")")) {
            while (!failed) {
                if (pp_word("...")) {
                    variadic = 1;
                    pp_save_token();
                    arity++;
                    pp_scan();
                    break;
                }
                if (pp_kind != 1)
                    return error_message("expected macro parameter name");
                if (arity == 127)
                    return error_message("macro parameter capacity exceeded");
                for (i = first; i < pp_tokens; i++)
                    if (pp_equal(pp_start, pp_length, pp_token_start[i], pp_token_length[i]))
                        return error_message("duplicate macro parameter");
                pp_save_token();
                arity++;
                pp_scan();
                if (!pp_word(","))
                    break;
                pp_scan();
            }
        }
        if (!pp_word(")"))
            return error_message("expected closing parenthesis in macro parameters");
        if (variadic)
            c89_extension("variadic macro");
        pp_allow_va = variadic;
        pp_scan();
    }
    body = pp_tokens;
    while (pp_kind && pp_kind != 6 && !failed) {

        pp_save_token();
        pp_scan();
    }
    n = pp_tokens - body;
    pp_allow_va = 0;
    for (i = body; i < pp_tokens; i++) {
        if (pp_paste_token(i) && (i == body || i + 1 == pp_tokens))
            return error_message("token paste cannot begin or end replacement list");
        if (arity >= 0 && (pp_token_word(i, "#") || pp_token_word(i, "%:"))) {
            i++;
            if (i == pp_tokens || pp_token_kind[i] != 1)
                return error_message("stringize requires macro parameter");
            old = variadic && pp_token_word(i, "__VA_ARGS__");
            for (p = 0; p < arity; p++)
                if (pp_equal(pp_token_start[i], pp_token_length[i], pp_token_start[first + p],
                             pp_token_length[first + p]))
                    old = 1;
            if (!old)
                return error_message("stringize requires macro parameter");
        }
    }
    if (pp_defined[id]) {
        if (pp_arity[id] != arity || pp_variadic[id] != variadic || pp_body_length[id] != n)
            return error_message("incompatible macro redefinition");
        old = pp_parameter_start[id];
        for (i = 0; i < arity; i++)
            if (!pp_equal(pp_token_start[first + i], pp_token_length[first + i],
                          pp_token_start[old + i], pp_token_length[old + i]))
                return error_message("incompatible macro redefinition");
        old = pp_body_start[id];
        for (i = 0; i < n; i++) {
            if (!pp_equal(pp_token_start[body + i], pp_token_length[body + i],
                          pp_token_start[old + i], pp_token_length[old + i]))
                return error_message("incompatible macro redefinition");
            if (i && pp_token_space[body + i] != pp_token_space[old + i])
                return error_message("incompatible macro redefinition");
        }
        pp_tokens = first;
    } else {
        pp_defined[id] = 1;
        pp_parameter_start[id] = first;
        pp_arity[id] = arity;
        pp_variadic[id] = variadic;
        pp_body_start[id] = body;
        pp_body_length[id] = n;
    }
    return 0;
}

int pp_test(int evaluate) {
    int start = pp_output, old_cursor = cursor, old_size = size, old_line = line,
        old_strings = string_used, id, paren, result;
    int head = 0, tail = 0, n;
    if (!evaluate) {
        pp_skip_line();
        return 0;
    }
    pp_condition = 1;
    while (pp_kind && pp_kind != 6 && !failed) {
        if (pp_kind == 1 && pp_word("defined")) {
            pp_scan();
            paren = pp_word("(");
            if (paren)
                pp_scan();
            if (pp_kind != 1)
                return error();
            id = pp_find(pp_start, pp_length);
            n = pp_node(((id >= 0 && pp_defined[id]) || pp_builtin(pp_start, pp_length)) ? -2 : -1,
                        1, 2, 1, 0);
            pp_scan();
            if (paren) {
                if (!pp_word(")"))
                    return error();
                pp_scan();
            }
        } else {
            n = pp_node(pp_start, pp_length, pp_kind, pp_space, 0);
            pp_scan();
        }
        if (tail)
            pp_node_next[tail] = n;
        else
            head = n;
        tail = n;
    }
    pp_expand_stream(head, 0, 0, 1);
    pp_hide_reset();
    cursor = start;
    size = pp_output;
    next();
    ce_expression(2, 1);
    if (token)
        error();
    result = ce_value != 0;
    cursor = old_cursor;
    size = old_size;
    line = old_line;
    string_used = old_strings;
    pp_output = start;
    pp_condition = 0;
    return result;
}

/* All file operations run in the self-built PE through its import table. */
typedef long long PP_Open(char *, unsigned int, int, void *, int, int, long long);
typedef int PP_Read(long long, char *, int, int *, void *);
typedef int PP_Close(long long);
typedef unsigned int PP_Environment(char *, char *, unsigned int);

/* Find separators at ACP character boundaries, not inside a DBCS character. */
int pp_path_directory(char *path, int length) {
    typedef int Lead(unsigned int, unsigned char);
    Lead *lead = (Lead *)__c1_win_IsDBCSLeadByteEx;
    int i, last = 0;
    for (i = 0; i < length; i++) {
        if ((unsigned char)path[i] >= 128 && i + 1 < length && lead(0, (unsigned char)path[i])) {
            i++;
            continue;
        }
        if (path[i] == '/' || path[i] == 92)
            last = i + 1;
    }
    return last;
}

long long pp_open_candidate(char *directory, int length) {
    int i = 0, j = 0;
    PP_Open *open = (PP_Open *)__c1_win_CreateFileA;
    while (i < length) {
        if (i == 1023) {
            error();
            return -1LL;
        }
        pp_candidate[i] = directory[i];
        i++;
    }
    if (i && pp_path_directory(directory, length) != length) {
        if (i == 1023) {
            error();
            return -1LL;
        }
        pp_candidate[i++] = '/';
    }
    while (pp_include_name[j]) {
        if (i == 1023) {
            error();
            return -1LL;
        }
        pp_candidate[i++] = pp_include_name[j++];
    }
    pp_candidate[i] = 0;
    return open(pp_candidate, 0x80000000U, 7, 0, 3, 0x80, 0);
}

long long pp_search_header(int quoted) {
    int i, last = 0, begin, length;
    long long handle;
    char *parent = pp_file_path + pp_file_depth * 1024;
    PP_Environment *environment = (PP_Environment *)__c1_win_GetEnvironmentVariableA;
    if (pp_include_name[0] == '/' || pp_include_name[0] == 92 || pp_include_name[1] == ':')
        return pp_open_candidate("", 0);
    if (quoted) {
        for (i = 0; parent[i]; i++)
            ;
        last = pp_path_directory(parent, i);
        if (last) {
            handle = pp_open_candidate(parent, last);
            if (handle != -1LL || failed)
                return handle;
        }
        handle = pp_open_candidate("", 0);
        if (handle != -1LL || failed)
            return handle;
    }
    length = environment("C1_INCLUDE", pp_include_env, 4096);
    if (length >= 4096) {
        error();
        return -1LL;
    }
    begin = 0;
    for (i = 0; i <= length; i++)
        if (i == length || pp_include_env[i] == ';') {
            if (i > begin) {
                handle = pp_open_candidate(pp_include_env + begin, i - begin);
                if (handle != -1LL || failed)
                    return handle;
            }
            begin = i + 1;
        }
    return pp_open_candidate("include", 7);
}

int pp_header_phases(int begin, int end) {
    int input = begin, output = begin, c, replacement, physical = 1;
    while (input < end) {
        c = pp_input[input++];
        if (c == 13) {
            if (input < end && pp_input[input] == 10)
                input++;
            c = 10;
        } else if (c == '?' && input + 1 < end && pp_input[input] == '?') {
            replacement = trigraph(pp_input[input + 1]);
            if (replacement) {
                c = replacement;
                input += 2;
            }
        }
        pp_input[output++] = c | (physical << 8);
        if (c == 10)
            physical++;
    }
    end = output;
    input = begin;
    output = begin;
    while (input < end) {
        c = pp_input[input++];
        if ((c & 255) == 92 && input < end && (pp_input[input] & 255) == 10)
            input++;
        else
            pp_input[output++] = c;
    }
    phase_last_line = physical;
    return output;
}

int pp_load_header(int quoted) {
    int begin = pp_arena_used, got, i, ok;
    long long handle;
    PP_Read *read = (PP_Read *)__c1_win_ReadFile;
    PP_Close *close = (PP_Close *)__c1_win_CloseHandle;
    if (pp_file_depth == 63)
        return error_message("include nesting limit exceeded");
    handle = pp_search_header(quoted);
    if (handle == -1LL)
        return error_message("cannot open include file");
    while (!failed) {
        got = 0;
        ok = read(handle, pp_read_buffer, 4096, &got, 0);
        if (!ok) {
            error_message("cannot read include file");
            break;
        }
        if (!got)
            break;
        if (got > 1048576 - pp_arena_used) {
            error();
            break;
        }
        for (i = 0; i < got; i++)
            pp_input[pp_arena_used++] = (unsigned char)pp_read_buffer[i];
    }
    if (!close(handle))
        error();
    if (failed)
        return 0;
    pp_arena_used = pp_header_phases(begin, pp_arena_used);
    pp_file_cursor[pp_file_depth] = pp_cursor;
    pp_file_end[pp_file_depth] = pp_input_size;
    pp_file_depth++;
    pp_file_condition[pp_file_depth] = pp_depth;
    pp_file_physical_end[pp_file_depth] = phase_last_line;
    pp_file_line_delta[pp_file_depth] = 0;
    pp_display_name(pp_candidate);
    i = 0;
    do {
        pp_file_path[pp_file_depth * 1024 + i] = pp_candidate[i];
    } while (pp_candidate[i++]);
    pp_cursor = begin;
    pp_input_size = pp_arena_used;
    pp_file_eof = 0;
    pp_beginning = 1;
    return pp_scan();
}

int pp_header_char(int *length, int c) {
    if (*length == 1023 || !c || c == 10 || c == 13)
        return error();
    pp_include_name[(*length)++] = c;
    return 0;
}

int pp_include(void) {
    int length = 0, quoted = 0, head = 0, tail = 0, n, next_node, i, start, count;
    pp_scan();
    if (pp_kind == 3 && (pp_input[pp_start] & 255) == '"') {
        quoted = 1;
        for (i = 1; i < pp_length - 1 && !failed; i++)
            pp_header_char(&length, pp_input[pp_start + i] & 255);
        pp_scan();
        pp_end_line();
    } else if (pp_kind == 5 && (pp_input[pp_start] & 255) == '<') {
        pp_cursor = pp_start + 1;
        while (pp_cursor < pp_input_size && (pp_input[pp_cursor] & 255) != '>' && !failed)
            pp_header_char(&length, pp_input[pp_cursor++] & 255);
        if (pp_cursor == pp_input_size || failed)
            return error();
        pp_cursor++;
        pp_scan();
        pp_end_line();
    } else {
        while (pp_kind && pp_kind != 6 && !failed) {
            n = pp_node(pp_start, pp_length, pp_kind, pp_space, 0);
            if (tail)
                pp_node_next[tail] = n;
            else
                head = n;
            tail = n;
            pp_scan();
        }
        head = pp_expand_stream(head, 0, 0, 0);
        n = head;
        if (!n || failed)
            return error();
        if (pp_node_kind[n] == 3 && pp_char(pp_node_start[n], 0) == '"' && !pp_node_next[n]) {
            quoted = 1;
            start = pp_node_start[n];
            count = pp_node_length[n];
            for (i = 1; i < count - 1 && !failed; i++)
                pp_header_char(&length, pp_char(start, i));
        } else if (pp_node_word(n, "<")) {
            n = pp_node_next[n];
            while (n && !pp_node_word(n, ">") && !failed) {
                if (pp_node_space[n])
                    pp_header_char(&length, ' ');
                for (i = 0; i < pp_node_length[n] && !failed; i++)
                    pp_header_char(&length, pp_char(pp_node_start[n], i));
                n = pp_node_next[n];
            }
            if (!n || pp_node_next[n])
                error();
            else if (pp_node_space[n])
                pp_header_char(&length, ' ');
        } else
            error();
        pp_release_list(head);
        pp_hide_reset();
    }
    if (failed || !length)
        return error();
    pp_include_name[length] = 0;
    pp_directive_space = 0;
    return pp_load_header(quoted);
}

int pp_line_control(void) {
    int head = 0, tail = 0, n, i, c, physical;
    long long number = 0;
    pp_scan();
    while (pp_kind && pp_kind != 6 && !failed) {
        n = pp_node(pp_start, pp_length, pp_kind, pp_space, 0);
        if (tail)
            pp_node_next[tail] = n;
        else
            head = n;
        tail = n;
        pp_scan();
    }
    head = pp_expand_stream(head, 0, 0, 0);
    n = head;
    if (!n || pp_node_kind[n] != 2 || failed)
        return error();
    for (i = 0; i < pp_node_length[n]; i++) {
        c = pp_char(pp_node_start[n], i);
        if (c < '0' || c > '9')
            return error();
        number = number * 10 + c - '0';
        if (number > 2147483647LL)
            return error();
    }
    if (!number)
        return error();
    n = pp_node_next[n];
    if (n) {
        if (pp_node_kind[n] != 3 || pp_char(pp_node_start[n], 0) != 34 || pp_node_next[n])
            return error();
        pp_file_display[pp_file_depth] = pp_node_start[n];
        pp_file_display_length[pp_file_depth] = pp_node_length[n];
    }
    physical = pp_file_physical_end[pp_file_depth];
    if (pp_kind == 6 && pp_length)
        physical = pp_input[pp_start] >> 8;
    pp_file_line_delta[pp_file_depth] = (int)number - physical - 1;
    pp_release_list(head);
    pp_hide_reset();
    return 0;
}

int pp_pragma_directive(void) {
    int head = 0, tail = 0, n;
    pp_scan();
    if (!pp_word("STDC"))
        return pp_skip_line();
    while (pp_kind && pp_kind != 6 && !failed) {
        n = pp_node(pp_start, pp_length, pp_kind, pp_space, 0);
        if (tail)
            pp_node_next[tail] = n;
        else
            head = n;
        tail = n;
        pp_scan();
    }
    if (!failed)
        pp_pragma_apply(head);
    pp_release_list(head);
    return 0;
}

int pp_diagnostic_prefix(void) {
    int i, start = pp_file_display[pp_file_depth], length = pp_file_display_length[pp_file_depth];
    diagnostic_text("C1 error: ");
    for (i = 0; i < length; i++)
        diagnostic_put(pp_char(start, i));
    diagnostic_put(':');
    diagnostic_number((unsigned int)pp_current_line);
    diagnostic_text(": ");
    return 0;
}

int pp_error_directive(void) {
    int i, any = 0;
    pp_diagnostic_prefix();
    diagnostic_text("#error");
    pp_scan();
    while (pp_kind && pp_kind != 6 && !failed) {
        if (!any || pp_space)
            diagnostic_put(' ');
        for (i = 0; i < pp_length; i++)
            diagnostic_put(pp_char(pp_start, i));
        any = 1;
        pp_scan();
    }
    diagnostic_end();
    return error();
}

int pp_directive(void) {
    int op = 0, id, selected = 0;
    pp_scan();
    if (!pp_kind || pp_kind == 6)
        return 0;
    if (pp_kind != 1) {
        if (pp_enabled)
            return error();
        return pp_skip_line();
    }
    if (pp_word("if"))
        op = 1;
    if (pp_word("ifdef"))
        op = 2;
    if (pp_word("ifndef"))
        op = 3;
    if (pp_word("elif"))
        op = 4;
    if (pp_word("else"))
        op = 5;
    if (pp_word("endif"))
        op = 6;
    if (op) {
        pp_scan();
        if (op <= 3) {
            if (pp_depth == 128)
                return error();
            if (!pp_enabled)
                pp_skip_line();
            else if (op == 1)
                selected = pp_test(1);
            else {
                if (pp_kind != 1)
                    return error();
                id = pp_find(pp_start, pp_length);
                selected = (id >= 0 && pp_defined[id]) || pp_builtin(pp_start, pp_length);
                if (op == 3)
                    selected = !selected;
                pp_scan();
                pp_end_line();
            }
            pp_parent[pp_depth] = pp_enabled;
            pp_taken[pp_depth] = selected;
            pp_else[pp_depth] = 0;
            pp_depth++;
            pp_enabled = pp_enabled && selected;
        } else {
            if (!pp_depth || (pp_file_depth && pp_depth <= pp_file_condition[pp_file_depth]))
                return error();
            id = pp_depth - 1;
            if (op == 6) {
                if (pp_parent[id])
                    pp_end_line();
                else
                    pp_skip_line();
                pp_enabled = pp_parent[id];
                pp_depth--;
            } else {
                if (pp_else[id])
                    return error();
                if (op == 5) {
                    if (pp_parent[id])
                        pp_end_line();
                    else
                        pp_skip_line();
                    pp_else[id] = 1;
                    selected = 1;
                } else
                    selected = pp_test(pp_parent[id] && !pp_taken[id]);
                pp_enabled = pp_parent[id] && !pp_taken[id] && selected;
                pp_taken[id] = pp_taken[id] || selected;
            }
        }
        return 0;
    }
    if (!pp_enabled)
        return pp_skip_line();
    if (pp_word("error"))
        return pp_error_directive();
    if (pp_word("line"))
        return pp_line_control();
    if (pp_word("include"))
        return pp_include();
    if (pp_word("define"))
        return pp_define();
    if (pp_word("undef")) {
        pp_scan();
        if (pp_kind != 1)
            return error();
        if (pp_builtin(pp_start, pp_length) || pp_word("defined") || pp_word("_Pragma"))
            return error_message("cannot define or undefine reserved preprocessing name");
        id = pp_find(pp_start, pp_length);
        if (id >= 0)
            pp_defined[id] = 0;
        pp_scan();
        return pp_end_line();
    }
    if (pp_word("pragma"))
        return pp_pragma_directive();
    return error();
}

int preprocess(void) {
    int i;
    pp_input_size = size;
    pp_arena_used = size;
    pp_enabled = 1;
    pp_beginning = 1;
    pp_text = pp_input;
    pp_file_physical_end[0] = phase_last_line;
    pp_display_name("<stdin>");
    for (i = 0; i < size; i++)
        pp_input[i] = source[i];
    pp_scan();
    while (pp_kind && !failed) {
        if (pp_kind == 6) {
            pp_beginning = 1;
            pp_scan();
        } else if (pp_beginning && (pp_word("#") || pp_word("%:"))) {
            pp_directive_space = pp_enabled;
            pp_directive();
            pp_directive_space = 0;
            pp_beginning = 1;
            if (pp_kind == 6)
                pp_scan();
        } else if (pp_enabled) {
            pp_expand_stream(0, 1, 0, 1);
            pp_hide_reset();
        } else {
            pp_beginning = 0;
            pp_scan();
        }
    }
    if (pp_depth)
        error();
    if (failed && !diagnostic_sent) {
        pp_diagnostic_prefix();
        diagnostic_text(diagnostic_reason ? diagnostic_reason : "preprocessing failed");
        diagnostic_end();
    }
    for (i = 0; i < pp_generated; i++)
        image[i] = 0;
    size = pp_output;
    cursor = 0;
    line = 1;
    token = 0;
    return 0;
}

int hosted_startup(void) {
    int i, id, match;
    char *p = hosted_startup_source, *spelling = "__c1_host_main";
    if (arity[6] != 2 || failed)
        return 0;
    size = 0;
    cursor = 0;
    line = 1;
    while (*p)
        source[size++] = *p++;
    next();
    while (token && !failed)
        function();
    for (id = 0; id < count; id++) {
        match = lengths[id] == 14;
        for (i = 0; i < 14 && match; i++)
            if (names[id * 256 + i] != spelling[i])
                match = 0;
        if (match) {
            call_symbol[hosted_startup_call] = id;
            return 0;
        }
    }
    return error();
}

int configure_standard(void) {
    PP_Environment *environment;
    char setting[8];
    unsigned int n;
    /* The frozen capacity compiler emits only the original four imports.
       Its stage0 child compiles the self-contained source in legacy mode. */
    /* This host uses symbolic imports; no frozen-IAT compatibility probe. */
    environment = (PP_Environment *)__c1_win_GetEnvironmentVariableA;
    n = environment("C1_STANDARD", setting, 8);
    if (!n)
        return 0;
    if (n == 3 && setting[0] == 'c' && setting[1] == '8' && setting[2] == '9') {
        c89_identifiers = 1;
        return 0;
    }
    if (n == 3 && setting[0] == 'c' && setting[1] == '9' && setting[2] == '9') {
        c99_mode = 1;
        return 0;
    }
    diagnostic_text("C1 error: C1_STANDARD must be c89 or c99");
    diagnostic_end();
    return error();
}

int compiler_output(int total) {
    DiagnosticHandle *get = (DiagnosticHandle *)__c1_win_GetStdHandle;
    DiagnosticWrite *write = (DiagnosticWrite *)__c1_win_WriteFile;
    long long handle = get(-11);
    char buffer[4096];
    int base = 0, count, i, offset, done;
    while (base < total) {
        count = total - base;
        if (count > 4096)
            count = 4096;
        for (i = 0; i < count; i = i + 1)
            buffer[i] = (char)image[base + i];
        offset = 0;
        while (offset < count) {
            done = 0;
            if (!write(handle, buffer + offset, count - offset, &done, 0))
                return 1;
            if (done <= 0 || done > count - offset)
                return 1;
            offset += done;
        }
        base += count;
    }
    return 0;
}

#include "src/bootstrap-c99/host/object-output.c"

int main(void) {
    int c, i, total, id;
    initialize();
    configure_standard();
    configure_object();
    if (failed)
        return 1;
    allocation = 0x600000;
    symbol_flags[6] = 2;
    arity[6] = 0;
    global_type[6] = 1;
    symbol_flags[10] = 5;
    symbol_flags[11] = 5;
    arity[11] = 1;
    if (object_mode) {
        symbol_flags[10] = 4;
        symbol_flags[11] = 4;
    }
    global_type[10] = 1;
    global_type[11] = 1;
    parameter_type[1408] = 1;
    file_function_type[6] = function_entity_signature(6);
    file_function_type[10] = function_entity_signature(10);
    file_function_type[11] = function_entity_signature(11);
    while ((c = __c0_getchar()) != -1) {
        if (size == 1048576) {
            diagnostic_text("C1 error: source input exceeds 1048576 bytes");
            diagnostic_end();
            return 1;
        }
        source[size] = c;
        size = size + 1;
    }
    source_phases();
    preprocess();
    if (object_mode)
        position = 4096;
    else
        pe_header();
    next();
    if (!failed && token == 0)
        error_message("empty translation unit");
    while (token != 0 && !failed)
        function();
    for (i = 0; i < count; i++)
        if (inline_seen[i] && !inline_external[i] && !(symbol_flags[i] & 32)) {
            if (inline_invalid[i])
                error_message(
                    "external inline definition uses internal linkage or non-const static object");
            symbol_flags[i] = symbol_flags[i] & ~1;
        }
    if (object_mode) {
        if (!failed)
            finish_objects();
        total = failed ? 0 : object_prepare();
        if (failed) {
            diagnostic_text("C1 error: ");
            diagnostic_text(diagnostic_reason ? diagnostic_reason
                                              : "invalid object translation unit");
            diagnostic_end();
            return 1;
        }
        return compiler_output(total);
    }
    if ((symbol_flags[6] & 1) == 0)
        error();
    hosted_startup();
    if (initializer_tail)
        patch(initializer_tail, position);
    if (!initializer_start)
        initializer_start = position;
    emit(0xc3);
    patch(initializer_call, initializer_start);
    runtime_function(10);
    runtime_function(11);
    for (i = 0; i < call_count && !failed; i = i + 1) {
        id = call_symbol[i];
        if ((symbol_flags[id] & 1) == 0)
            error();
        else {
            if (call_arity[i] >= 0 && !arity_matches(arity[id], call_arity[i]))
                error();
            patch(call_position[i], function_address[id]);
        }
    }
    finish_objects();
    if (failed) {
        if (!diagnostic_sent) {
            diagnostic_text("C1 error: ");
            diagnostic_text(diagnostic_reason ? diagnostic_reason
                                              : "invalid or unsupported translation unit");
            diagnostic_end();
        }
        return 1;
    }
    total = ((position + 511) / 512) * 512;
    write32(0x9c, total - 512);
    write32(0x198, total - 512);
    return compiler_output(total);
}
