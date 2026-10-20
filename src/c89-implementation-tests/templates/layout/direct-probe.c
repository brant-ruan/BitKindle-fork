#include <stddef.h>

static void clear_zero(void *p, int n) {
    unsigned char *s = p;
    while (n--)
        *s++ = 0;
}

struct Plain {
    char a;
    short b;
    int c;
    long d;
    void *e;
    double f;
    char g;
};

struct Nested {
    char a;
    struct Plain b;
    char c;
};

union Mixed {
    char a;
    short b;
    int c;
    void *d;
    double e;
    char f[9];
};

struct Bits {
    unsigned int a : 3;
    int b : 3;
    unsigned int c : 26;
    unsigned int d : 1;
    char tail;
};

struct Crossing {
    unsigned int a : 31;
    unsigned int b : 2;
};

struct Zero {
    unsigned int a : 1;
    unsigned int : 0;
    unsigned int b : 1;
};

struct Separate {
    unsigned int a : 1;
    char middle;
    unsigned int b : 1;
};

enum Range { low = -2147483647 - 1, high = 2147483647 };

static int bytes(const void *p, int n, const unsigned char *expected) {
    int i;
    const unsigned char *s = p;
    for (i = 0; i < n; i++)
        if (s[i] != expected[i])
            return 0;
    return 1;
}

int main(void) {
    struct Bits b;
    struct Crossing c;
    struct Zero z;
    struct Separate s;
    union Mixed u;
    enum Range e;
    unsigned char bb[] = {0x3d, 0, 0, 0, 1, 0, 0, 0, 42, 0, 0, 0};
    unsigned char cb[] = {0xff, 0xff, 0xff, 0x7f, 3, 0, 0, 0};
    unsigned char zb[] = {1, 0, 0, 0, 1, 0, 0, 0};
    unsigned char sb[] = {1, 0, 0, 0, 42, 0, 0, 0, 1, 0, 0, 0};
    if (sizeof(struct Plain) != 40 || offsetof(struct Plain, b) != 2 ||
        offsetof(struct Plain, c) != 4 || offsetof(struct Plain, d) != 8)
        return 1;
    if (offsetof(struct Plain, e) != 16 || offsetof(struct Plain, f) != 24 ||
        offsetof(struct Plain, g) != 32)
        return 2;
    if (sizeof(struct Nested) != 56 || offsetof(struct Nested, b) != 8 ||
        offsetof(struct Nested, c) != 48)
        return 3;
    if (sizeof(union Mixed) != 16 || offsetof(union Mixed, f) || offsetof(union Mixed, e))
        return 4;
    if (sizeof(b) != 12 || offsetof(struct Bits, tail) != 8 || sizeof(c) != 8 || sizeof(z) != 8 ||
        sizeof(s) != 12 || offsetof(struct Separate, middle) != 4)
        return 5;
    clear_zero(&b, sizeof(b));
    b.a = 5;
    b.b = -1;
    b.c = 0;
    b.d = 1;
    b.tail = 42;
    if (b.a != 5 || b.b != -1 || b.c || b.d != 1 || !bytes(&b, 12, bb))
        return 6;
    clear_zero(&c, sizeof(c));
    c.a = 2147483647U;
    c.b = 3;
    if (c.a != 2147483647U || c.b != 3 || !bytes(&c, 8, cb))
        return 7;
    clear_zero(&z, sizeof(z));
    z.a = 1;
    z.b = 1;
    if (!bytes(&z, 8, zb))
        return 8;
    clear_zero(&s, sizeof(s));
    s.a = 1;
    s.middle = 42;
    s.b = 1;
    if (!bytes(&s, 12, sb))
        return 9;
    clear_zero(&u, sizeof(u));
    u.c = 0x41424344;
    if (u.a != 0x44 || u.b != 0x4344)
        return 10;
    e = low;
    if (sizeof(e) != 4 || e != -2147483647 - 1)
        return 11;
    e = high;
    if (e != 2147483647)
        return 12;
    return 0;
}
