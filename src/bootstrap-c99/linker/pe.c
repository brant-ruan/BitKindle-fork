#ifndef C1_LINK_PE_C
#define C1_LINK_PE_C
#include "writer.c"

/* Initial fixed-base PE32+ backend. No imports, base relocations, or unwind
   tables yet. Sections use 4 KiB virtual and 512-byte file alignment.
   Source alignments must divide those values. Maximum output is 64 MiB. */
int lk_pe_layout(struct lk_section *s, int count, unsigned long long *file_size,
                 unsigned long long *image_size) {
    unsigned long long address, offset, raw, span;
    int pass, i;
    if (!s || !file_size || !image_size || count < 1 || count > 96)
        return 1;
    for (i = 0; i < count; i++)
        if (!s[i].memory_size || s[i].file_size > s[i].memory_size || !s[i].alignment ||
            s[i].alignment > 4096 || (s[i].alignment & (s[i].alignment - 1)) ||
            !s[i].file_alignment || s[i].file_alignment > 512 ||
            (s[i].file_alignment & (s[i].file_alignment - 1)) || s[i].flags < 1 || s[i].flags > 7 ||
            s[i].memory_size > 0x7fff0000ULL || s[i].file_size > 67108864)
            return 1;
    for (pass = 0; pass < 2; pass++) {
        offset = (0x188 + (unsigned long long)count * 40 + 511) & ~511ULL;
        address = (offset + 4095) & ~4095ULL;
        for (i = 0; i < count; i++) {
            raw = (s[i].file_size + 511) & ~511ULL;
            span = (s[i].memory_size + 4095) & ~4095ULL;
            if (raw > 67108864 - offset || span > 0x7fff0000ULL - address)
                return 2;
            if (pass) {
                s[i].address = 0x400000 + address;
                s[i].file_offset = raw ? offset : 0;
            }
            offset += raw;
            address += span;
        }
    }
    *file_size = offset;
    *image_size = address;
    return 0;
}

int lk_pe_write(struct lk_section *sections, unsigned char **payloads, int count,
                unsigned long long entry, unsigned char *out, unsigned long long capacity,
                unsigned long long *written) {
    struct lk_section s[96];
    unsigned long long total, image_size, header, raw, code, data, bss, base_code, i, j;
    unsigned int flags;
    unsigned char *p;
    int n, status, entry_ok;
    if (!sections || !payloads || !written || count < 1 || count > 96 || (!out && capacity))
        return 1;
    for (n = 0; n < count; n++)
        s[n] = sections[n];
    status = lk_pe_layout(s, count, &total, &image_size);
    if (status)
        return status;
    entry_ok = 0;
    code = 0;
    data = 0;
    bss = 0;
    base_code = 0;
    for (n = 0; n < count; n++) {
        if (s[n].address != sections[n].address || s[n].file_offset != sections[n].file_offset ||
            (s[n].file_size && !payloads[n]))
            return 1;
        raw = (s[n].file_size + 511) & ~511ULL;
        if (s[n].flags & LK_EXECUTE) {
            code += raw;
            if (!base_code)
                base_code = s[n].address - 0x400000;
            if (entry >= s[n].address && entry - s[n].address < s[n].file_size)
                entry_ok = 1;
        } else
            data += raw;
        if (!raw)
            bss += (s[n].memory_size + 4095) & ~4095ULL;
    }
    if (!entry_ok)
        return 1;
    if (!out) {
        *written = total;
        return 0;
    }
    if (capacity < total)
        return 2;
    header = (0x188 + (unsigned long long)count * 40 + 511) & ~511ULL;
    for (i = 0; i < total; i++)
        out[i] = 0;
    lk_object_put32(out, 0x5a4d);
    lk_object_put32(out + 0x3c, 0x80);
    lk_object_put32(out + 0x80, 0x4550);
    lk_object_put32(out + 0x84, 0x8664 | ((unsigned int)count << 16));
    lk_object_put32(out + 0x94, 0x2300f0);
    lk_object_put32(out + 0x98, 0x20b);
    lk_object_put32(out + 0x9c, (unsigned int)code);
    lk_object_put32(out + 0xa0, (unsigned int)data);
    lk_object_put32(out + 0xa4, (unsigned int)bss);
    lk_object_put32(out + 0xa8, (unsigned int)(entry - 0x400000));
    lk_object_put32(out + 0xac, (unsigned int)base_code);
    lk_object_put64(out + 0xb0, 0x400000);
    lk_object_put32(out + 0xb8, 4096);
    lk_object_put32(out + 0xbc, 512);
    lk_object_put32(out + 0xc0, 6);
    lk_object_put32(out + 0xc8, 6);
    lk_object_put32(out + 0xd0, (unsigned int)image_size);
    lk_object_put32(out + 0xd4, (unsigned int)header);
    lk_object_put32(out + 0xdc, 0x1000003);
    lk_object_put64(out + 0xe0, 0x100000);
    lk_object_put64(out + 0xe8, 0x1000);
    lk_object_put64(out + 0xf0, 0x100000);
    lk_object_put64(out + 0xf8, 0x1000);
    lk_object_put32(out + 0x104, 16);
    for (n = 0; n < count; n++) {
        p = out + 0x188 + n * 40;
        p[0] = '.';
        p[1] = 's';
        p[2] = (unsigned char)('0' + n / 10);
        p[3] = (unsigned char)('0' + n % 10);
        raw = (s[n].file_size + 511) & ~511ULL;
        lk_object_put32(p + 8, (unsigned int)s[n].memory_size);
        lk_object_put32(p + 12, (unsigned int)(s[n].address - 0x400000));
        lk_object_put32(p + 16, (unsigned int)raw);
        lk_object_put32(p + 20, (unsigned int)s[n].file_offset);
        flags = (s[n].flags & LK_EXECUTE) ? 0x20 : (raw ? 0x40 : 0x80);
        if (s[n].flags & LK_READ)
            flags |= 0x40000000U;
        if (s[n].flags & LK_WRITE)
            flags |= 0x80000000U;
        if (s[n].flags & LK_EXECUTE)
            flags |= 0x20000000U;
        lk_object_put32(p + 36, flags);
        for (j = 0; j < s[n].file_size; j++)
            out[s[n].file_offset + j] = payloads[n][j];
    }
    *written = total;
    return 0;
}
#endif
