#ifndef C1_LINK_IMPORTS_C
#define C1_LINK_IMPORTS_C
#include "writer.c"

struct lk_import {
    const char *dll, *name;
    unsigned int ordinal;
};

struct lk_import_layout {
    unsigned int size, descriptor_size, iat_offset, iat_size;
};

/* Exact DLL spelling determines grouping; order is first occurrence. Each
   record gets its own IAT slot, including repeated imports. Name imports have
   ordinal zero; ordinal imports have NULL name and ordinal 1..65535. Input,
   output, slot_rva and layout storage must be disjoint and remain live.
   NULL out/capacity zero queries layout. Failure changes no output. */
int lk_build_imports(struct lk_import *imports, int count, unsigned int section_rva,
                     unsigned char *out, unsigned long long capacity, unsigned int *slot_rva,
                     struct lk_import_layout *layout) {
    int group[256], leaders[256], dll_length[256], name_length[256], members[256];
    unsigned int ilt[256], iat[256], dll_offset[256], name_offset[256], slots[256];
    unsigned int cursor, descriptors, iat_start, iat_size, slot;
    int i, j, g, groups = 0, n;
    unsigned long long thunk;
    unsigned char *p;
    if (!layout || count < 0 || count > 256 || (count && (!imports || !slot_rva)) ||
        (!out && capacity))
        return 1;
    if (!count) {
        layout->size = 0;
        layout->descriptor_size = 0;
        layout->iat_offset = 0;
        layout->iat_size = 0;
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (!imports[i].dll || imports[i].ordinal > 65535)
            return 1;
        n = 0;
        while (n < 256 && imports[i].dll[n]) {
            if ((unsigned char)imports[i].dll[n] < 33 || (unsigned char)imports[i].dll[n] > 126)
                return 1;
            n++;
        }
        if (!n || n == 256)
            return 1;
        dll_length[i] = n;
        if (imports[i].ordinal) {
            if (imports[i].name)
                return 1;
            name_length[i] = 0;
        } else {
            if (!imports[i].name)
                return 1;
            n = 0;
            while (n < 256 && imports[i].name[n]) {
                if ((unsigned char)imports[i].name[n] < 33 ||
                    (unsigned char)imports[i].name[n] > 126)
                    return 1;
                n++;
            }
            if (!n || n == 256)
                return 1;
            name_length[i] = n;
        }
        for (g = 0; g < groups; g++)
            if (lk_name_equal(imports[i].dll, imports[leaders[g]].dll))
                break;
        if (g == groups) {
            leaders[groups++] = i;
            members[g] = 0;
        }
        group[i] = g;
        members[g]++;
    }
    descriptors = (unsigned int)(groups + 1) * 20;
    cursor = (descriptors + 7) & ~7U;
    for (g = 0; g < groups; g++) {
        ilt[g] = cursor;
        cursor += (members[g] + 1) * 8;
    }
    iat_start = cursor;
    for (g = 0; g < groups; g++) {
        iat[g] = cursor;
        cursor += (members[g] + 1) * 8;
    }
    iat_size = cursor - iat_start;
    for (g = 0; g < groups; g++) {
        dll_offset[g] = cursor;
        cursor += dll_length[leaders[g]] + 1;
    }
    for (i = 0; i < count; i++) {
        name_offset[i] = 0;
        if (!imports[i].ordinal) {
            cursor = (cursor + 1) & ~1U;
            name_offset[i] = cursor;
            cursor += name_length[i] + 3;
        }
    }
    if (cursor > 0xffffffffU - section_rva)
        return 1;
    if (out && capacity < cursor)
        return 2;
    for (g = 0; g < groups; g++)
        members[g] = 0;
    for (i = 0; i < count; i++) {
        g = group[i];
        slots[i] = iat[g] + members[g] * 8;
        members[g]++;
    }
    if (out) {
        for (slot = 0; slot < cursor; slot++)
            out[slot] = 0;
        for (g = 0; g < groups; g++) {
            p = out + g * 20;
            lk_object_put32(p, section_rva + ilt[g]);
            lk_object_put32(p + 12, section_rva + dll_offset[g]);
            lk_object_put32(p + 16, section_rva + iat[g]);
            for (j = 0; j < dll_length[leaders[g]]; j++)
                out[dll_offset[g] + j] = (unsigned char)imports[leaders[g]].dll[j];
        }
        for (i = 0; i < count; i++) {
            g = group[i];
            thunk = imports[i].ordinal ? (0x8000000000000000ULL | imports[i].ordinal)
                                       : (section_rva + name_offset[i]);
            lk_object_put64(out + slots[i], thunk);
            lk_object_put64(out + ilt[g] + slots[i] - iat[g], thunk);
            if (!imports[i].ordinal)
                for (j = 0; j < name_length[i]; j++)
                    out[name_offset[i] + 2 + j] = (unsigned char)imports[i].name[j];
        }
    }
    for (i = 0; i < count; i++)
        slot_rva[i] = section_rva + slots[i];
    layout->size = cursor;
    layout->descriptor_size = descriptors;
    layout->iat_offset = iat_start;
    layout->iat_size = iat_size;
    return 0;
}
#endif
