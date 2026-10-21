#ifndef C1_LINK_MACHO_C
#define C1_LINK_MACHO_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mach-o/loader.h>
#include <sys/stat.h>
#include "sections.c"

#define MACHO_PAGE_SIZE 0x4000ULL  /* Apple Silicon 16KB */

/* Mach-O 布局结构 */
struct lk_macho_layout {
    unsigned long long image_base;
    unsigned long long text_vmaddr;
    unsigned long long text_vmsize;
    unsigned long long text_fileoff;
    unsigned long long text_filesize;

    unsigned long long data_vmaddr;
    unsigned long long data_vmsize;
    unsigned long long data_fileoff;
    unsigned long long data_filesize;

    unsigned long long linkedit_vmaddr;
    unsigned long long linkedit_vmsize;
    unsigned long long linkedit_fileoff;
    unsigned long long linkedit_filesize;

    unsigned long long total_file_size;
    unsigned int ncmds;
    unsigned int sizeofcmds;
    unsigned long long entry_fileoff;
};

/* 计算页面向上对齐 */
static unsigned long long lk_align_page(unsigned long long val) {
    return (val + MACHO_PAGE_SIZE - 1) & ~(MACHO_PAGE_SIZE - 1);
}

/* 布局 Mach-O 镜像 */
int lk_macho_compute_layout(struct lk_section *sections, int section_count,
                           unsigned long long entry_addr, struct lk_macho_layout *layout) {
    int i;
    unsigned long long text_cursor, data_cursor;
    unsigned int cmds_size = 0;
    unsigned int cmd_count = 0;
    int has_data = 0;
    int text_sect_count = 0;
    int data_sect_count = 0;

    if (!sections || section_count <= 0 || !layout)
        return 1;

    memset(layout, 0, sizeof(*layout));
    layout->image_base = 0x100000000ULL; /* 4GB 基地址 */
    layout->text_vmaddr = layout->image_base;

    /* 统计节分布 */
    for (i = 0; i < section_count; i++) {
        if (sections[i].flags & LK_WRITE) {
            has_data = 1;
            data_sect_count++;
        } else {
            text_sect_count++;
        }
    }

    /* 预估 Load Commands 大小 */
    /* 1. LC_SEGMENT_64 (__PAGEZERO) */
    cmd_count++;
    cmds_size += sizeof(struct segment_command_64);

    /* 2. LC_SEGMENT_64 (__TEXT) + sections */
    cmd_count++;
    cmds_size += sizeof(struct segment_command_64) + text_sect_count * sizeof(struct section_64);

    /* 3. LC_SEGMENT_64 (__DATA) + sections (如果有) */
    if (has_data) {
        cmd_count++;
        cmds_size += sizeof(struct segment_command_64) + data_sect_count * sizeof(struct section_64);
    }

    /* 4. LC_SEGMENT_64 (__LINKEDIT) */
    cmd_count++;
    cmds_size += sizeof(struct segment_command_64);

    /* 5. LC_LOAD_DYLINKER */
    cmd_count++;
    cmds_size += sizeof(struct dylinker_command) + 20; /* /usr/lib/dyld 对齐 */

    /* 6. LC_LOAD_DYLIB (/usr/lib/libSystem.B.dylib) */
    cmd_count++;
    cmds_size += sizeof(struct dylib_command) + 32;

    /* 7. LC_MAIN */
    cmd_count++;
    cmds_size += sizeof(struct entry_point_command);

    /* 8. LC_BUILD_VERSION */
    cmd_count++;
    cmds_size += sizeof(struct build_version_command) + sizeof(struct build_tool_version);

    layout->ncmds = cmd_count;
    layout->sizeofcmds = cmds_size;

    /* __TEXT 内部 section 的偏移量：给 Load Commands 预留充足空间 (如 4KB 对齐到 0x1000) */
    text_cursor = 0x1000;

    /* 布局只读/执行节 */
    for (i = 0; i < section_count; i++) {
        if (sections[i].flags & LK_WRITE)
            continue;
        /* 节对齐 */
        unsigned long long align = sections[i].alignment ? sections[i].alignment : 16;
        text_cursor = (text_cursor + align - 1) & ~(align - 1);
        sections[i].file_offset = text_cursor;
        sections[i].address = layout->text_vmaddr + text_cursor;
        text_cursor += sections[i].file_size;
    }

    layout->text_fileoff = 0;
    layout->text_filesize = lk_align_page(text_cursor);
    layout->text_vmsize = layout->text_filesize;

    /* 布局可写数据节 */
    if (has_data) {
        layout->data_vmaddr = layout->text_vmaddr + layout->text_vmsize;
        layout->data_fileoff = layout->text_filesize;
        data_cursor = layout->data_fileoff;

        for (i = 0; i < section_count; i++) {
            if (!(sections[i].flags & LK_WRITE))
                continue;
            unsigned long long align = sections[i].alignment ? sections[i].alignment : 16;
            data_cursor = (data_cursor + align - 1) & ~(align - 1);
            sections[i].file_offset = sections[i].file_size ? data_cursor : 0;
            sections[i].address = layout->data_vmaddr + (data_cursor - layout->data_fileoff);
            data_cursor += sections[i].file_size;
        }
        layout->data_filesize = lk_align_page(data_cursor - layout->data_fileoff);
        layout->data_vmsize = layout->data_filesize;
    }

    /* 布局 __LINKEDIT */
    unsigned long long linkedit_file_start = has_data ? (layout->data_fileoff + layout->data_filesize)
                                                      : layout->text_filesize;
    layout->linkedit_vmaddr = has_data ? (layout->data_vmaddr + layout->data_vmsize)
                                       : (layout->text_vmaddr + layout->text_vmsize);
    layout->linkedit_fileoff = linkedit_file_start;
    layout->linkedit_filesize = MACHO_PAGE_SIZE; /* 预留 16KB 给代码签名与符号 */
    layout->linkedit_vmsize = MACHO_PAGE_SIZE;

    layout->total_file_size = layout->linkedit_fileoff + layout->linkedit_filesize;

    if (entry_addr >= layout->text_vmaddr)
        layout->entry_fileoff = entry_addr - layout->text_vmaddr;
    else
        layout->entry_fileoff = 0;

    return 0;
}

