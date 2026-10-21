#ifndef C1_LINK_RELOCATE_C
#define C1_LINK_RELOCATE_C

/* Relocation kinds: 原版 4 种 + ARM64 扩展类型 */
#define LK_ABS64 1
#define LK_ABS32 2
#define LK_REL32 3
#define LK_RVA32 4
#define LK_ARM64_BRANCH26 5
#define LK_ARM64_PAGE21 6
#define LK_ARM64_PAGEOFF12 7

/* Return 0 on success, 1 for kind/buffer errors, 2 for address arithmetic
   overflow, 3 for an unrepresentable field. On any error leave bytes intact.
   place is the virtual address of the first byte of the relocation field. */
int lk_relocate(unsigned char *bytes, unsigned long long length, unsigned long long offset,
                unsigned long long place, unsigned long long target, long long addend,
                unsigned long long image_base, int kind) {
    unsigned long long value, magnitude, end, delta;
    int width, i;

    if (kind < LK_ABS64 || kind > LK_ARM64_PAGEOFF12)
        return 1;

    /* 计算目标实际地址 value = target + addend */
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

    /* ARM64 B / BL: 26-bit PC-relative branch */
    if (kind == LK_ARM64_BRANCH26) {
        long long branch_delta;
        unsigned int insn;
        if (length < 4 || offset > length - 4)
            return 1;
        branch_delta = (long long)value - (long long)place;
        if (branch_delta & 3)
            return 3; /* 必须 4 字节对齐 */
        if (branch_delta < -134217728LL || branch_delta > 134217724LL)
            return 3; /* 越界 (±128MB) */
        insn = (unsigned int)bytes[offset] | ((unsigned int)bytes[offset + 1] << 8) |
               ((unsigned int)bytes[offset + 2] << 16) | ((unsigned int)bytes[offset + 3] << 24);
        insn = (insn & 0xFC000000) | (((unsigned int)(branch_delta >> 2)) & 0x03FFFFFF);
        bytes[offset] = (unsigned char)insn;
        bytes[offset + 1] = (unsigned char)(insn >> 8);
        bytes[offset + 2] = (unsigned char)(insn >> 16);
        bytes[offset + 3] = (unsigned char)(insn >> 24);
        return 0;
    }

    /* ARM64 ADRP: 21-bit page relative offset */
    if (kind == LK_ARM64_PAGE21) {
        long long page_delta;
        unsigned int insn, immlo, immhi;
        if (length < 4 || offset > length - 4)
            return 1;
        page_delta = (long long)(value & ~0xFFFULL) - (long long)(place & ~0xFFFULL);
        page_delta >>= 12;
        if (page_delta < -1048576LL || page_delta > 1048575LL)
            return 3; /* 越界 (±4GB) */
        insn = (unsigned int)bytes[offset] | ((unsigned int)bytes[offset + 1] << 8) |
               ((unsigned int)bytes[offset + 2] << 16) | ((unsigned int)bytes[offset + 3] << 24);
        immlo = ((unsigned int)page_delta) & 3;
        immhi = (((unsigned int)page_delta) >> 2) & 0x7FFFF;
        insn = (insn & 0x9F00001F) | (immlo << 29) | (immhi << 5);
        bytes[offset] = (unsigned char)insn;
        bytes[offset + 1] = (unsigned char)(insn >> 8);
        bytes[offset + 2] = (unsigned char)(insn >> 16);
        bytes[offset + 3] = (unsigned char)(insn >> 24);
        return 0;
    }

    /* ARM64 Page Offset: 12-bit offset (ADD / LDR / STR) */
    if (kind == LK_ARM64_PAGEOFF12) {
        unsigned int insn, imm12;
        if (length < 4 || offset > length - 4)
            return 1;
        imm12 = (unsigned int)(value & 0xFFF);
        insn = (unsigned int)bytes[offset] | ((unsigned int)bytes[offset + 1] << 8) |
               ((unsigned int)bytes[offset + 2] << 16) | ((unsigned int)bytes[offset + 3] << 24);
        /* 针对 ADD 指令的 12 位立即数修补 */
        insn = (insn & 0xFFC003FF) | (imm12 << 10);
        bytes[offset] = (unsigned char)insn;
        bytes[offset + 1] = (unsigned char)(insn >> 8);
        bytes[offset + 2] = (unsigned char)(insn >> 16);
        bytes[offset + 3] = (unsigned char)(insn >> 24);
        return 0;
    }

    /* 通用基本类型 */
    width = (kind == LK_ABS64) ? 8 : 4;
    if (!bytes || length < (unsigned long long)width || offset > length - (unsigned long long)width)
        return 1;

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
