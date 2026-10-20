/* C89 3.8.3 example: compare expanded expressions with the published result. */
int f(int a) {
    return a + 3;
}

int t(int a) {
    return a + 5;
}

int m(int a, int b) {
    return a * 7 + b;
}

int probe(int y, int initial) {
    int z[1], actual1, actual2, expected1, expected2;
    z[0] = initial;
#define x 3
#define f(a) f(x * (a))
#undef x
#define x 2
#define g f
#define z z[0]
#define h g(~
#define m(a) a(w)
#define w 0,1
#define t(a) a
    actual1 = f(y + 1) + f(f(z)) % t(t(g)(0) + t)(1);
    actual2=g(x+(3,4)-w) | h 5) & m
        (f)^m(m);
#undef x
#undef f
#undef g
#undef z
#undef h
#undef m
#undef w
#undef t
    expected1 = f(2 * (y + 1)) + f(2 * (f(2 * (z[0])))) % f(2 * (0)) + t(1);
    expected2 = f(2 * (2 + (3, 4) - 0, 1)) | f(2 * (~5)) & f(2 * (0, 1)) ^ m(0, 1);
    if (actual1 != expected1)
        return 1;
    if (actual2 != expected2)
        return 2;
    return 0;
}

int main(void) {
    int y, z, status;
    for (y = -4; y <= 4; y++)
        for (z = -4; z <= 4; z++) {
            status = probe(y, z);
            if (status)
                return status;
        }
    return 0;
}
