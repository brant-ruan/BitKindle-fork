#ifndef C1_LINK_MERGE_C
#define C1_LINK_MERGE_C
#include "object.c"

struct lk_object_counts {
    int sections, symbols, fixups;
};

int lk_merge_objects(unsigned char **objects, unsigned long long *lengths, int count,
                     struct lk_section *sections, unsigned char **payloads, int section_capacity,
                     struct lk_symbol *symbols, int symbol_capacity, struct lk_fixup *fixups,
                     int fixup_capacity, struct lk_object_counts *counts) {
    struct lk_object_view v;
    int i, j, ns, ny, nf, bs, by, bf;
    if (!counts || count < 0 || count > 1024 || (count && (!objects || !lengths)) ||
        section_capacity < 0 || symbol_capacity < 0 || fixup_capacity < 0)
        return 1;
    ns = 0;
    ny = 0;
    nf = 0;
    for (i = 0; i < count; i++) {
        if (lk_object_read(objects[i], lengths[i], &v))
            return 1;
        if (v.sections > (unsigned int)(1024 - ns) || v.symbols > (unsigned int)(4096 - ny) ||
            v.fixups > (unsigned int)(16384 - nf))
            return 2;
        ns += (int)v.sections;
        ny += (int)v.symbols;
        nf += (int)v.fixups;
    }
    if (ns > section_capacity || ny > symbol_capacity || nf > fixup_capacity)
        return 2;
    if ((ns && (!sections || !payloads)) || (ny && !symbols) || (nf && !fixups))
        return 1;
    bs = 0;
    by = 0;
    bf = 0;
    for (i = 0; i < count; i++) {
        lk_object_read(objects[i], lengths[i], &v);
        for (j = 0; j < (int)v.sections; j++)
            lk_object_section(&v, j, sections + bs + j, payloads + bs + j);
        for (j = 0; j < (int)v.symbols; j++) {
            lk_object_symbol(&v, j, i, symbols + by + j);
            if (symbols[by + j].section >= 0)
                symbols[by + j].section += bs;
        }
        for (j = 0; j < (int)v.fixups; j++) {
            lk_object_fixup(&v, j, fixups + bf + j);
            fixups[bf + j].section += bs;
            fixups[bf + j].symbol += by;
        }
        bs += (int)v.sections;
        by += (int)v.symbols;
        bf += (int)v.fixups;
    }
    counts->sections = ns;
    counts->symbols = ny;
    counts->fixups = nf;
    return 0;
}

#endif
