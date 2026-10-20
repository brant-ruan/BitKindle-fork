/* Protocol test driver; compiled only by a previously built custom compiler.
   One unsigned C floating token per line. Output:
   status,type,16 hex digits,inexact,underflow. */
#include "../c1/fp-convert.c"
int fp_test_input[1048578];

int main(void) {
    int c, n = 0, status, i;
    char *hex = "0123456789ABCDEF";
    while (1) {
        c = __c0_getchar();
        if (c == -1 && !n)
            return 0;
        if (c == 10 || c == -1) {
            status = fp_parse(fp_test_input, n);
            __c0_putchar('0' + status);
            __c0_putchar(',');
            __c0_putchar('1');
            __c0_putchar('0' + fp_type - 10);
            __c0_putchar(',');
            for (i = 60; i >= 0; i -= 4)
                __c0_putchar(hex[(int)((fp_bits >> i) & 15)]);
            __c0_putchar(',');
            __c0_putchar('0' + fp_inexact);
            __c0_putchar(',');
            __c0_putchar('0' + fp_underflow);
            __c0_putchar(10);
            n = 0;
            if (c == -1)
                return 0;
        } else {
            if (n == 1048578)
                return 4;
            fp_test_input[n++] = c;
        }
    }
}
