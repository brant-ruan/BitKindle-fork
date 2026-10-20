#ifndef C1_LINK_RELOCATE_C
#define C1_LINK_RELOCATE_C
/* Private linker-core kinds; these are not a COFF file-format declaration. */
#define LK_ABS64 1
#define LK_ABS32 2
#define LK_REL32 3
#define LK_RVA32 4

/* Return 0 on success, 1 for kind/buffer errors, 2 for address arithmetic
   overflow, 3 for an unrepresentable field. On any error leave bytes intact.
   place is the virtual address of the first byte of the relocation field. */
int lk_relocate(unsigned char *bytes, unsigned long long length, unsigned long long offset,
                unsigned long long place, unsigned long long target, long long addend,
                unsigned long long image_base, int kind) {
    unsigned long long value, magnitude, end, delta;
    int width, i;
    if (kind < LK_ABS64 || kind > LK_RVA32)
        return 1;
    width = kind == LK_ABS64 ? 8 : 4;
    if (!bytes || length < (unsigned long long)width || offset > length - (unsigned long long)width)
        return 1;
    if (addend >= 0) {
        if (target > ~0ULL - (unsigned long long)addend)
            return 2;
        value = target + (unsigned long long)addend;
    } else {
        magnitude = 0ULL - (unsigned long long)addend;
        if (target < magnitude)
            return 2;
        value = target - magnitude;
    }
    if (kind == LK_ABS32 && value > 0xffffffffULL)
        return 3;
    if (kind == LK_RVA32) {
        if (value < image_base || value - image_base > 0xffffffffULL)
            return 3;
        value -= image_base;
    }
    if (kind == LK_REL32) {
        if (place > ~0ULL - 4)
            return 2;
        end = place + 4;
        if (value >= end) {
            delta = value - end;
            if (delta > 0x7fffffffULL)
                return 3;
            value = delta;
        } else {
            delta = end - value;
            if (delta > 0x80000000ULL)
                return 3;
            value = 0ULL - delta;
        }
    }
    for (i = 0; i < width; i++) {
        bytes[offset + i] = (unsigned char)value;
        value >>= 8;
    }
    return 0;
}
#endif
