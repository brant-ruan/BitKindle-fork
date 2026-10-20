#include "src/bootstrap-c99/components/wide-io/wchar.h"
/* Compile with C1_OUTPUT=object. Link as a separate runtime translation unit. */
#include "src/bootstrap-c99/components/wide-io/lib/runtime.c"
#include "src/bootstrap-c99/components/wide-io/lib/arguments.c"

/* Bootstrap byte I/O bypasses FILE buffering, text translation and exit hooks. */
int __c0_getchar(void) {
    typedef void *Standard(int);
    typedef int Read(void *, void *, unsigned int, unsigned int *, void *);
    Standard *standard = (Standard *)__c1_win_GetStdHandle;
    Read *read = (Read *)__c1_win_ReadFile;
    unsigned char byte;
    unsigned int count;
    if (!read(standard(-10), &byte, 1, &count, 0) || count != 1)
        return -1;
    return byte;
}

int __c0_putchar(int c) {
    typedef void *Standard(int);
    typedef int Write(void *, const void *, unsigned int, unsigned int *, void *);
    Standard *standard = (Standard *)__c1_win_GetStdHandle;
    Write *write = (Write *)__c1_win_WriteFile;
    unsigned char byte = (unsigned char)c;
    unsigned int count;
    if (!write(standard(-11), &byte, 1, &count, 0) || count != 1)
        return -1;
    return byte;
}
#ifdef C1_MAIN_ARGS
int main(int argc, char **argv);
#else
int main(void);
#endif
int __c1_start(void) {
    int result;
#ifdef C1_MAIN_ARGS
    int argc;
    char **argv;
    if (__c1_get_arguments(&argc, &argv))
        return 127;
    result = main(argc, argv);
#else
    result = main();
#endif
    exit(result);
    return result;
}

#include "src/bootstrap-c99/components/inttypes/inttypes.c"
#include "src/bootstrap-c99/components/wctype/wctype.c"
#include "src/bootstrap-c99/components/wchar-string/wide.c"
#include "src/bootstrap-c99/components/restartable/convert.c"
#include "src/bootstrap-c99/components/wide-integer/convert.c"
#include "src/bootstrap-c99/components/wide-float/convert.c"
#include "src/bootstrap-c99/components/wide-time/convert.c"
#include "src/bootstrap-c99/components/wide-io/convert.c"
