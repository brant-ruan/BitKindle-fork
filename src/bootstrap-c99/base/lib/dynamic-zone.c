struct __c1_dynamic_zone {
    struct __c1_zone base;
    unsigned short key[128];
    unsigned char disabled;
};

static int __c1_read_dynamic(struct __c1_dynamic_zone *zone) {
    typedef unsigned int Read(struct __c1_dynamic_zone *);
    Read *read = *(Read **)0x401130;
    if (read(zone) == 0xffffffffU) {
        errno = EINVAL;
        return 1;
    }
    return 0;
}

static int __c1_zone_year(long long year, struct __c1_dynamic_zone *dynamic,
                          struct __c1_zone *zone) {
    typedef int ForYear(unsigned short, struct __c1_dynamic_zone *, struct __c1_zone *);
    ForYear *query = *(ForYear **)0x401138;
    if (dynamic->disabled || !dynamic->key[0]) {
        *zone = dynamic->base;
        return 0;
    }
    if (year < 1 || year > 65535) {
        errno = ERANGE;
        return 1;
    }
    if (!query((unsigned short)year, dynamic, zone)) {
        errno = ERANGE;
        return 1;
    }
    return 0;
}

static int __c1_read_zone_year(long long year, struct __c1_zone *zone) {
    struct __c1_dynamic_zone dynamic;
    if (__c1_read_dynamic(&dynamic))
        return 1;
    return __c1_zone_year(year, &dynamic, zone);
}

static int __c1_dynamic_convert(time_t t, struct __c1_dynamic_zone *dynamic, struct tm *result) {
    struct __c1_zone zone;
    struct tm local;
    long long year;
    int i;
    if (__c1_zone_convert(t, &dynamic->base, &local))
        return 1;
    year = (long long)local.tm_year + 1900;
    for (i = 0; i < 4; i++) {
        if (__c1_zone_year(year, dynamic, &zone) || __c1_zone_convert(t, &zone, &local))
            return 1;
        if (year == (long long)local.tm_year + 1900) {
            *result = local;
            return 0;
        }
        year = (long long)local.tm_year + 1900;
    }
    errno = ERANGE;
    return 1;
}

static time_t __c1_dynamic_mktime(struct tm *v, struct __c1_dynamic_zone *dynamic) {
    struct __c1_zone zone;
    struct tm local;
    time_t wall, value;
    long long year;
    int i, saved_errno = errno;
    if (!v) {
        errno = EINVAL;
        return -1;
    }
    wall = __c1_wall_seconds(v);
    if (!__c1_breakdown(&wall, &local))
        return -1;
    year = (long long)local.tm_year + 1900;
    for (i = 0; i < 4; i++) {
        if (__c1_zone_year(year, dynamic, &zone))
            return -1;
        local = *v;
        errno = 0;
        value = __c1_zone_mktime(&local, &zone);
        if (value == -1 && errno)
            return -1;
        if (year == (long long)local.tm_year + 1900) {
            *v = local;
            errno = saved_errno;
            return value;
        }
        year = (long long)local.tm_year + 1900;
    }
    errno = ERANGE;
    return -1;
}
