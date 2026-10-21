/*
 * c1_arm64.c
 * BitKindle macOS ARM64 原生 C 语言编译器
 * 
 * 读取 C 源码，执行词法、语法解析与语义分析，发射原生 ARM64 机器码，
 * 并直接生成符合 BitKindle 规范的 .c1o (0xAA64) 目标文件。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "../linker/writer.c"

#define MAX_SOURCE 1048576
#define MAX_TOKENS 262144
#define MAX_SYMBOLS 4096
#define MAX_FIXUPS 16384
#define MAX_CODE 1048576

/* Token 种类 */
enum {
    TK_EOF = 0,
    TK_INT,
    TK_CHAR,
    TK_VOID,
    TK_RETURN,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
    TK_IDENT,
    TK_NUM,
    TK_STR,
    TK_EQ,       /* == */
    TK_NE,       /* != */
    TK_LE,       /* <= */
    TK_GE,       /* >= */
    TK_AND,      /* && */
    TK_OR,       /* || */
    TK_SHL,      /* << */
    TK_SHR       /* >> */
};

/* 符号记录 */
struct Symbol {
    char name[64];
    int is_func;
    int is_global;
    int stack_offset; /* 局部变量在栈上的偏移量 (相对于 x29) */
    int value;        /* 函数在代码缓冲区中的偏移量 */
    int size;
    int param_count;
};

/* 全局编译器状态 */
static char source[MAX_SOURCE];
static int source_len = 0;
static int src_pos = 0;
static int line_num = 1;

static int token = 0;
static int token_val = 0;
static char token_str[256];

static struct Symbol symbols[MAX_SYMBOLS];
static int symbol_count = 0;

static uint32_t code_buf[MAX_CODE / 4];
static int code_pos = 0; /* 指令索引 (字节数 = code_pos * 4) */

/* 重定位记录 (由编译器记录并在生成 .c1o 时写出) */
struct Fixup {
    int symbol_idx;
    int code_offset; /* 字节偏移 */
    int kind;
};
static struct Fixup fixups[MAX_FIXUPS];
static int fixup_count = 0;

static int current_stack_size = 0;
static int local_sym_start = 0;

/* ==================== 错误处理 ==================== */
static void error(const char *msg) {
    fprintf(stderr, "c1-arm64: error at line %d: %s (near '%s')\n", line_num, msg, token_str);
    exit(1);
}

/* ==================== 词法分析 (Lexer) ==================== */
static void next_char(void) {
    src_pos++;
}

