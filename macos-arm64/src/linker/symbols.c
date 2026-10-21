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
            for (j = 0; j < count; j++) {
                if (s[j].binding != LK_GLOBAL || !lk_name_equal(s[i].name, s[j].name))
                    continue;
                chosen = j;
                break;
            }
            if (chosen < 0 && s[i].binding == LK_COMMON) {
                for (j = 0; j < count; j++) {
                    if (s[j].binding != LK_COMMON || !lk_name_equal(s[i].name, s[j].name))
                        continue;
                    if (chosen < 0) {
                        chosen = j;
                        continue;
                    }
                    if (s[j].size > s[chosen].size)
                        s[chosen].size = s[j].size;
                    if (s[j].alignment > s[chosen].alignment)
                        s[chosen].alignment = s[j].alignment;
                }
            }
        }
        s[i].selected = chosen;
    }
    return 0;
}

int lk_symbol_address(struct lk_symbol *symbols, int count, int index,
                      unsigned long long *bases, int section_count,
                      unsigned long long *address) {
    int target;
    if (!symbols || count < 0 || index < 0 || index >= count || !bases || !address)
        return 1;
    target = symbols[index].selected;
    if (target < 0)
        return 4; /* 未解析符号 */
    if (symbols[target].section < 0 || symbols[target].section >= section_count)
        return 1;
    *address = bases[symbols[target].section] + symbols[target].value;
    return 0;
}

#endif
