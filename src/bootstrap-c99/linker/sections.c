#ifndef C1_LINK_SECTIONS_C
#define C1_LINK_SECTIONS_C
#define LK_READ 1
#define LK_WRITE 2
#define LK_EXECUTE 4

struct lk_section {
    unsigned long long memory_size, file_size, alignment, file_alignment;
    unsigned long long address, file_offset;
    int flags;
};

/* Layout in input order. Zero-fill sections consume virtual space only and
   get file_offset=0. The caller chooses alignments appropriate to its output
   format. No output fields or end pointers are changed on failure. */
int lk_layout_sections(struct lk_section *s, int count, unsigned long long memory_base,
                       unsigned long long memory_limit, unsigned long long file_base,
                       unsigned long long file_limit, unsigned long long *memory_end,
                       unsigned long long *file_end) {
    unsigned long long memory_cursor, file_cursor, padding, offset;
    int i, pass;
    if (count < 0 || count > 1024 || (!s && count) || !memory_end || !file_end ||
        memory_base > memory_limit || file_base > file_limit)
        return 1;
    for (i = 0; i < count; i++) {
        if (s[i].file_size > s[i].memory_size || !s[i].alignment ||
            (s[i].alignment & (s[i].alignment - 1)) || !s[i].file_alignment ||
            (s[i].file_alignment & (s[i].file_alignment - 1)) || s[i].flags < 1 || s[i].flags > 7)
            return 1;
    }
    for (pass = 0; pass < 2; pass++) {
        memory_cursor = memory_base;
        file_cursor = file_base;
        for (i = 0; i < count; i++) {
            padding = (0ULL - memory_cursor) & (s[i].alignment - 1);
            if (padding > memory_limit - memory_cursor)
                return 3;
            memory_cursor += padding;
            if (s[i].memory_size > memory_limit - memory_cursor)
                return 3;
            offset = 0;
            if (s[i].file_size) {
                padding = (0ULL - file_cursor) & (s[i].file_alignment - 1);
                if (padding > file_limit - file_cursor)
                    return 3;
                file_cursor += padding;
                if (s[i].file_size > file_limit - file_cursor)
                    return 3;
                offset = file_cursor;
                file_cursor += s[i].file_size;
            }
            if (pass) {
                s[i].address = memory_cursor;
                s[i].file_offset = offset;
            }
            memory_cursor += s[i].memory_size;
        }
    }
    *memory_end = memory_cursor;
    *file_end = file_cursor;
    return 0;
}
#endif
