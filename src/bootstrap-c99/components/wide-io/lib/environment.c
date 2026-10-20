#include "winimports.h"
#ifndef __C1_ENVIRONMENT_C
#define __C1_ENVIRONMENT_C
#include <stdlib.h>
static char *__c1_environment_value;

char *getenv(const char *name) {
    typedef unsigned int ReadEnvironment(const char *, char *, unsigned int);
    ReadEnvironment *read = (ReadEnvironment *)__c1_win_GetEnvironmentVariableA;
    unsigned int needed, got;
    char *value, *next;
    if (!name || !*name)
        return NULL;
    needed = read(name, NULL, 0);
    if (!needed)
        return NULL;
    value = malloc((size_t)needed);
    if (!value)
        return NULL;
    for (;;) {
        value[0] = 0;
        got = read(name, value, needed);
        if (got && got < needed)
            break;
        if (!got) {
            got = read(name, NULL, 0);
            if (!got) {
                free(value);
                return NULL;
            }
            if (got == 1) {
                value[0] = 0;
                break;
            }
        }
        next = realloc(value, (size_t)got);
        if (!next) {
            free(value);
            return NULL;
        }
        value = next;
        needed = got;
    }
    /* Keep the previous value alive through lookup: name may point into it. */
    free(__c1_environment_value);
    __c1_environment_value = value;
    return value;
}
#endif