static void next_token(void) {
    while (src_pos < source_len) {
        char c = source[src_pos];

        /* 跳过空白与换行 */
        if (c == ' ' || c == '\t' || c == '\r') {
            next_char();
            continue;
        }
        if (c == '\n') {
            line_num++;
            next_char();
            continue;
        }

        /* 跳过单行注释 // */
        if (c == '/' && source[src_pos + 1] == '/') {
            src_pos += 2;
            while (src_pos < source_len && source[src_pos] != '\n')
                src_pos++;
            continue;
        }

        /* 跳过多行块注释 */
        if (c == '/' && source[src_pos + 1] == '*') {
            src_pos += 2;
            while (src_pos + 1 < source_len && !(source[src_pos] == '*' && source[src_pos + 1] == '/')) {
                if (source[src_pos] == '\n') line_num++;
                src_pos++;
            }
            src_pos += 2;
            continue;
        }

        /* 标识符与关键字 */
        if (isalpha(c) || c == '_') {
            int len = 0;
            while ((isalnum(source[src_pos]) || source[src_pos] == '_') && len < 63) {
                token_str[len++] = source[src_pos++];
            }
            token_str[len] = '\0';

            if (strcmp(token_str, "int") == 0) { token = TK_INT; return; }
            if (strcmp(token_str, "char") == 0) { token = TK_CHAR; return; }
            if (strcmp(token_str, "void") == 0) { token = TK_VOID; return; }
            if (strcmp(token_str, "return") == 0) { token = TK_RETURN; return; }
            if (strcmp(token_str, "if") == 0) { token = TK_IF; return; }
            if (strcmp(token_str, "else") == 0) { token = TK_ELSE; return; }
            if (strcmp(token_str, "while") == 0) { token = TK_WHILE; return; }
            if (strcmp(token_str, "for") == 0) { token = TK_FOR; return; }

            token = TK_IDENT;
            return;
        }

        /* 十进制与十六进制整数 */
        if (isdigit(c)) {
            int val = 0;
            if (c == '0' && (source[src_pos + 1] == 'x' || source[src_pos + 1] == 'X')) {
                src_pos += 2;
                while (isxdigit(source[src_pos])) {
                    char h = source[src_pos++];
                    val = val * 16 + (isdigit(h) ? h - '0' : tolower(h) - 'a' + 10);
                }
            } else {
                while (isdigit(source[src_pos])) {
                    val = val * 10 + (source[src_pos++] - '0');
                }
            }
            token = TK_NUM;
            token_val = val;
            snprintf(token_str, sizeof(token_str), "%d", val);
            return;
        }

        /* 双字符运算符 */
        if (c == '=' && source[src_pos + 1] == '=') { src_pos += 2; token = TK_EQ; strcpy(token_str, "=="); return; }
        if (c == '!' && source[src_pos + 1] == '=') { src_pos += 2; token = TK_NE; strcpy(token_str, "!="); return; }
        if (c == '<' && source[src_pos + 1] == '=') { src_pos += 2; token = TK_LE; strcpy(token_str, "<="); return; }
        if (c == '>' && source[src_pos + 1] == '=') { src_pos += 2; token = TK_GE; strcpy(token_str, ">="); return; }
        if (c == '&' && source[src_pos + 1] == '&') { src_pos += 2; token = TK_AND; strcpy(token_str, "&&"); return; }
        if (c == '|' && source[src_pos + 1] == '|') { src_pos += 2; token = TK_OR; strcpy(token_str, "||"); return; }
        if (c == '<' && source[src_pos + 1] == '<') { src_pos += 2; token = TK_SHL; strcpy(token_str, "<<"); return; }
        if (c == '>' && source[src_pos + 1] == '>') { src_pos += 2; token = TK_SHR; strcpy(token_str, ">>"); return; }

        /* 单字符操作符/标点 */
        token = c;
        token_str[0] = c;
        token_str[1] = '\0';
        src_pos++;
        return;
    }
    token = TK_EOF;
    strcpy(token_str, "EOF");
}

static void match(int expected) {
    if (token == expected) {
        next_token();
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected token '%c' (%d)", expected < 128 ? expected : '?', expected);
        error(buf);
    }
}

/* ==================== 符号管理 ==================== */
static struct Symbol *find_symbol(const char *name) {
    int i;
    /* 局部符号优先于全局符号 */
    for (i = symbol_count - 1; i >= 0; i--) {
        if (strcmp(symbols[i].name, name) == 0)
            return &symbols[i];
    }
    return NULL;
}

static int add_symbol(const char *name, int is_func, int is_global) {
    if (symbol_count >= MAX_SYMBOLS)
        error("too many symbols");
    struct Symbol *s = &symbols[symbol_count];
    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->is_func = is_func;
    s->is_global = is_global;
    return symbol_count++;
}

/* ==================== ARM64 代码发射器 ==================== */
static void emit(uint32_t insn) {
    if (code_pos >= MAX_CODE / 4)
        error("code buffer overflow");
    code_buf[code_pos++] = insn;
}

/* 加载 32 位立即数到寄存器 Rd: movz + movk */
static void emit_mov_imm(int rd, int val) {
    uint32_t uval = (uint32_t)val;
    uint32_t low16 = uval & 0xFFFF;
    uint32_t high16 = (uval >> 16) & 0xFFFF;

    /* movz Rd, #low16, lsl #0 */
    emit(0x52800000 | (rd & 0x1F) | (low16 << 5));
    if (high16) {
        /* movk Rd, #high16, lsl #16 */
        emit(0x72a00000 | (rd & 0x1F) | (high16 << 5));
    }
}

/* 压栈 X0 到栈底 (保持 16 字节对齐): str x0, [sp, #-16]! */
static void emit_push_x0(void) {
    emit(0xf81f0fe0);
}

/* 弹栈到 X1: ldr x1, [sp], #16 */
static void emit_pop_x1(void) {
    emit(0xf84107e1);
}

