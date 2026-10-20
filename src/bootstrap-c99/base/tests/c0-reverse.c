/* A C-written intermediate utility, compiled exclusively by C0 and A1. */
int buffer[4096];

int main(void) {
    int n = 0;
    int c;
    while ((c = __c0_getchar()) != -1) {
        if (n == 4096)
            return 1;
        buffer[n] = c;
        n = n + 1;
    }
    while (n > 0) {
        n = n - 1;
        if (__c0_putchar(buffer[n]) == -1)
            return 2;
    }
    return 0;
}
