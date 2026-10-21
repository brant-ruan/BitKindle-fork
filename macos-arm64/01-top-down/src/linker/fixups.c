#ifndef C1_LINK_FIXUPS_C
#define C1_LINK_FIXUPS_C
#include "sections.c"
#include "symbols.c"
#include "relocate.c"

struct lk_fixup {
    int section, symbol, kind;
    unsigned long long offset;
    long long addend;
};

int lk_apply_fixups(struct lk_section *sections, unsigned char **payloads, int section_count,
                    struct lk_symbol *symbols, int symbol_count, struct lk_fixup *fixups, int count,
                    unsigned long long image_base) {
    unsigned long long bases[1024], target, place, width, previous_width;
    unsigned char probe[8];
    int i, j, pass, s, id, selected, target_section, status;
    if (section_count < 0 || section_count > 1024 || symbol_count < 0 || symbol_count > 4096 ||
        count < 0 || count > 16384 || (!sections && section_count) ||
        (!payloads && section_count) || (!symbols && symbol_count) || (!fixups && count))
        return 1;
    for (i = 0; i < section_count; i++)
        bases[i] = sections[i].address;
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < count; i++) {
            s = fixups[i].section;
            id = fixups[i].symbol;
            if (s < 0 || s >= section_count || id < 0 || id >= symbol_count ||
                fixups[i].kind < LK_ABS64 || fixups[i].kind > LK_ARM64_PAGEOFF12)
                return 1;
            width = (fixups[i].kind == LK_ABS64) ? 8 : 4;
            if (!payloads[s] || sections[s].file_size > sections[s].memory_size ||
                sections[s].file_size < width || fixups[i].offset > sections[s].file_size - width)
                return 1;
            if (sections[s].address > ~0ULL - fixups[i].offset)
                return 2;
            place = sections[s].address + fixups[i].offset;
            selected = symbols[id].selected;
            if (selected < 0)
                return 4;
            if (selected >= symbol_count)
                return 1;
            if (symbols[selected].binding == LK_LOCAL || symbols[selected].binding == LK_GLOBAL) {
                target_section = symbols[selected].section;
                if (target_section < 0 || target_section >= section_count)
                    return 1;
                if (symbols[selected].value > sections[target_section].memory_size ||
                    symbols[selected].size >
                        sections[target_section].memory_size - symbols[selected].value)
                    return 1;
            }
            status = lk_symbol_address(symbols, symbol_count, id, bases, section_count, &target);
            if (status)
                return status;
            if (!pass) {
                for (j = 0; j < i; j++) {
                    if (fixups[j].section != s)
                        continue;
                    previous_width = (fixups[j].kind == LK_ABS64) ? 8 : 4;
                    if (fixups[i].offset < fixups[j].offset + previous_width &&
                        fixups[j].offset < fixups[i].offset + width)
                        return 1;
                }
                status = lk_relocate(probe, width, 0, place, target, fixups[i].addend, image_base,
                                     fixups[i].kind);
                if (status)
                    return status;
            } else {
                status = lk_relocate(payloads[s], sections[s].file_size, fixups[i].offset, place,
                                     target, fixups[i].addend, image_base, fixups[i].kind);
                if (status)
                    return status;
            }
        }
    }
    return 0;
}

#endif
