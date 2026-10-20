#ifndef __C1_TIME_H
#define __C1_TIME_H
#include <stddef.h>
typedef long long time_t;
typedef long long clock_t;
#define CLOCKS_PER_SEC 10000000
clock_t clock(void);

struct tm {
    int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst;
};

time_t time(time_t *);
double difftime(time_t, time_t);
struct tm *gmtime(const time_t *);
char *asctime(const struct tm *);
struct tm *localtime(const time_t *);
time_t mktime(struct tm *);
char *ctime(const time_t *);
size_t strftime(char *, size_t, const char *, const struct tm *);
#endif