/* 局部变量栈存储: str w0, [x29, #-offset] */
static void emit_store_local(int offset) {
    /* ARM64 STR (immediate, unscaled offset): stur w0, [x29, #-offset] */
    int simm9 = (-offset) & 0x1FF;
    emit(0xb8000000 | (simm9 << 12) | (29 << 5) | 0);
}

/* 局部变量栈读取: ldr w0, [x29, #-offset] */
static void emit_load_local(int offset) {
    int simm9 = (-offset) & 0x1FF;
    emit(0xb8400000 | (simm9 << 12) | (29 << 5) | 0);
}

/* ==================== 语法分析与代码生成 ==================== */
static void parse_expression(void);

/* 解析基本因子 (Primary Expression) */
static void parse_primary(void) {
    if (token == TK_NUM) {
        emit_mov_imm(0, token_val);
        next_token();
    } else if (token == TK_IDENT) {
        char name[64];
        strcpy(name, token_str);
        next_token();

        /* 函数调用 */
        if (token == '(') {
            match('(');
            int arg_count = 0;
            if (token != ')') {
                while (1) {
                    parse_expression();
                    emit_push_x0(); /* 把参数暂存进栈 */
                    arg_count++;
                    if (token == ',') {
                        match(',');
                    } else {
                        break;
                    }
                }
            }
            match(')');

            /* 将参数从栈中弹出到对应的 X0 ~ X7 寄存器 (倒序弹出) */
            int i;
            for (i = arg_count - 1; i >= 0; i--) {
                if (i < 8) {
                    /* ldr xN, [sp], #16 */
                    emit(0xf84107e0 | i);
                } else {
                    /* 超出 8 个的参数通常直接保留在栈上，这里简易起见丢弃偏移 */
                    emit(0xf84107e0 | 16);
                }
            }

            /* 记录函数调用的重定位条目并生成 bl 指令 */
            struct Symbol *func_sym = find_symbol(name);
            int sym_id;
            if (!func_sym) {
                sym_id = add_symbol(name, 1, 1);
            } else {
                sym_id = (int)(func_sym - symbols);
            }

            fixups[fixup_count].symbol_idx = sym_id;
            fixups[fixup_count].code_offset = code_pos * 4;
            fixups[fixup_count].kind = LK_ARM64_BRANCH26;
            fixup_count++;

            /* bl 0 (占位，待链接时由 c1link-macho 重定位回填) */
            emit(0x94000000);
        } else {
            /* 局部变量读取 */
            struct Symbol *sym = find_symbol(name);
            if (!sym) {
                char buf[128];
                snprintf(buf, sizeof(buf), "undefined variable '%s'", name);
                error(buf);
            }
            emit_load_local(sym->stack_offset);
        }
    } else if (token == '(') {
        match('(');
        parse_expression();
        match(')');
    } else {
        error("unexpected token in primary expression");
    }
}

/* 一元表达式 */
static void parse_unary(void) {
    if (token == '-') {
        next_token();
        parse_primary();
        /* neg w0, w0 -> sub w0, wzr, w0 */
        emit(0x4b0003e0);
    } else if (token == '!') {
        next_token();
        parse_primary();
        /* cmp w0, #0; cset w0, eq */
        emit(0x7100001f);
        emit(0x1a9f17e0);
    } else {
        parse_primary();
    }
}

/* 乘除模 */
static void parse_multiplicative(void) {
    parse_unary();
    while (token == '*' || token == '/' || token == '%') {
        int op = token;
        next_token();
        emit_push_x0();
        parse_unary();
        emit_pop_x1();
        if (op == '*') {
            /* mul w0, w1, w0 */
            emit(0x1b007c20);
        } else if (op == '/') {
            /* sdiv w0, w1, w0 */
            emit(0x1ac00c20);
        } else if (op == '%') {
            /* sdiv w2, w1, w0; msub w0, w2, w0, w1 */
            emit(0x1ac00c22);
            emit(0x1b008040);
        }
    }
}

/* 加减 */
static void parse_additive(void) {
    parse_multiplicative();
    while (token == '+' || token == '-') {
        int op = token;
        next_token();
        emit_push_x0();
        parse_multiplicative();
        emit_pop_x1();
        if (op == '+') {
            /* add w0, w1, w0 */
            emit(0x0b000020);
        } else {
            /* sub w0, w1, w0 */
            emit(0x4b000020);
        }
    }
}

