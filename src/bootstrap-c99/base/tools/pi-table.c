/* All generated digits come from this self-built integer-only C program.
   Machin's identity: pi = 16 atan(1/5) - 4 atan(1/239).
   Fixed point uses 2048 fractional bits, with an explicit error interval. */
#define WORDS 112
int __c0_putchar(int);
static int broken;

static void clear(unsigned int *a) {
    int i;
    for (i = 0; i < WORDS; i++)
        a[i] = 0;
}

static void copy(unsigned int *a, const unsigned int *b) {
    int i;
    for (i = 0; i < WORDS; i++)
        a[i] = b[i];
}

static int compare(const unsigned int *a, const unsigned int *b) {
    int i;
    for (i = WORDS - 1; i >= 0; i--) {
        if (a[i] > b[i])
            return 1;
        if (a[i] < b[i])
            return -1;
    }
    return 0;
}

static int nonzero(const unsigned int *a) {
    int i;
    for (i = 0; i < WORDS; i++)
        if (a[i])
            return 1;
    return 0;
}

static void add(unsigned int *a, const unsigned int *b) {
    unsigned long long carry = 0, t;
    int i;
    for (i = 0; i < WORDS; i++) {
        t = (unsigned long long)a[i] + b[i] + carry;
        a[i] = (unsigned int)t;
        carry = t >> 32;
    }
    if (carry)
        broken = 1;
}

static void subtract(unsigned int *a, const unsigned int *b) {
    unsigned long long borrow = 0, t, v;
    int i;
    for (i = 0; i < WORDS; i++) {
        t = (unsigned long long)b[i] + borrow;
        v = a[i];
        a[i] = (unsigned int)(v - t);
        borrow = v < t;
    }
    if (borrow)
        broken = 1;
}

static void shift(unsigned int *a) {
    unsigned int carry = 0, next;
    int i;
    for (i = 0; i < WORDS; i++) {
        next = a[i] >> 31;
        a[i] = (a[i] << 1) | carry;
        carry = next;
    }
    if (carry)
        broken = 1;
}

static void divide_small(unsigned int *a, unsigned int divisor) {
    unsigned long long carry = 0, t;
    int i;
    for (i = WORDS - 1; i >= 0; i--) {
        t = (carry << 32) | a[i];
        a[i] = (unsigned int)(t / divisor);
        carry = t % divisor;
    }
}

static void arctangent(unsigned int *sum, unsigned int denominator) {
    unsigned int term[WORDS], part[WORDS];
    int index = 0;
    clear(sum);
    clear(term);
    term[64] = 1;
    divide_small(term, denominator);
    while (nonzero(term)) {
        if (index >= 512) {
            broken = 1;
            return;
        }
        copy(part, term);
        divide_small(part, 2 * index + 1);
        if (index & 1)
            subtract(sum, part);
        else
            add(sum, part);
        divide_small(term, denominator * denominator);
        index++;
    }
    /* Each retained summand was truncated by less than one fixed-point unit;
    the first omitted alternating-series remainder is also less than one.
    With at most 512 terms, |error| < 513 fixed-point units. */
}

static void reciprocal(unsigned int *quotient, const unsigned int *denominator) {
    unsigned int remainder[WORDS];
    int bit;
    clear(quotient);
    clear(remainder);
    /* 2^(2048+1281) / (pi*2^2048) = (2/pi)*2^1280. */
    for (bit = 3329; bit >= 0; bit--) {
        shift(remainder);
        if (bit == 3329)
            remainder[0] |= 1;
        if (compare(remainder, denominator) >= 0) {
            subtract(remainder, denominator);
            quotient[bit / 32] |= 1U << (bit % 32);
        }
    }
}

static void text(const char *s) {
    while (*s) {
        if (__c0_putchar((unsigned char)*s++) < 0)
            broken = 1;
    }
}

static void hexadecimal(unsigned int n) {
    int i;
    for (i = 7; i >= 0; i--)
        if (__c0_putchar("0123456789ABCDEF"[(n >> (4 * i)) & 15]) < 0)
            broken = 1;
}

int main(void) {
    unsigned int a[WORDS], b[WORDS], lower[WORDS], upper[WORDS], margin[WORDS], q1[WORDS],
        q2[WORDS];
    int i;
    arctangent(a, 5);
    arctangent(b, 239);
    for (i = 0; i < 4; i++)
        shift(a);
    for (i = 0; i < 2; i++)
        shift(b);
    subtract(a, b);
    /* 16*513 + 4*513 = 10260 < 16384: certified enclosure of pi. */
    clear(margin);
    margin[0] = 16384;
    copy(lower, a);
    subtract(lower, margin);
    copy(upper, a);
    add(upper, margin);
    reciprocal(q1, upper);
    reciprocal(q2, lower);
    if (broken || compare(q1, q2))
        return 1;
    for (i = 40; i < WORDS; i++)
        if (q1[i])
            return 1;
    if (!(q1[39] & 0x80000000U))
        return 1;
    text("/* floor((2/pi)*2^1280), certified by the self-built pi-table tool. */\n");
    text("#ifndef __C1_TWO_OVER_PI_H\n#define __C1_TWO_OVER_PI_H\n");
    text("static const unsigned int __c1_two_over_pi[40]={\n");
    for (i = 0; i < 40; i++) {
        text("0x");
        hexadecimal(q1[i]);
        text("U");
        text(i == 39 ? "\n" : ",\n");
    }
    text("};\n#endif\n");
    return broken ? 1 : 0;
}
