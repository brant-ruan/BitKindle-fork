//
// seed_arm64.s
// BitKindle macOS ARM64 机器码种子
// 职责：从标准输入 stdin (0) 读取十六进制文本流，过滤注释与空白，
// 将连续的 2 个十六进制字符解析为 1 个二进制字节，写出到标准输出 stdout (1)。
//
.global _main
.p2align 2

_main:
    stp x29, x30, [sp, #-32]!
    mov x29, sp

    // 局部栈空间:
    // [sp, #16] -> 1 字节读缓冲区 in_buf
    // [sp, #17] -> 1 字节写缓冲区 out_buf
    // w19 -> 高 4 位暂存器 (-1 表示当前等待高 4 位)
    mov w19, #-1

.Lread_loop:
    // sys_read(0, sp+16, 1)
    mov x0, #0
    add x1, sp, #16
    mov x2, #1
    movz x16, #3
    movk x16, #0x200, lsl #16
    svc #0x80

    // 如果读取返回 <= 0，则退出循环
    cmp x0, #0
    b.le .Ldone

    // 读取到的字符
    ldrb w20, [sp, #16]

    // 跳过空白字符 (' ', '\t', '\r', '\n')
    cmp w20, #32
    b.eq .Lread_loop
    cmp w20, #10
    b.eq .Lread_loop
    cmp w20, #13
    b.eq .Lread_loop
    cmp w20, #9
    b.eq .Lread_loop

    // 如果是注释 '#'，则一直读到换行符 '\n'
    cmp w20, #'#'
    b.ne .Lparse_hex

.Lskip_comment:
    mov x0, #0
    add x1, sp, #16
    mov x2, #1
    movz x16, #3
    movk x16, #0x200, lsl #16
    svc #0x80
    cmp x0, #0
    b.le .Ldone
    ldrb w20, [sp, #16]
    cmp w20, #10
    b.ne .Lskip_comment
    b .Lread_loop

.Lparse_hex:
    // 将 ASCII 字符 w20 转为数值 (0-15) 放入 w21
    // '0'-'9'
    sub w21, w20, #'0'
    cmp w20, #'0'
    b.lt .Lread_loop
    cmp w20, #'9'
    b.le .Lhave_nibble

    // 'a'-'f'
    sub w21, w20, #'a'
    add w21, w21, #10
    cmp w20, #'a'
    b.lt .Lcheck_upper
    cmp w20, #'f'
    b.le .Lhave_nibble

.Lcheck_upper:
    // 'A'-'F'
    sub w21, w20, #'A'
    add w21, w21, #10
    cmp w20, #'A'
    b.lt .Lread_loop
    cmp w20, #'F'
    b.gt .Lread_loop

.Lhave_nibble:
    // 如果没有暂存的高 4 位，则存入 w19 并继续读下一个半字节
    cmp w19, #0
    b.lt .Lsave_high

    // 组合高 4 位 (w19) 和低 4 位 (w21)
    lsl w19, w19, #4
    orr w19, w19, w21
    strb w19, [sp, #17]

    // sys_write(1, sp+17, 1)
    mov x0, #1
    add x1, sp, #17
    mov x2, #1
    movz x16, #4
    movk x16, #0x200, lsl #16
    svc #0x80

    // 重置高 4 位暂存器
    mov w19, #-1
    b .Lread_loop

.Lsave_high:
    mov w19, w21
    b .Lread_loop

.Ldone:
    // sys_exit(0)
    mov x0, #0
    mov sp, x29
    ldp x29, x30, [sp], #32
    ret
