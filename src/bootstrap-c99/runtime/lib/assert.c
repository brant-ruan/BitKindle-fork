#ifndef __C1_ASSERT_C
#define __C1_ASSERT_C
#include <stdio.h>
#include <stdlib.h>

void __c1_assert_fail(const char *expression, const char *file, int line, const char *function) {
    fprintf(stderr, "Assertion failed: %s, file %s, line %d, function %s\n", expression, file, line,
            function);
    fflush(stderr);
    abort();
}
#endif