/* 关系比较 (<, <=, >, >=) */
static void parse_relational(void) {
    parse_additive();
    while (token == '<' || token == '>' || token == TK_LE || token == TK_GE) {
        int op = token;
        next_token();
        emit_push_x0();
        parse_additive();
        emit_pop_x1();
        /* cmp w1, w0 (subs wzr, w1, w0) */
        emit(0x6b00003f);
        if (op == '<') {
            /* cset w0, lt */
            emit(0x1a9fa7e0);
        } else if (op == '>') {
            /* cset w0, gt */
            emit(0x1a9fd7e0);
        } else if (op == TK_LE) {
            /* cset w0, le */
            emit(0x1a9fc7e0);
        } else if (op == TK_GE) {
            /* cset w0, ge */
            emit(0x1a9fb7e0);
        }
    }
}

/* 相等比较 (==, !=) */
static void parse_equality(void) {
    parse_relational();
    while (token == TK_EQ || token == TK_NE) {
        int op = token;
        next_token();
        emit_push_x0();
        parse_relational();
        emit_pop_x1();
        /* cmp w1, w0 */
        emit(0x6b00003f);
        if (op == TK_EQ) {
            /* cset w0, eq (cond 0000 -> 0) */
            emit(0x1a9f17e0);
        } else {
            /* cset w0, ne (cond 0001 -> 1) */
            emit(0x1a9f07e0);
        }
    }
}

static int is_assignment_next(void) {
    int p = src_pos;
    while (p < source_len && (source[p] == ' ' || source[p] == '\t' || source[p] == '\r' || source[p] == '\n'))
        p++;
    return (p < source_len && source[p] == '=' && (p + 1 >= source_len || source[p + 1] != '='));
}

/* 赋值与完整表达式 */
static void parse_expression(void) {
    /* 检查赋值: ident = expr */
    if (token == TK_IDENT && is_assignment_next()) {
        char name[64];
        strcpy(name, token_str);
        next_token();
        match('=');
        parse_expression();
        struct Symbol *sym = find_symbol(name);
        if (!sym) {
            char buf[128];
            snprintf(buf, sizeof(buf), "undefined variable '%s'", name);
            error(buf);
        }
        emit_store_local(sym->stack_offset);
        return;
    }
    parse_equality();
}

/* 语句解析 */
static void parse_statement(void);

static void parse_block(void) {
    match('{');
    while (token != '}' && token != TK_EOF) {
        parse_statement();
    }
    match('}');
}

static void parse_statement(void) {
    if (token == '{') {
        parse_block();
    } else if (token == TK_INT || token == TK_CHAR) {
        /* 变量声明: int a = 10; */
        next_token();
        while (1) {
            if (token != TK_IDENT)
                error("expected identifier in declaration");
            char name[64];
            strcpy(name, token_str);
            next_token();

            /* 分配局部变量栈偏移 (每次对齐 16 字节) */
            current_stack_size += 16;
            int sym_id = add_symbol(name, 0, 0);
            symbols[sym_id].stack_offset = current_stack_size;

            if (token == '=') {
                match('=');
                parse_expression();
                emit_store_local(symbols[sym_id].stack_offset);
            }
            if (token == ',') {
                match(',');
            } else {
                break;
            }
        }
        match(';');
    } else if (token == TK_RETURN) {
        next_token();
        if (token != ';') {
            parse_expression();
        }
        match(';');
        /* 函数返回尾声 */
        /* mov sp, x29 */
        emit(0x910003bf);
        /* ldp x29, x30, [sp], #16 */
        emit(0xa8c17bfd);
        /* ret */
        emit(0xd65f03c0);
    } else if (token == TK_IF) {
        next_token();
        match('(');
        parse_expression();
        match(')');

        /* cbz w0, <else_or_end> */
        int branch_jump = code_pos;
        emit(0x34000000); /* 预留占位 */

        parse_statement();

        if (token == TK_ELSE) {
            next_token();
            /* b <end> */
            int end_jump = code_pos;
            emit(0x14000000);

            /* 回填 cbz 跳转到 else 起始位置 */
            int else_offset = (code_pos - branch_jump);
            code_buf[branch_jump] = 0x34000000 | ((else_offset & 0x7FFFF) << 5);

            parse_statement();

            /* 回填 b 跳转到 if-else 结束位置 */
            int finish_offset = (code_pos - end_jump);
            code_buf[end_jump] = 0x14000000 | (finish_offset & 0x03FFFFFF);
        } else {
            /* 回填 cbz 到 if 结束位置 */
            int finish_offset = (code_pos - branch_jump);
            code_buf[branch_jump] = 0x34000000 | ((finish_offset & 0x7FFFF) << 5);
        }
    } else if (token == TK_WHILE) {
        next_token();
        match('(');
        int loop_start = code_pos;
        parse_expression();
        match(')');

        /* cbz w0, <loop_end> */
        int exit_jump = code_pos;
        emit(0x34000000);

        parse_statement();

        /* b <loop_start> */
        int back_offset = (loop_start - code_pos);
        emit(0x14000000 | (back_offset & 0x03FFFFFF));

        /* 回填 exit_jump */
        int end_offset = (code_pos - exit_jump);
        code_buf[exit_jump] = 0x34000000 | ((end_offset & 0x7FFFF) << 5);
    } else {
        parse_expression();
        match(';');
    }
}

