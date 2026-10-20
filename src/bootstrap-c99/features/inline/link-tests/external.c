int shared(void) {
    return 42;
}

int (*from_a(void))(void);
int (*from_b(void))(void);
int call_a(void);
int call_b(void);

int main(void) {
    int a = call_a(), b = call_b();
    if (a != 13 && a != 42)
        return 1;
    if (b != 17 && b != 42)
        return 2;
    if (from_a() != shared || from_b() != shared || from_a() != from_b())
        return 3;
    if (from_a()() != 42 || from_b()() != 42)
        return 4;
    return 0;
}
