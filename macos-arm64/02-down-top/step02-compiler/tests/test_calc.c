int add_numbers(int a, int b) {
    return a + b;
}

int compute_sum(int n) {
    int sum = 0;
    int i = 1;
    while (i <= n) {
        sum = add_numbers(sum, i);
        i = i + 1;
    }
    return sum;
}

int main() {
    int total = compute_sum(10); /* 55 */
    if (total > 50) {
        return total - 13; /* 55 - 13 = 42 */
    } else {
        return 0;
    }
}
