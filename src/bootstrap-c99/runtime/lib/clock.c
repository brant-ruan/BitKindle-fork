#include "winimports.h"
#ifndef __C1_CLOCK_C
#define __C1_CLOCK_C
#include <time.h>
#include <errno.h>

static clock_t __c1_clock_combine(unsigned long long kernel, unsigned long long user) {
    if (user > 9223372036854775807ULL || kernel > 9223372036854775807ULL - user) {
        errno = ERANGE;
        return (clock_t)-1;
    }
    return (clock_t)(kernel + user);
}

clock_t clock(void) {
    typedef void *Current(void);
    typedef int Times(void *, unsigned long long *, unsigned long long *, unsigned long long *,
                      unsigned long long *);
    Current *current = (Current *)__c1_win_GetCurrentProcess;
    Times *times = (Times *)__c1_win_GetProcessTimes;
    unsigned long long creation, exited, kernel, user;
    if (!times(current(), &creation, &exited, &kernel, &user)) {
        errno = EINVAL;
        return (clock_t)-1;
    }
    return __c1_clock_combine(kernel, user);
}
#endif
