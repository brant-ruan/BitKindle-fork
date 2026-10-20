int factorial(int n) {
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}

int main(void) {
    int a = 0;
    int i = 0;
    while (i < 5) {
        a = a + i;
        i = i + 1;
    }
    if (a != 10)
        return 99;
    return factorial(5);
}
