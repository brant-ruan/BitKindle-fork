#include <stddef.h>
#include <float.h>
static float folded_even = (float)16777217;
static float folded_odd = (float)16777219;
static float folded_mid = (float)1.000000059604644775390625;
static float folded_next = (float)1.000000178813934326171875;

int main(void) {
    volatile int even = 16777217, odd = 16777219;
    volatile double middle = 1.000000059604644775390625;
    volatile double next = 1.000000178813934326171875;
    volatile double fraction = 3.75, negative = -3.75;
    volatile size_t large = ((size_t)1 << 53) + 1;
    float a = (float)even, b = (float)odd, c = (float)middle, d = (float)next;
    double e = (double)large;
    if (FLT_ROUNDS != 1)
        return 1;
    if (a != 16777216.0f || b != 16777220.0f)
        return 2;
    if (c != 1.0f || d != 1.0000002384185791015625f)
        return 3;
    if (folded_even != a || folded_odd != b || folded_mid != c || folded_next != d)
        return 4;
    if ((float)-even != -16777216.0f || (float)-odd != -16777220.0f)
        return 5;
    if ((float)-middle != -1.0f || (float)-next != -1.0000002384185791015625f)
        return 6;
    if ((int)fraction != 3 || (int)negative != -3 || (long)negative != -3)
        return 7;
    if (e != 9007199254740992.0)
        return 8;
    large = ((size_t)1 << 53) + 3;
    if ((double)large != 9007199254740996.0)
        return 9;
    return 0;
}
