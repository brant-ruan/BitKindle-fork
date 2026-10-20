#include <string.h>

int main(int argc, char **argv) {
    return argc != 3 || strcmp(argv[1], "two words") || strcmp(argv[2], "42") || argv[3] != 0;
}