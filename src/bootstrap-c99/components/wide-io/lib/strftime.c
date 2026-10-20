#ifndef __C1_STRFTIME_C
#define __C1_STRFTIME_C
#include "localtime.c"
#include <string.h>

struct __c1_time_output {
    char *buffer;
    size_t capacity, used;
    int failed, zone_loaded;
    struct __c1_zone zone;
};

static void __c1_time_text(struct __c1_time_output *out, const char *s) {
    while (*s && !out->failed) {
        if (out->used + 1 >= out->capacity) {
            out->failed = 1;
            return;
        }
        out->buffer[out->used++] = *s++;
    }
}

static void __c1_time_number(struct __c1_time_output *out, long long n, int width, int space) {
    char b[40];
    snprintf(b, sizeof(b), space ? "%*lld" : "%0*lld", width, n);
    __c1_time_text(out, b);
}

static int __c1_iso_weeks(long long year) {
    int weekday = (int)((__c1_year_day(year) + 3) % 7);
    if (weekday < 0)
        weekday += 7;
    return weekday == 3 || (weekday == 2 && __c1_leap_year(year)) ? 53 : 52;
}

static long long __c1_iso_year(const struct tm *v, int *week) {
    long long year = (long long)v->tm_year + 1900;
    int w = (v->tm_yday + 11 - (v->tm_wday ? v->tm_wday : 7)) / 7;
    if (w < 1) {
        year--;
        w = __c1_iso_weeks(year);
    } else if (w > __c1_iso_weeks(year)) {
        year++;
        w = 1;
    }
    *week = w;
    return year;
}

static void __c1_time_zone_text(struct __c1_time_output *out, const struct tm *v, int name) {
    long long minutes;
    unsigned short *wide;
    int i, ascii = 1;
    if (v->tm_isdst < 0)
        return;
    if (!out->zone_loaded) {
        out->zone_loaded = __c1_read_zone_year((long long)v->tm_year + 1900, &out->zone) ? -1 : 1;
    }
    if (out->zone_loaded < 0)
        return;
    minutes = out->zone.bias;
    if (out->zone.standard.month && out->zone.daylight.month)
        minutes += v->tm_isdst ? out->zone.daylight_bias : out->zone.standard_bias;
    minutes = -minutes;
    if (name) {
        wide = v->tm_isdst ? out->zone.daylight_name : out->zone.standard_name;
        for (i = 0; i < 32 && wide[i]; i++)
            if (wide[i] > 127)
                ascii = 0;
        if (ascii && i > 0) {
            for (i = 0; i < 32 && wide[i]; i++) {
                char c[2];
                c[0] = (char)wide[i];
                c[1] = 0;
                __c1_time_text(out, c);
            }
            return;
        }
        __c1_time_text(out, "UTC");
        if (!minutes)
            return;
    }
    __c1_time_text(out, minutes < 0 ? "-" : "+");
    if (minutes < 0)
        minutes = -minutes;
    __c1_time_number(out, minutes / 60, 2, 0);
    __c1_time_number(out, minutes % 60, 2, 0);
}

