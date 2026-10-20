/* Independently authored Windows x64 machine-code ABI probes.
   All bytes below are C source strings emitted by the custom compiler.
   No external assembler, compiler or binary patcher generates them. */
#include "src/bootstrap-c99/base/include/stdarg.h"

struct Result {
    unsigned long long marker, status;
};

double mixed(int a, double b, float c, double d, float e, double f) {
    if (a != 11 || b != 2.5 || c != 3.75f || d != 4.5 || e != 5.25f || f != 6.5)
        return 0.0;
    return 9.25;
}

double wrong_mixed(int a, double b, float c, double d, float e, double f) {
    return 8.25;
}

float returned_float(void) {
    return 3.75f;
}

float wrong_float(void) {
    return 2.5f;
}

double variadic(double first, ...) {
    va_list ap;
    double result = first;
    int i;
    va_start(ap, first);
    for (i = 0; i < 5; i++)
        result += va_arg(ap, double);
    va_end(ap);
    return result;
}

struct Result hidden(double a, float b, int c, double d) {
    struct Result r;
    r.marker = 0x1122334455667788ULL;
    r.status = a != 2.5 || b != 3.75f || c != 11 || d != 4.5;
    return r;
}

int main(void) {
    int (*check_mixed)(int, double, float, double, float, double);
    int (*call_mixed)(void *);
    float (*raw_float)(void);
    double (*raw_double)(void);
    int (*raw_integer)(void);
    int (*call_float)(void *);
    int (*unprototyped)();
    int (*varargs)(double, ...);
    int (*call_variadic)(void *);
    struct Result (*raw_hidden)(double, float, int, double);
    int (*call_hidden)(void *, void *);
    struct Result r;

    /* Check ECX, XMM1/2/3, then [rsp+40]/[rsp+48].
       R10 accumulates XOR differences; return whether it is nonzero. */
    check_mixed = (int (*)(int, double, float, double, float,
                           double)) "\x41\x89\xca\x41\x83\xf2\x0b"
                                    "\x66\x48\x0f\x7e\xc8\x49\xbb\x00\x00\x00\x00\x00\x00\x04\x40"
                                    "\x4c\x31\xd8\x49\x09\xc2"
                                    "\x66\x0f\x7e\xd0\x35\x00\x00\x70\x40\x49\x09\xc2"
                                    "\x66\x48\x0f\x7e\xd8\x49\xbb\x00\x00\x00\x00\x00\x00\x12\x40"
                                    "\x4c\x31\xd8\x49\x09\xc2"
                                    "\x8b\x44\x24\x28\x35\x00\x00\xa8\x40\x49\x09\xc2"
                                    "\x48\x8b\x44\x24\x30\x49\xbb\x00\x00\x00\x00\x00\x00\x1a\x40"
                                    "\x4c\x31\xd8\x49\x09\xc2"
                                    "\x4d\x85\xd2\x0f\x95\xc0\x0f\xb6\xc0\xc3";
    if (check_mixed(11, 2.5, 3.75f, 4.5, 5.25f, 6.5))
        return 1;
    if (!check_mixed(12, 2.5, 3.75f, 4.5, 5.25f, 6.5))
        return 2;
    if (!check_mixed(11, 2.0, 3.75f, 4.5, 5.25f, 6.5))
        return 3;
    if (!check_mixed(11, 2.5, 3.5f, 4.5, 5.25f, 6.5))
        return 4;
    if (!check_mixed(11, 2.5, 3.75f, 4.0, 5.25f, 6.5))
        return 5;
    if (!check_mixed(11, 2.5, 3.75f, 4.5, 5.0f, 6.5))
        return 6;
    if (!check_mixed(11, 2.5, 3.75f, 4.5, 5.25f, 6.0))
        return 7;

    /* XMM0 carries 3.75; EAX deliberately contains a different sentinel. */
    raw_float = (float (*)(void)) "\xb8\x00\x00\x70\x40\x66\x0f\x6e\xc0\xb8\xef\xbe\xad\xde\xc3";
    raw_double = (double (*)(void)) "\x48\xb8\x00\x00\x00\x00\x00\x00\x0e\x40"
                                    "\x66\x48\x0f\x6e\xc0\xb8\xef\xbe\xad\xde\xc3";
    if (raw_float() != 3.75f || raw_double() != 3.75)
        return 8;
    raw_integer = (int (*)(void))raw_float;
    if ((unsigned)raw_integer() != 0xdeadbeefU)
        return 9;

    /* RCX is the C function address. Allocate shadow+two stack slots.
       Populate only XMM for FP register args; poison their GPR counterparts.
       Require the returned XMM0 bits to equal 9.25. */
    call_mixed = (int (*)(void *)) "\x49\x89\xcb\x48\x83\xec\x38"
                                   "\xc7\x44\x24\x20\x00\x00\xa8\x40"
                                   "\x48\xb8\x00\x00\x00\x00\x00\x00\x1a\x40\x48\x89\x44\x24\x28"
                                   "\xb9\x0b\x00\x00\x00"
                                   "\x48\xb8\x00\x00\x00\x00\x00\x00\x04\x40\x66\x48\x0f\x6e\xc8"
                                   "\xb8\x00\x00\x70\x40\x66\x0f\x6e\xd0"
                                   "\x48\xb8\x00\x00\x00\x00\x00\x00\x12\x40\x66\x48\x0f\x6e\xd8"
                                   "\x31\xd2\x45\x31\xc0\x45\x31\xc9\x41\xff\xd3"
                                   "\x66\x48\x0f\x7e\xc0\x48\xba\x00\x00\x00\x00\x00\x80\x22\x40"
                                   "\x48\x31\xd0\x48\x85\xc0\x0f\x95\xc0\x0f\xb6\xc0"
                                   "\x48\x83\xc4\x38\xc3";
    if (call_mixed((void *)mixed))
        return 10;
    if (!call_mixed((void *)wrong_mixed))
        return 11;
    call_float = (int (*)(void *)) "\x49\x89\xcb\x48\x83\xec\x28\x41\xff\xd3"
                                   "\x66\x0f\x7e\xc0\x35\x00\x00\x70\x40"
                                   "\x85\xc0\x0f\x95\xc0\x0f\xb6\xc0\x48\x83\xc4\x28\xc3";
    if (call_float((void *)returned_float))
        return 12;
    if (!call_float((void *)wrong_float))
        return 13;

    /* Require GPR/XMM duplication at all four positions and exact binary64
       values 1.25,2.5,3.75,4.5. This also detects omitted float promotion. */
    unprototyped = (int (*)()) "\x66\x48\x0f\x7e\xc0\x48\x31\xc8\x49\x89\xc2"
                               "\x66\x48\x0f\x7e\xc8\x48\x31\xd0\x49\x09\xc2"
                               "\x66\x48\x0f\x7e\xd0\x4c\x31\xc0\x49\x09\xc2"
                               "\x66\x48\x0f\x7e\xd8\x4c\x31\xc8\x49\x09\xc2"
                               "\x48\xb8\x00\x00\x00\x00\x00\x00\xf4\x3f\x48\x31\xc8\x49\x09\xc2"
                               "\x48\xb8\x00\x00\x00\x00\x00\x00\x04\x40\x48\x31\xd0\x49\x09\xc2"
                               "\x48\xb8\x00\x00\x00\x00\x00\x00\x0e\x40\x4c\x31\xc0\x49\x09\xc2"
                               "\x48\xb8\x00\x00\x00\x00\x00\x00\x12\x40\x4c\x31\xc8\x49\x09\xc2"
                               "\x4d\x85\xd2\x0f\x95\xc0\x0f\xb6\xc0\xc3";
    if (unprototyped(1.25f, 2.5f, 3.75f, 4.5f))
        return 14;
    if (!unprototyped(1.0f, 2.5f, 3.75f, 4.5f))
        return 15;
    varargs = (int (*)(double, ...))unprototyped;
    if (varargs(1.25, 2.5f, 3.75f, 4.5f))
        return 16;
    if (!varargs(1.25, 2.5f, 3.75f, 4.0f))
        return 17;

    /* Call a real C variadic function with 1..6 in duplicated registers
       and stack slots, then require binary64 21.0 in XMM0. */
    call_variadic = (int (*)(void *)) "\x49\x89\xcb\x48\x83\xec\x38"
                                      "\x48\xb8\x00\x00\x00\x00\x00\x00\x14\x40\x48\x89\x44\x24\x20"
                                      "\x48\xb8\x00\x00\x00\x00\x00\x00\x18\x40\x48\x89\x44\x24\x28"
                                      "\x48\xb9\x00\x00\x00\x00\x00\x00\xf0\x3f"
                                      "\x48\xba\x00\x00\x00\x00\x00\x00\x00\x40"
                                      "\x49\xb8\x00\x00\x00\x00\x00\x00\x08\x40"
                                      "\x49\xb9\x00\x00\x00\x00\x00\x00\x10\x40"
                                      "\x66\x48\x0f\x6e\xc1\x66\x48\x0f\x6e\xca"
                                      "\x66\x49\x0f\x6e\xd0\x66\x49\x0f\x6e\xd9"
                                      "\x41\xff\xd3\x66\x48\x0f\x7e\xc0"
                                      "\x48\xba\x00\x00\x00\x00\x00\x00\x35\x40"
                                      "\x48\x31\xd0\x48\x85\xc0\x0f\x95\xc0\x0f\xb6\xc0"
                                      "\x48\x83\xc4\x38\xc3";
    if (call_variadic((void *)variadic))
        return 18;

    /* Hidden return pointer in RCX shifts FP args to XMM1/XMM2 and the
       fourth explicit argument to [rsp+40]. Return the same pointer in RAX. */
    raw_hidden = (struct Result (*)(
        double, float, int, double)) "\x45\x89\xca\x41\x83\xf2\x0b"
                                     "\x66\x48\x0f\x7e\xc8\x48\xba\x00\x00\x00\x00\x00\x00\x04\x40"
                                     "\x48\x31\xd0\x49\x09\xc2"
                                     "\x66\x0f\x7e\xd0\x35\x00\x00\x70\x40\x49\x09\xc2"
                                     "\x48\x8b\x44\x24\x28\x48\xba\x00\x00\x00\x00\x00\x00\x12\x40"
                                     "\x48\x31\xd0\x49\x09\xc2"
                                     "\x48\xb8\x88\x77\x66\x55\x44\x33\x22\x11\x48\x89\x01"
                                     "\x4c\x89\x51\x08\x48\x89\xc8\xc3";
    r = raw_hidden(2.5, 3.75f, 11, 4.5);
    if (r.marker != 0x1122334455667788ULL || r.status)
        return 19;
    r = raw_hidden(2.5, 3.75f, 12, 4.5);
    if (!r.status)
        return 20;
    call_hidden =
        (int (*)(void *, void *)) "\x53\x48\x89\xd3\x49\x89\xcb\x48\x83\xec\x30"
                                  "\x48\xb8\x00\x00\x00\x00\x00\x00\x12\x40\x48\x89\x44\x24\x20"
                                  "\x48\x89\xd9"
                                  "\x48\xb8\x00\x00\x00\x00\x00\x00\x04\x40\x66\x48\x0f\x6e\xc8"
                                  "\xb8\x00\x00\x70\x40\x66\x0f\x6e\xd0"
                                  "\x41\xb9\x0b\x00\x00\x00\x31\xd2\x45\x31\xc0"
                                  "\x41\xff\xd3\x48\x39\xd8\x0f\x95\xc0\x0f\xb6\xc0"
                                  "\x48\x83\xc4\x30\x5b\xc3";
    r.marker = 0;
    r.status = 1;
    if (call_hidden((void *)hidden, &r))
        return 21;
    if (r.marker != 0x1122334455667788ULL || r.status)
        return 22;
    return 0;
}
