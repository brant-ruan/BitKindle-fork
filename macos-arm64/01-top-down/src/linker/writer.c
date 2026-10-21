#ifndef C1_LINK_WRITER_C
#define C1_LINK_WRITER_C
#include "object.c"

static void lk_object_put32(unsigned char *p, unsigned int v) {
    int i;
    for (i = 0; i < 4; i++) {
        p[i] = (unsigned char)v;
        v >>= 8;
    }
}

static void lk_object_put64(unsigned char *p, unsigned long long v) {
    int i;
    for (i = 0; i < 8; i++) {
        p[i] = (unsigned char)v;
        v >>= 8;
    }
}

int lk_object_write(struct lk_section *sections, unsigned char **payloads, int section_count,
                    struct lk_symbol *symbols, int symbol_count, struct lk_fixup *fixups,
                    int fixup_count, unsigned char *out, unsigned long long capacity,
                    unsigned long long *written) {
    int i, j, n, lengths[4096];
    unsigned long long names, payload, total, so, fo, no, po, nc, pc, width;
    unsigned char *p;
    struct lk_section *s;
    struct lk_symbol *y;
    struct lk_fixup *f;
    if (!written || section_count < 0 || section_count > 1024 || symbol_count < 0 ||
        symbol_count > 4096 || fixup_count < 0 || fixup_count > 16384 || (!out && capacity))
        return 1;
    if ((section_count && (!sections || !payloads)) || (symbol_count && !symbols) ||
        (fixup_count && !fixups))
        return 1;
    payload = 0;
    names = 0;
    for (i = 0; i < section_count; i++) {
        s = sections + i;
        if (s->file_size > s->memory_size || !s->alignment || (s->alignment & (s->alignment - 1)) ||
            !s->file_alignment || (s->file_alignment & (s->file_alignment - 1)) || s->flags < 1 ||
            s->flags > 7)
            return 1;
        if (s->file_size > 16777216 - payload || (s->file_size && !payloads[i]))
            return 1;
        payload += s->file_size;
    }
    for (i = 0; i < symbol_count; i++) {
        y = symbols + i;
        if (!y->name || y->binding < LK_UNDEFINED || y->binding > LK_COMMON || !y->alignment ||
            (y->alignment & (y->alignment - 1)))
            return 1;
        n = 0;
        while (n < 256 && y->name[n])
            n++;
        if (!n || n == 256)
            return 1;
        lengths[i] = n + 1;
        names += n + 1;
        if (names > 1048576)
            return 1;
        if (y->binding == LK_LOCAL || y->binding == LK_GLOBAL) {
            if (y->section < 0 || y->section >= section_count)
                return 1;
            s = sections + y->section;
            if (y->value > s->memory_size || y->size > s->memory_size - y->value)
                return 1;
        } else if (y->section != -1 || y->value)
            return 1;
    }
    for (i = 0; i < fixup_count; i++) {
        f = fixups + i;
        if (f->section < 0 || f->section >= section_count || f->symbol < 0 ||
            f->symbol >= symbol_count || f->kind < LK_ABS64 || f->kind > LK_ARM64_PAGEOFF12)
            return 1;
        width = (f->kind == LK_ABS64) ? 8 : 4;
        s = sections + f->section;
        if (s->file_size < width || f->offset > s->file_size - width)
            return 1;
    }
    so = 32 + (unsigned long long)section_count * 48;
    fo = so + (unsigned long long)symbol_count * 40;
    no = fo + (unsigned long long)fixup_count * 32;
    po = no + names;
    total = po + payload;
    if (!out) {
        *written = total;
        return 0;
    }
    if (capacity < total)
        return 2;
    for (pc = 0; pc < total; pc++)
        out[pc] = 0;
    out[0] = 'C';
    out[1] = '1';
    out[2] = 'O';
    out[3] = '1';
    lk_object_put32(out + 4, 0xAA64); /* ARM64 目标文件标识 */
    lk_object_put32(out + 8, section_count);
    lk_object_put32(out + 12, symbol_count);
    lk_object_put32(out + 16, fixup_count);
    lk_object_put32(out + 20, (unsigned int)names);
    lk_object_put32(out + 24, (unsigned int)payload);
    pc = 0;
    for (i = 0; i < section_count; i++) {
        s = sections + i;
        p = out + 32 + (unsigned long long)i * 48;
        lk_object_put64(p, s->memory_size);
        lk_object_put64(p + 8, s->file_size);
        lk_object_put64(p + 16, s->alignment);
        lk_object_put64(p + 24, s->file_alignment);
        lk_object_put64(p + 32, s->file_size ? pc : 0);
        lk_object_put32(p + 40, s->flags);
        for (nc = 0; nc < s->file_size; nc++)
            out[po + pc + nc] = payloads[i][nc];
        pc += s->file_size;
    }
    nc = 0;
    for (i = 0; i < symbol_count; i++) {
        y = symbols + i;
        p = out + so + (unsigned long long)i * 40;
        lk_object_put32(p, (unsigned int)nc);
        lk_object_put32(p + 4, y->binding);
        lk_object_put32(p + 8, y->section);
        lk_object_put64(p + 16, y->value);
        lk_object_put64(p + 24, y->size);
        lk_object_put64(p + 32, y->alignment);
        for (j = 0; j < lengths[i]; j++)
            out[no + nc + j] = (unsigned char)y->name[j];
        nc += lengths[i];
    }
    for (i = 0; i < fixup_count; i++) {
        f = fixups + i;
        p = out + fo + (unsigned long long)i * 32;
        lk_object_put32(p, f->section);
        lk_object_put32(p + 4, f->symbol);
        lk_object_put32(p + 8, f->kind);
        lk_object_put64(p + 16, f->offset);
        lk_object_put64(p + 24, (unsigned long long)f->addend);
    }
    *written = total;
    return 0;
}

#endif
