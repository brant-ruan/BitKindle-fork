#ifndef C1_LINK_SYMBOLS_C
#define C1_LINK_SYMBOLS_C
#define LK_UNDEFINED 0
#define LK_LOCAL 1
#define LK_GLOBAL 2
#define LK_COMMON 3

struct lk_symbol {
    const char *name;
    int module, binding, section;
    unsigned long long value, size, alignment;
    int selected, address_ready;
};

static int lk_name_equal(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* Validate first; a rejected table is not partially rebound. Undefined
   declarations may remain unused. Address lookup reports unresolved uses. */
int lk_bind_symbols(struct lk_symbol *s, int count) {
    int i, j, n, chosen;
    if (count < 0 || count > 4096 || (!s && count))
        return 1;
    for (i = 0; i < count; i++) {
        if (!s[i].name || !s[i].name[0] || s[i].module < 0 || s[i].binding < 0 || s[i].binding > 3)
            return 1;
        n = 0;
        while (n < 256 && s[i].name[n])
            n++;
        if (n == 256)
            return 1;
        if (!s[i].alignment || (s[i].alignment & (s[i].alignment - 1)))
            return 1;
        if (s[i].binding == LK_LOCAL || s[i].binding == LK_GLOBAL) {
            if (s[i].section < 0 || s[i].value > ~0ULL - s[i].size)
                return 1;
        } else if (s[i].section != -1 || s[i].value)
            return 1;
    }
    for (i = 0; i < count; i++)
        for (j = 0; j < i; j++) {
            if (!lk_name_equal(s[i].name, s[j].name))
                continue;
            if (s[i].binding == LK_GLOBAL && s[j].binding == LK_GLOBAL)
                return 2;
            if (s[i].module == s[j].module && s[i].binding != LK_UNDEFINED &&
                s[j].binding != LK_UNDEFINED &&
                (s[i].binding == LK_LOCAL || s[j].binding == LK_LOCAL))
                return 2;
        }
    for (i = 0; i < count; i++) {
        chosen = -1;
        if (s[i].binding == LK_LOCAL || s[i].binding == LK_GLOBAL)
            chosen = i;
        else {
            for (j = 0; j < count; j++)
                if (s[j].binding == LK_GLOBAL && lk_name_equal(s[i].name, s[j].name)) {
                    chosen = j;
                    break;
                }
            if (chosen < 0)
                for (j = 0; j < count; j++)
                    if (s[j].binding == LK_COMMON && lk_name_equal(s[i].name, s[j].name)) {
                        chosen = j;
                        break;
                    }
        }
        s[i].selected = chosen;
        s[i].address_ready = 0;
    }
    return 0;
}

/* Merge common storage by maximum size/alignment, in first-symbol order.
   A strong definition overrides common storage. Two passes make an exhausted
   address range fail without publishing any partially assigned addresses. */
int lk_layout_common(struct lk_symbol *s, int count, unsigned long long base,
                     unsigned long long limit, unsigned long long *end) {
    unsigned long long cursor, alignment, size, padding;
    int pass, i, j;
    if (count < 0 || count > 4096 || (!s && count) || !end || base > limit)
        return 1;
    for (pass = 0; pass < 2; pass++) {
        cursor = base;
        for (i = 0; i < count; i++)
            if (s[i].binding == LK_COMMON && s[i].selected == i) {
                size = 0;
                alignment = 1;
                for (j = 0; j < count; j++)
                    if (s[j].binding == LK_COMMON && s[j].selected == i) {
                        if (s[j].size > size)
                            size = s[j].size;
                        if (s[j].alignment > alignment)
                            alignment = s[j].alignment;
                    }
                padding = (0ULL - cursor) & (alignment - 1);
                if (padding > limit - cursor)
                    return 3;
                cursor += padding;
                if (size > limit - cursor)
                    return 3;
                if (pass) {
                    s[i].value = cursor;
                    s[i].address_ready = 1;
                }
                cursor += size;
            }
    }
    *end = cursor;
    return 0;
}

int lk_symbol_address(struct lk_symbol *s, int count, int id, unsigned long long *section_base,
                      int sections, unsigned long long *address) {
    int chosen, section;
    if (!s || !address || count < 0 || count > 4096 || id < 0 || id >= count || sections < 0)
        return 1;
    chosen = s[id].selected;
    if (chosen < 0)
        return 4;
    if (chosen >= count)
        return 1;
    if (s[chosen].binding == LK_COMMON) {
        if (!s[chosen].address_ready)
            return 4;
        *address = s[chosen].value;
        return 0;
    }
    if (s[chosen].binding != LK_LOCAL && s[chosen].binding != LK_GLOBAL)
        return 4;
    section = s[chosen].section;
    if (!section_base || section < 0 || section >= sections)
        return 1;
    if (section_base[section] > ~0ULL - s[chosen].value)
        return 3;
    *address = section_base[section] + s[chosen].value;
    return 0;
}
#endif