/* 解析顶层函数定义 */
static void parse_function(void) {
    /* 返回类型 */
    next_token(); /* int / void */
    if (token != TK_IDENT)
        error("expected function name");

    char func_name[64];
    strcpy(func_name, token_str);
    next_token();

    int sym_id = add_symbol(func_name, 1, 1);
    symbols[sym_id].value = code_pos * 4; /* 函数入口相对代码段起始的字节偏移 */

    match('(');
    current_stack_size = 0;
    int param_count = 0;

    /* 记录局部符号起点，方便在函数结束时隔离局部变量 */
    local_sym_start = symbol_count;

    if (token != ')' && token != TK_VOID) {
        while (1) {
            next_token(); /* int */
            if (token != TK_IDENT)
                error("expected parameter name");
            char param_name[64];
            strcpy(param_name, token_str);
            next_token();

            current_stack_size += 16;
            int p_sym = add_symbol(param_name, 0, 0);
            symbols[p_sym].stack_offset = current_stack_size;
            param_count++;

            if (token == ',') {
                match(',');
            } else {
                break;
            }
        }
    } else if (token == TK_VOID) {
        next_token();
    }
    match(')');
    symbols[sym_id].param_count = param_count;

    /* 函数前言 (Prologue) */
    /* stp x29, x30, [sp, #-16]! */
    emit(0xa9bf7bfd);
    /* mov x29, sp */
    emit(0x910003fd);

    /* 预留栈空间: sub sp, sp, #frame_size (稍后回填) */
    int frame_sub_pos = code_pos;
    emit(0xd10003ff); /* sub sp, sp, #0 */

    /* 将传入的实参寄存器 (X0 ~ X7) 保存到对应的栈槽中 */
    int p;
    for (p = 0; p < param_count; p++) {
        if (p < 8) {
            /* str wP, [x29, #-offset] */
            int offset = (p + 1) * 16;
            int simm9 = (-offset) & 0x1FF;
            emit(0xb8000000 | (simm9 << 12) | (29 << 5) | p);
        }
    }

    /* 解析函数体 */
    parse_block();

    /* 函数默认返回指令 (防止无显式 return 时落空) */
    /* mov sp, x29 */
    emit(0x910003bf);
    /* ldp x29, x30, [sp], #16 */
    emit(0xa8c17bfd);
    /* ret */
    emit(0xd65f03c0);

    /* 回填栈帧大小: sub sp, sp, #current_stack_size (向上 16 字节对齐) */
    int aligned_frame = (current_stack_size + 15) & ~15;
    if (aligned_frame > 0) {
        /* sub sp, sp, #aligned_frame: 0xd10003ff | (imm12 << 10) */
        code_buf[frame_sub_pos] = 0xd10003ff | ((aligned_frame & 0xFFF) << 10);
    }
    symbols[sym_id].size = code_pos * 4 - symbols[sym_id].value;
}

/* 顶层解析循环 */
static void parse_translation_unit(void) {
    while (token != TK_EOF) {
        if (token == TK_INT || token == TK_CHAR || token == TK_VOID) {
            parse_function();
        } else {
            error("unrecognized top-level declaration");
        }
    }
}

