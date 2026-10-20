#include "src/bootstrap-c99/complex/cross-api.h"

int main(void) {
    D a = make_value(2, 3), b;
    F f;
    L l;
    struct Box box;
    D (*call)(D) = double_echo;
    if (global_value != 3.5 || global_array[2] != 3)
        return 1;
    b = call(a);
    if ((double)b != 2 || imaginary(b) != 3)
        return 2;
    f = float_echo((F)a);
    l = long_echo((L)a);
    if ((float)f != 2 || imaginary(f) != 3 || (long double)l != 2 || imaginary(l) != 3)
        return 3;
    b = many(1, a, 4, f, l, a);
    if ((double)b != 13 || imaginary(b) != 12)
        return 4;
    b = varied(3, f, a, l);
    if ((double)b != 6 || imaginary(b) != 9)
        return 5;
    box.tag = 7;
    box.value = a;
    box = box_echo(box);
    if (box.tag != 8 || (double)box.value != 4 || imaginary(box.value) != 3)
        return 6;
    change(&global_value);
    if ((double)global_value != 4.5 || imaginary(global_value) != 2)
        return 7;
    return 0;
}
