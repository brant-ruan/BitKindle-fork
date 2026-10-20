#ifndef C1_LINK_OBJECT_C
#define C1_LINK_OBJECT_C
#include "fixups.c"

struct lk_object_view {
    unsigned char *bytes;
    unsigned long long length, section_offset, symbol_offset, fixup_offset, name_offset,
        payload_offset;
    unsigned int sections, symbols, fixups, names, payload;
};

static unsigned int lk_object_u32(unsigned char *p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) |
           ((unsigned int)p[3] << 24);
}

static unsigned long long lk_object_u64(unsigned char *p) {
    return (unsigned long long)lk_object_u32(p) | ((unsigned long long)lk_object_u32(p + 4) << 32);
}

/* C1O1 is a private canonical object representation, not COFF. Validate
   every extent before publishing a view. The caller retains the byte buffer. */
int lk_object_read(unsigned char *bytes, unsigned long long length, struct lk_object_view *out) {
    struct lk_object_view v;
    unsigned char *p, *q;
    unsigned long long memory_size, file_size, alignment, file_alignment, offset, size, cursor,
        total, width;
    unsigned int i, section, binding, n;
    if (!bytes || !out || length < 32)
        return 1;
    if (bytes[0] != 'C' || bytes[1] != '1' || bytes[2] != 'O' || bytes[3] != '1' ||
        lk_object_u32(bytes + 4) != 0x8664 || lk_object_u32(bytes + 28))
        return 1;
    v.bytes = bytes;
    v.length = length;
    v.sections = lk_object_u32(bytes + 8);
    v.symbols = lk_object_u32(bytes + 12);
    v.fixups = lk_object_u32(bytes + 16);
    v.names = lk_object_u32(bytes + 20);
    v.payload = lk_object_u32(bytes + 24);
    if (v.sections > 1024 || v.symbols > 4096 || v.fixups > 16384 || v.names > 1048576 ||
        v.payload > 16777216)
        return 1;
    v.section_offset = 32;
    v.symbol_offset = v.section_offset + (unsigned long long)v.sections * 48;
    v.fixup_offset = v.symbol_offset + (unsigned long long)v.symbols * 40;
    v.name_offset = v.fixup_offset + (unsigned long long)v.fixups * 32;
    v.payload_offset = v.name_offset + v.names;
    total = v.payload_offset + v.payload;
    if (total != length)
        return 1;
    cursor = 0;
    for (i = 0; i < v.sections; i++) {
        p = bytes + v.section_offset + (unsigned long long)i * 48;
        memory_size = lk_object_u64(p);
        file_size = lk_object_u64(p + 8);
        alignment = lk_object_u64(p + 16);
        file_alignment = lk_object_u64(p + 24);
        offset = lk_object_u64(p + 32);
        if (file_size > memory_size || !alignment || (alignment & (alignment - 1)) ||
            !file_alignment || (file_alignment & (file_alignment - 1)) ||
            lk_object_u32(p + 40) < 1 || lk_object_u32(p + 40) > 7 || lk_object_u32(p + 44))
            return 1;
        if (file_size) {
            if (offset != cursor || file_size > v.payload - cursor)
                return 1;
            cursor += file_size;
        } else if (offset)
            return 1;
    }
    if (cursor != v.payload)
        return 1;
    cursor = 0;
    for (i = 0; i < v.symbols; i++) {
        p = bytes + v.symbol_offset + (unsigned long long)i * 40;
        offset = lk_object_u32(p);
        binding = lk_object_u32(p + 4);
        section = lk_object_u32(p + 8);
        if (binding > LK_COMMON || lk_object_u32(p + 12) || offset != cursor || cursor >= v.names)
            return 1;
        n = 0;
        while (n < 256 && cursor + n < v.names && bytes[v.name_offset + cursor + n])
            n++;
        if (!n || n == 256 || cursor + n >= v.names)
            return 1;
        cursor += n + 1;
        offset = lk_object_u64(p + 16);
        size = lk_object_u64(p + 24);
        alignment = lk_object_u64(p + 32);
        if (!alignment || (alignment & (alignment - 1)))
            return 1;
        if (binding == LK_LOCAL || binding == LK_GLOBAL) {
            if (section >= v.sections)
                return 1;
            q = bytes + v.section_offset + (unsigned long long)section * 48;
            memory_size = lk_object_u64(q);
            if (offset > memory_size || size > memory_size - offset)
                return 1;
        } else if (section != 0xffffffffU || offset)
            return 1;
    }
    if (cursor != v.names)
        return 1;
    for (i = 0; i < v.fixups; i++) {
        p = bytes + v.fixup_offset + (unsigned long long)i * 32;
        section = lk_object_u32(p);
        binding = lk_object_u32(p + 8);
        if (section >= v.sections || lk_object_u32(p + 4) >= v.symbols || binding < LK_ABS64 ||
            binding > LK_RVA32 || lk_object_u32(p + 12))
            return 1;
        q = bytes + v.section_offset + (unsigned long long)section * 48;
        file_size = lk_object_u64(q + 8);
        offset = lk_object_u64(p + 16);
        width = binding == LK_ABS64 ? 8 : 4;
        if (file_size < width || offset > file_size - width)
            return 1;
    }
    *out = v;
    return 0;
}

int lk_object_section(struct lk_object_view *v, int index, struct lk_section *out,
                      unsigned char **payload) {
    unsigned char *p;
    struct lk_section s;
    if (!v || !out || !payload || index < 0 || (unsigned int)index >= v->sections)
        return 1;
    p = v->bytes + v->section_offset + (unsigned long long)index * 48;
    s.memory_size = lk_object_u64(p);
    s.file_size = lk_object_u64(p + 8);
    s.alignment = lk_object_u64(p + 16);
    s.file_alignment = lk_object_u64(p + 24);
    s.flags = (int)lk_object_u32(p + 40);
    s.address = 0;
    s.file_offset = 0;
    *payload = s.file_size ? v->bytes + v->payload_offset + lk_object_u64(p + 32) : 0;
    *out = s;
    return 0;
}

int lk_object_symbol(struct lk_object_view *v, int index, int module, struct lk_symbol *out) {
    unsigned char *p;
    struct lk_symbol s;
    if (!v || !out || index < 0 || (unsigned int)index >= v->symbols || module < 0)
        return 1;
    p = v->bytes + v->symbol_offset + (unsigned long long)index * 40;
    s.name = (char *)(v->bytes + v->name_offset + lk_object_u32(p));
    s.module = module;
    s.binding = (int)lk_object_u32(p + 4);
    s.section = (int)lk_object_u32(p + 8);
    s.value = lk_object_u64(p + 16);
    s.size = lk_object_u64(p + 24);
    s.alignment = lk_object_u64(p + 32);
    s.selected = -1;
    s.address_ready = 0;
    *out = s;
    return 0;
}

int lk_object_fixup(struct lk_object_view *v, int index, struct lk_fixup *out) {
    unsigned char *p;
    struct lk_fixup f;
    if (!v || !out || index < 0 || (unsigned int)index >= v->fixups)
        return 1;
    p = v->bytes + v->fixup_offset + (unsigned long long)index * 32;
    f.section = (int)lk_object_u32(p);
    f.symbol = (int)lk_object_u32(p + 4);
    f.kind = (int)lk_object_u32(p + 8);
    f.offset = lk_object_u64(p + 16);
    f.addend = (long long)lk_object_u64(p + 24);
    *out = f;
    return 0;
}
#endif