/* ==================== 生成 .c1o 目标文件 ==================== */
static int emit_c1o_object(const char *outpath) {
    /* 1. 构造代码节 */
    struct lk_section sec;
    memset(&sec, 0, sizeof(sec));
    sec.memory_size = code_pos * 4;
    sec.file_size = code_pos * 4;
    sec.alignment = 16;
    sec.file_alignment = 16;
    sec.flags = LK_READ | LK_EXECUTE;

    unsigned char *payload = (unsigned char *)code_buf;

    /* 2. 导出符号表 (筛选出全局符号) */
    struct lk_symbol export_syms[MAX_SYMBOLS];
    int export_count = 0;

    int i;
    for (i = 0; i < symbol_count; i++) {
        if (symbols[i].is_global) {
            memset(&export_syms[export_count], 0, sizeof(struct lk_symbol));
            export_syms[export_count].name = symbols[i].name;
            export_syms[export_count].binding = LK_GLOBAL;
            export_syms[export_count].section = 0; /* 属于代码节 */
            export_syms[export_count].value = symbols[i].value;
            export_syms[export_count].size = symbols[i].size;
            export_syms[export_count].alignment = 4;
            export_count++;
        }
    }

    /* 3. 构造重定位记录 */
    struct lk_fixup lk_fixups[MAX_FIXUPS];
    for (i = 0; i < fixup_count; i++) {
        lk_fixups[i].section = 0;
        /* 查找对应的导出符号索引 */
        int target_sym_idx = 0;
        int j;
        for (j = 0; j < export_count; j++) {
            if (strcmp(export_syms[j].name, symbols[fixups[i].symbol_idx].name) == 0) {
                target_sym_idx = j;
                break;
            }
        }
        lk_fixups[i].symbol = target_sym_idx;
        lk_fixups[i].kind = fixups[i].kind;
        lk_fixups[i].offset = fixups[i].code_offset;
        lk_fixups[i].addend = 0;
    }

    /* 4. 写出 .c1o 二进制 */
    size_t out_capacity = sizeof(code_buf) + 65536;
    unsigned char *out_buf = (unsigned char *)malloc(out_capacity);
    if (!out_buf) return 1;

    unsigned long long written = 0;
    int ret = lk_object_write(&sec, &payload, 1, export_syms, export_count,
                              lk_fixups, fixup_count, out_buf, out_capacity, &written);
    if (ret != 0) {
        fprintf(stderr, "c1-arm64: lk_object_write failed (%d)\n", ret);
        free(out_buf);
        return 1;
    }

    FILE *f = fopen(outpath, "wb");
    if (!f) {
        perror("fopen outpath failed");
        free(out_buf);
        return 1;
    }
    fwrite(out_buf, 1, (size_t)written, f);
    fclose(f);
    free(out_buf);

    printf("[SUCCESS] c1-arm64 compiled '%s' -> '%s' (%llu bytes, %d fixups)\n",
           token_str, outpath, written, fixup_count);
    return 0;
}

/* ==================== main 入口 ==================== */
int main(int argc, char **argv) {
    const char *inpath = NULL;
    const char *outpath = "a.c1o";

    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outpath = argv[++i];
        } else if (!inpath) {
            inpath = argv[i];
        }
    }

    if (!inpath) {
        fprintf(stderr, "usage: c1-arm64 [-o output.c1o] input.c\n");
        return 1;
    }

    FILE *f = fopen(inpath, "rb");
    if (!f) {
        fprintf(stderr, "c1-arm64: cannot open input file '%s'\n", inpath);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    source_len = (int)ftell(f);
    fseek(f, 0, SEEK_SET);

    if (source_len >= MAX_SOURCE) {
        fclose(f);
        fprintf(stderr, "c1-arm64: input file too large\n");
        return 1;
    }

    fread(source, 1, source_len, f);
    source[source_len] = '\0';
    fclose(f);

    /* 启动词法与语法分析 */
    src_pos = 0;
    line_num = 1;
    next_token();

    parse_translation_unit();

    /* 输出 .c1o 目标文件 */
    if (emit_c1o_object(outpath) != 0) {
        return 1;
    }

    return 0;
}
