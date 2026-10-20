#include <stdlib.h>

static unsigned int __c1_random_state = 1U;

void srand(unsigned int seed) {
    __c1_random_state = seed;
}

int rand(void) {
    __c1_random_state = __c1_random_state * 1103515245U + 12345U;
    return (int)((__c1_random_state >> 16) & 32767U);
}