/* 写出 Mach-O 完整可执行文件并进行 codesign 签名 */
int lk_write_macho(const char *path, struct lk_section *sections, unsigned char **payloads,
                  int section_count, struct lk_macho_layout *layout) {
    FILE *f;
    unsigned char *buffer;
    uint32_t cursor = 0;
    int i, text_idx = 0, data_idx = 0;

    buffer = (unsigned char *)calloc(1, (size_t)layout->total_file_size);
    if (!buffer)
        return 1;

    /* 1. Header */
    struct mach_header_64 header;
    memset(&header, 0, sizeof(header));
    header.magic = MH_MAGIC_64;
    header.cputype = CPU_TYPE_ARM64;
    header.cpusubtype = CPU_SUBTYPE_ARM64_ALL;
    header.filetype = MH_EXECUTE;
    header.ncmds = layout->ncmds;
    header.sizeofcmds = layout->sizeofcmds;
    header.flags = MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE;

    memcpy(buffer + cursor, &header, sizeof(header));
    cursor += sizeof(header);

    /* 2. Load Command: __PAGEZERO */
    struct segment_command_64 seg_pagezero;
    memset(&seg_pagezero, 0, sizeof(seg_pagezero));
    seg_pagezero.cmd = LC_SEGMENT_64;
    seg_pagezero.cmdsize = sizeof(seg_pagezero);
    strncpy(seg_pagezero.segname, "__PAGEZERO", sizeof(seg_pagezero.segname));
    seg_pagezero.vmaddr = 0x0;
    seg_pagezero.vmsize = layout->image_base;
    memcpy(buffer + cursor, &seg_pagezero, sizeof(seg_pagezero));
    cursor += sizeof(seg_pagezero);

    /* 3. Load Command: __TEXT */
    int text_sect_count = 0;
    for (i = 0; i < section_count; i++)
        if (!(sections[i].flags & LK_WRITE))
            text_sect_count++;

    struct segment_command_64 seg_text;
    memset(&seg_text, 0, sizeof(seg_text));
    seg_text.cmd = LC_SEGMENT_64;
    seg_text.cmdsize = sizeof(seg_text) + text_sect_count * sizeof(struct section_64);
    strncpy(seg_text.segname, "__TEXT", sizeof(seg_text.segname));
    seg_text.vmaddr = layout->text_vmaddr;
    seg_text.vmsize = layout->text_vmsize;
    seg_text.fileoff = layout->text_fileoff;
    seg_text.filesize = layout->text_filesize;
    seg_text.maxprot = VM_PROT_READ | VM_PROT_EXECUTE;
    seg_text.initprot = VM_PROT_READ | VM_PROT_EXECUTE;
    seg_text.nsects = text_sect_count;
    memcpy(buffer + cursor, &seg_text, sizeof(seg_text));
    cursor += sizeof(seg_text);

    for (i = 0; i < section_count; i++) {
        if (sections[i].flags & LK_WRITE)
            continue;
        struct section_64 sec;
        memset(&sec, 0, sizeof(sec));
        strncpy(sec.sectname, text_idx == 0 ? "__text" : "__text_extra", sizeof(sec.sectname));
        text_idx++;
        strncpy(sec.segname, "__TEXT", sizeof(sec.segname));
        sec.addr = sections[i].address;
        sec.size = sections[i].file_size;
        sec.offset = (uint32_t)sections[i].file_offset;
        sec.align = 2;
        sec.flags = S_REGULAR | S_ATTR_SOME_INSTRUCTIONS | S_ATTR_PURE_INSTRUCTIONS;
        memcpy(buffer + cursor, &sec, sizeof(sec));
        cursor += sizeof(sec);
    }

    /* 4. Load Command: __DATA (如果有) */
    int data_sect_count = 0;
    for (i = 0; i < section_count; i++)
        if (sections[i].flags & LK_WRITE)
            data_sect_count++;

    if (data_sect_count > 0) {
        struct segment_command_64 seg_data;
        memset(&seg_data, 0, sizeof(seg_data));
        seg_data.cmd = LC_SEGMENT_64;
        seg_data.cmdsize = sizeof(seg_data) + data_sect_count * sizeof(struct section_64);
        strncpy(seg_data.segname, "__DATA", sizeof(seg_data.segname));
        seg_data.vmaddr = layout->data_vmaddr;
        seg_data.vmsize = layout->data_vmsize;
        seg_data.fileoff = layout->data_fileoff;
        seg_data.filesize = layout->data_filesize;
        seg_data.maxprot = VM_PROT_READ | VM_PROT_WRITE;
        seg_data.initprot = VM_PROT_READ | VM_PROT_WRITE;
        seg_data.nsects = data_sect_count;
        memcpy(buffer + cursor, &seg_data, sizeof(seg_data));
        cursor += sizeof(seg_data);

        for (i = 0; i < section_count; i++) {
            if (!(sections[i].flags & LK_WRITE))
                continue;
            struct section_64 sec;
            memset(&sec, 0, sizeof(sec));
            snprintf(sec.sectname, sizeof(sec.sectname), "__data_%d", data_idx++);
            strncpy(sec.segname, "__DATA", sizeof(sec.segname));
            sec.addr = sections[i].address;
            sec.size = sections[i].memory_size;
            sec.offset = (uint32_t)sections[i].file_offset;
            sec.align = 3;
            sec.flags = sections[i].file_size ? S_REGULAR : S_ZEROFILL;
            memcpy(buffer + cursor, &sec, sizeof(sec));
            cursor += sizeof(sec);
        }
    }

    /* 5. Load Command: __LINKEDIT */
    struct segment_command_64 seg_linkedit;
    memset(&seg_linkedit, 0, sizeof(seg_linkedit));
    seg_linkedit.cmd = LC_SEGMENT_64;
    seg_linkedit.cmdsize = sizeof(seg_linkedit);
    strncpy(seg_linkedit.segname, "__LINKEDIT", sizeof(seg_linkedit.segname));
    seg_linkedit.vmaddr = layout->linkedit_vmaddr;
    seg_linkedit.vmsize = layout->linkedit_vmsize;
    seg_linkedit.fileoff = layout->linkedit_fileoff;
    seg_linkedit.filesize = layout->linkedit_filesize;
    seg_linkedit.maxprot = VM_PROT_READ;
    seg_linkedit.initprot = VM_PROT_READ;
    memcpy(buffer + cursor, &seg_linkedit, sizeof(seg_linkedit));
    cursor += sizeof(seg_linkedit);

    /* 6. LC_LOAD_DYLINKER */
    struct {
        struct dylinker_command cmd;
        char name[20];
    } __attribute__((packed)) dylinker;
    memset(&dylinker, 0, sizeof(dylinker));
    dylinker.cmd.cmd = LC_LOAD_DYLINKER;
    dylinker.cmd.cmdsize = sizeof(dylinker);
    dylinker.cmd.name.offset = sizeof(struct dylinker_command);
    strcpy(dylinker.name, "/usr/lib/dyld");
    memcpy(buffer + cursor, &dylinker, sizeof(dylinker));
    cursor += sizeof(dylinker);

    /* 7. LC_LOAD_DYLIB (/usr/lib/libSystem.B.dylib) */
    struct {
        struct dylib_command cmd;
        char name[32];
    } __attribute__((packed)) libsys;
    memset(&libsys, 0, sizeof(libsys));
    libsys.cmd.cmd = LC_LOAD_DYLIB;
    libsys.cmd.cmdsize = sizeof(libsys);
    libsys.cmd.dylib.name.offset = sizeof(struct dylib_command);
    libsys.cmd.dylib.timestamp = 2;
    libsys.cmd.dylib.current_version = 0x05000000;
    libsys.cmd.dylib.compatibility_version = 0x01000000;
    strcpy(libsys.name, "/usr/lib/libSystem.B.dylib");
    memcpy(buffer + cursor, &libsys, sizeof(libsys));
    cursor += sizeof(libsys);

    /* 8. LC_MAIN */
    struct entry_point_command entry_cmd;
    memset(&entry_cmd, 0, sizeof(entry_cmd));
    entry_cmd.cmd = LC_MAIN;
    entry_cmd.cmdsize = sizeof(entry_cmd);
    entry_cmd.entryoff = layout->entry_fileoff;
    entry_cmd.stacksize = 0;
    memcpy(buffer + cursor, &entry_cmd, sizeof(entry_cmd));
    cursor += sizeof(entry_cmd);

    /* 9. LC_BUILD_VERSION */
    struct {
        struct build_version_command cmd;
        struct build_tool_version tool;
    } __attribute__((packed)) build_ver;
    memset(&build_ver, 0, sizeof(build_ver));
    build_ver.cmd.cmd = LC_BUILD_VERSION;
    build_ver.cmd.cmdsize = sizeof(build_ver);
    build_ver.cmd.platform = PLATFORM_MACOS;
    build_ver.cmd.minos = (14 << 16);
    build_ver.cmd.sdk = (14 << 16);
    build_ver.cmd.ntools = 1;
    build_ver.tool.tool = TOOL_LD;
    build_ver.tool.version = 0;
    memcpy(buffer + cursor, &build_ver, sizeof(build_ver));
    cursor += sizeof(build_ver);

    /* 写入各个节的实际 payload 数据 */
    for (i = 0; i < section_count; i++) {
        if (sections[i].file_size && payloads[i] && sections[i].file_offset) {
            memcpy(buffer + sections[i].file_offset, payloads[i], (size_t)sections[i].file_size);
        }
    }

    f = fopen(path, "wb");
    if (!f) {
        free(buffer);
        return 1;
    }
    fwrite(buffer, 1, (size_t)layout->total_file_size, f);
    fclose(f);
    free(buffer);

    chmod(path, 0755);

    /* 执行 codesign 签名 */
    char sign_cmd[512];
    snprintf(sign_cmd, sizeof(sign_cmd), "codesign -s - --force \"%s\" >/dev/null 2>&1", path);
    int ret = system(sign_cmd);
    if (ret != 0) {
        /* 如果签名命令返回非零，尝试不加重定向输出以排查 */
        fprintf(stderr, "warning: codesign returned %d\n", ret);
    }

    return 0;
}

#endif
