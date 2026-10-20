int main(int argc, char **argv) {
    int x = 7;
    if (argc != 2 || argv[1][0] != 120)
        return 9;
    return x != 7;
}