static void __c1_time_format(struct __c1_time_output *out, const char *format, const struct tm *v,
                             int depth) {
    static char *weekdays[7] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                "Thursday", "Friday", "Saturday"};
    static char *months[12] = {"January",   "February", "March",    "April",
                               "May",       "June",     "July",     "August",
                               "September", "October",  "November", "December"};
    char c, modifier, b[4];
    const char *composite;
    long long year, iso;
    int week;
    if (depth > 8) {
        out->failed = 1;
        return;
    }
    year = (long long)v->tm_year + 1900;
    while (*format && !out->failed) {
        c = *format++;
        if (c != '%') {
            b[0] = c;
            b[1] = 0;
            __c1_time_text(out, b);
            continue;
        }
        modifier = 0;
        if (*format == 'E' || *format == 'O')
            modifier = *format++;
        c = *format++;
        if (!c) {
            out->failed = 1;
            return;
        }
        if ((modifier == 'E' && !strchr("cCxXyY", c)) ||
            (modifier == 'O' && !strchr("deHImMSuUVwWy", c))) {
            out->failed = 1;
            return;
        }
        composite = NULL;
        switch (c) {
        case 'a':
        case 'A':
            if (v->tm_wday < 0 || v->tm_wday > 6) {
                out->failed = 1;
                break;
            }
            if (c == 'A')
                __c1_time_text(out, weekdays[v->tm_wday]);
            else {
                memcpy(b, weekdays[v->tm_wday], 3);
                b[3] = 0;
                __c1_time_text(out, b);
            }
            break;
        case 'b':
        case 'B':
        case 'h':
            if (v->tm_mon < 0 || v->tm_mon > 11) {
                out->failed = 1;
                break;
            }
            if (c == 'B')
                __c1_time_text(out, months[v->tm_mon]);
            else {
                memcpy(b, months[v->tm_mon], 3);
                b[3] = 0;
                __c1_time_text(out, b);
            }
            break;
        case 'c':
            composite = "%a %b %e %H:%M:%S %Y";
            break;
        case 'C':
            __c1_time_number(out, __c1_floor_div(year, 100), 2, 0);
            break;
        case 'd':
            __c1_time_number(out, v->tm_mday, 2, 0);
            break;
        case 'D':
        case 'x':
            composite = "%m/%d/%y";
            break;
        case 'e':
            __c1_time_number(out, v->tm_mday, 2, 1);
            break;
        case 'F':
            composite = "%Y-%m-%d";
            break;
        case 'g':
        case 'G':
        case 'V':
            if (v->tm_wday < 0 || v->tm_wday > 6 || v->tm_yday < 0 || v->tm_yday > 365) {
                out->failed = 1;
                break;
            }
            iso = __c1_iso_year(v, &week);
            if (c == 'V')
                __c1_time_number(out, week, 2, 0);
            else if (c == 'G')
                __c1_time_number(out, iso, 4, 0);
            else
                __c1_time_number(out, (iso % 100 + 100) % 100, 2, 0);
            break;
        case 'H':
            __c1_time_number(out, v->tm_hour, 2, 0);
            break;
        case 'I':
            __c1_time_number(out, v->tm_hour % 12 ? v->tm_hour % 12 : 12, 2, 0);
            break;
        case 'j':
            __c1_time_number(out, (long long)v->tm_yday + 1, 3, 0);
            break;
        case 'm':
            __c1_time_number(out, (long long)v->tm_mon + 1, 2, 0);
            break;
        case 'M':
            __c1_time_number(out, v->tm_min, 2, 0);
            break;
        case 'n':
            __c1_time_text(out, "\n");
            break;
        case 'p':
            __c1_time_text(out, v->tm_hour < 12 ? "AM" : "PM");
            break;
        case 'r':
            composite = "%I:%M:%S %p";
            break;
        case 'R':
            composite = "%H:%M";
            break;
        case 'S':
            __c1_time_number(out, v->tm_sec, 2, 0);
            break;
        case 't':
            __c1_time_text(out, "\t");
            break;
        case 'T':
        case 'X':
            composite = "%H:%M:%S";
            break;
        case 'u':
            __c1_time_number(out, v->tm_wday ? v->tm_wday : 7, 1, 0);
            break;
        case 'U':
            __c1_time_number(out, ((long long)v->tm_yday + 7 - v->tm_wday) / 7, 2, 0);
            break;
        case 'w':
            __c1_time_number(out, v->tm_wday, 1, 0);
            break;
        case 'W':
            __c1_time_number(out, ((long long)v->tm_yday + 7 - ((long long)v->tm_wday + 6) % 7) / 7,
                             2, 0);
            break;
        case 'y':
            __c1_time_number(out, (year % 100 + 100) % 100, 2, 0);
            break;
        case 'Y':
            __c1_time_number(out, year, 4, 0);
            break;
        case 'z':
        case 'Z':
            __c1_time_zone_text(out, v, c == 'Z');
            break;
        case '%':
            __c1_time_text(out, "%");
            break;
        default:
            out->failed = 1;
            break;
        }
        if (composite)
            __c1_time_format(out, composite, v, depth + 1);
    }
}

size_t strftime(char *buffer, size_t capacity, const char *format, const struct tm *v) {
    struct __c1_time_output out;
    if (!capacity)
        return 0;
    if (!buffer || !format || !v) {
        errno = EINVAL;
        return 0;
    }
    out.buffer = buffer;
    out.capacity = capacity;
    out.used = 0;
    out.failed = 0;
    out.zone_loaded = 0;
    __c1_time_format(&out, format, v, 0);
    if (out.failed) {
        buffer[0] = 0;
        return 0;
    }
    buffer[out.used] = 0;
    return out.used;
}
#endif
