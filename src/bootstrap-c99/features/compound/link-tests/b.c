struct S {
    int a, b;
};
extern struct S *p;
extern int *q;

int check(void) {
    return p->a + p->b != 42 || q[0] || q[1] || q[2] || q[3] != 42;
}
