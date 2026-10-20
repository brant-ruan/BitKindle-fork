#ifndef __C1_TIME_C
#define __C1_TIME_C
#include <time.h>
#include <errno.h>
#include <stdio.h>
static struct tm __c1_calendar;
static char __c1_asctime_buffer[26];

static long long __c1_floor_div(long long n, long long d) {
    long long q = n / d;
    if (n % d < 0)
        q--;
    return q;
}

static int __c1_leap_year(long long y) {
    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

static long long __c1_year_day(long long y) {
    long long p = y - 1;
    return 365 * (y - 1970) + __c1_floor_div(p, 4) - __c1_floor_div(p, 100) +
           __c1_floor_div(p, 400) - 477;
}

time_t time(time_t *result) {
    typedef void ReadTime(unsigned long long *);
    ReadTime *read = *(ReadTime **)0x4010e0;
    unsigned long long ticks;
    time_t value;
    read(&ticks);
    value = (time_t)(ticks / 10000000) - 11644473600LL;
    if (result)
        *result = value;
    return value;
}

double difftime(time_t end, time_t begin) {
    unsigned long long magnitude;
    if (end >= begin) {
        magnitude = (unsigned long long)end - (unsigned long long)begin;
        return (double)magnitude;
    }
    magnitude = (unsigned long long)begin - (unsigned long long)end;
    return -(double)magnitude;
}

static struct tm *__c1_breakdown(const time_t *value, struct tm *destination) {
    static int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    long long days, seconds, lo = -2147481748LL, hi = 2147485548LL, mid;
    int remaining, month, width, weekday;
    struct tm result;
    if (!value) {
        errno = EINVAL;
        return NULL;
    }
    days = *value / 86400;
    seconds = *value % 86400;
    if (seconds < 0) {
        seconds += 86400;
        days--;
    }
    if (days < __c1_year_day(lo) || days >= __c1_year_day(hi)) {
        errno = ERANGE;
        return NULL;
    }
    while (lo + 1 < hi) {
        mid = lo + (hi - lo) / 2;
        if (__c1_year_day(mid) <= days)
            lo = mid;
        else
            hi = mid;
    }
    remaining = (int)(days - __c1_year_day(lo));
    result.tm_yday = remaining;
    month = 0;
    while (month < 11) {
        width = months[month] + (month == 1 && __c1_leap_year(lo));
        if (remaining < width)
            break;
        remaining -= width;
        month++;
    }
    weekday = (int)((days + 4) % 7);
    if (weekday < 0)
        weekday += 7;
    result.tm_year = (int)(lo - 1900);
    result.tm_mon = month;
    result.tm_mday = remaining + 1;
    result.tm_wday = weekday;
    result.tm_isdst = 0;
    result.tm_hour = (int)(seconds / 3600);
    result.tm_min = (int)(seconds / 60 % 60);
    result.tm_sec = (int)(seconds % 60);
    *destination = result;
    return destination;
}

struct tm *gmtime(const time_t *value) {
    return __c1_breakdown(value, &__c1_calendar);
}

char *asctime(const struct tm *value) {
    char *week = "SunMonTueWedThuFriSat", *month = "JanFebMarAprMayJunJulAugSepOctNovDec";
    long long year;
    if (!value) {
        errno = EINVAL;
        return NULL;
    }
    year = (long long)value->tm_year + 1900;
    if (value->tm_wday < 0 || value->tm_wday > 6 || value->tm_mon < 0 || value->tm_mon > 11 ||
        value->tm_mday < 1 || value->tm_mday > 31 || value->tm_hour < 0 || value->tm_hour > 23 ||
        value->tm_min < 0 || value->tm_min > 59 || value->tm_sec < 0 || value->tm_sec > 61 ||
        year < -999 || year > 9999) {
        errno = ERANGE;
        return NULL;
    }
    sprintf(__c1_asctime_buffer, "%.3s %.3s%3d %02d:%02d:%02d %d\n", week + 3 * value->tm_wday,
            month + 3 * value->tm_mon, value->tm_mday, value->tm_hour, value->tm_min, value->tm_sec,
            (int)year);
    return __c1_asctime_buffer;
}
#endif
