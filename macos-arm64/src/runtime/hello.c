int main(void) {
    puts("==================================================");
    puts("Hello from BitKindle Native Darwin ARM64 Toolchain!");
    puts("Running directly on Apple Silicon without external libs!");
    puts("==================================================");
    print("Testing integer formatting: 12345 + 67890 = ");
    print_int(12345 + 67890);
    putchar('\n');
    return 42;
}
