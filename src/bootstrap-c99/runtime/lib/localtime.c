#include "winimports.h"
#ifndef __C1_LOCALTIME_C
#define __C1_LOCALTIME_C
#include "time.c"

struct __c1_transition {
    unsigned short year, month, weekday, day, hour, minute, second, millisecond;
};

struct __c1_zone {
    int bias;
    unsigned short standard_name[32];
    struct __c1_transition standard;
    int standard_bias;
    unsigned short daylight_name[32];
    struct __c1_transition daylight;
    int daylight_bias;
};

static int __c1_month_days(long long y, int m) {
    static int lengths[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return lengths[m] + (m == 1 && __c1_leap_year(y));
}

static long long __c1_month_start(long long y, int m) {
    long long d = __c1_year_day(y);
    int i;
    for (i = 0; i < m; i++)
        d += __c1_month_days(y, i);
    return d;
}

static time_t __c1_wall_seconds(const struct tm *v) {
    long long y = (long long)v->tm_year + 1900, q = __c1_floor_div(v->tm_mon, 12);
    int m = (int)((long long)v->tm_mon - q * 12);
    y += q;
    return (__c1_month_start(y, m) + (long long)v->tm_mday - 1) * 86400 +
           (long long)v->tm_hour * 3600 + (long long)v->tm_min * 60 + v->tm_sec;
}

static int __c1_time_shift(time_t t, long long offset, time_t *result) {
    if ((offset > 0 && t > 9223372036854775807LL - offset) ||
        (offset < 0 && t < (-9223372036854775807LL - 1) - offset)) {
        errno = ERANGE;
        return 1;
    }
    *result = t + offset;
    return 0;
}

static time_t __c1_transition_time(long long year, const struct __c1_transition *rule) {
    long long first;
    int weekday, day;
    if (rule->year)
        year = rule->year;
    first = __c1_month_start(year, rule->month - 1);
    if (rule->year)
        day = rule->day;
    else {
        weekday = (int)((first + 4) % 7);
        if (weekday < 0)
            weekday += 7;
        day = 1 + (rule->weekday - weekday + 7) % 7 + 7 * (rule->day - 1);
        if (day > __c1_month_days(year, rule->month - 1))
            day -= 7;
    }
    return (first + day - 1) * 86400 + (long long)rule->hour * 3600 + (long long)rule->minute * 60 +
           rule->second;
}

static int __c1_rule_valid(const struct __c1_transition *r) {
    return r->month >= 1 && r->month <= 12 && r->day >= 1 && r->day <= (r->year ? 31 : 5) &&
           r->weekday <= 6 && r->hour < 24 && r->minute < 60 && r->second < 60;
}

static int __c1_zone_convert(time_t t, const struct __c1_zone *zone, struct tm *result) {
    long long standard_offset = -(long long)zone->bias * 60, daylight_offset, start, end;
    time_t adjusted;
    struct tm local;
    int dst = 0, has_dst = zone->standard.month && zone->daylight.month;
    if (has_dst)
        standard_offset -= (long long)zone->standard_bias * 60;
    if (__c1_time_shift(t, standard_offset, &adjusted) || !__c1_breakdown(&adjusted, &local))
        return 1;
    if (has_dst) {
        if (!__c1_rule_valid(&zone->standard) || !__c1_rule_valid(&zone->daylight)) {
            errno = ERANGE;
            return 1;
        }
        daylight_offset = -((long long)zone->bias + zone->daylight_bias) * 60;
        start = __c1_transition_time((long long)local.tm_year + 1900, &zone->daylight) -
                standard_offset;
        end = __c1_transition_time((long long)local.tm_year + 1900, &zone->standard) -
              daylight_offset;
        if (zone->standard.year || zone->daylight.year || start < end)
            dst = t >= start && t < end;
        else if (start > end)
            dst = t >= start || t < end;
        if (dst) {
            if (__c1_time_shift(t, daylight_offset, &adjusted) ||
                !__c1_breakdown(&adjusted, &local))
                return 1;
        }
    }
    local.tm_isdst = dst;
    *result = local;
    return 0;
}

static int __c1_read_zone(struct __c1_zone *zone) {
    typedef unsigned int ReadZone(struct __c1_zone *);
    ReadZone *read = (ReadZone *)__c1_win_GetTimeZoneInformation;
    if (read(zone) == 0xffffffffU) {
        errno = EINVAL;
        return 1;
    }
    return 0;
}

static time_t __c1_zone_mktime(struct tm *v, const struct __c1_zone *zone) {
    time_t wall, chosen, alternate;
    long long standard_bias = zone->bias, daylight_bias = zone->bias;
    struct tm normalized, result;
    int has_dst = zone->standard.month && zone->daylight.month;
    if (!v) {
        errno = EINVAL;
        return -1;
    }
    wall = __c1_wall_seconds(v);
    if (!__c1_breakdown(&wall, &normalized))
        return -1;
    if (has_dst) {
        standard_bias += zone->standard_bias;
        daylight_bias += zone->daylight_bias;
    }
    if (__c1_time_shift(wall, (v->tm_isdst > 0 ? daylight_bias : standard_bias) * 60, &chosen))
        return -1;
    if (v->tm_isdst < 0 && has_dst) {
        if (__c1_zone_convert(chosen, zone, &result))
            return -1;
        if (__c1_wall_seconds(&result) != wall) {
            if (__c1_time_shift(wall, daylight_bias * 60, &alternate) ||
                __c1_zone_convert(alternate, zone, &result))
                return -1;
            if (__c1_wall_seconds(&result) == wall)
                chosen = alternate;
        }
    }
    if (__c1_zone_convert(chosen, zone, &result))
        return -1;
    *v = result;
    return chosen;
}

#include "dynamic-zone.c"

struct tm *localtime(const time_t *t) {
    struct __c1_dynamic_zone zone;
    struct tm result;
    if (!t) {
        errno = EINVAL;
        return NULL;
    }
    if (__c1_read_dynamic(&zone) || __c1_dynamic_convert(*t, &zone, &result))
        return NULL;
    __c1_calendar = result;
    return &__c1_calendar;
}

time_t mktime(struct tm *v) {
    struct __c1_dynamic_zone zone;
    if (__c1_read_dynamic(&zone))
        return -1;
    return __c1_dynamic_mktime(v, &zone);
}

char *ctime(const time_t *t) {
    struct tm *v = localtime(t);
    return v ? asctime(v) : NULL;
}
#endif
