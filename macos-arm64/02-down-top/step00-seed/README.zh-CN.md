# 阶段 0：纯十六进制 ARM64 机器码种子 (Step 00 - Seed)

## 概述

自举编译的核心哲学在于**信任根的极小化与透明化**。本项目绝不依赖宿主预编译好的庞大编译器二进制文件，而是从一份完全由十六进制字符构成的原始机器码文本（`seed-arm64.hex`）破土萌芽。

在 Apple Silicon (macOS Darwin ARM64) 平台上，这份种子程序直接实现了一个精简的 Mach-O 64 位可执行文件，用于从标准输入读取字符并回写至标准输出。

---

## 文件结构

- `src/seed-arm64.hex`: 纯十六进制机器码种子文件（经过 16KB 页边界对齐与自包含 Header）；
- `src/seed_arm64.s`: 汇编助记符源码，详细注释了 Mach-O Header、Load Commands 与系统调用汇编逻辑；
- `decode_seed.sh`: 纯 POSIX Shell 解码脚本，将十六进制文本转换为原生二进制并附加 ad-hoc 签名；
- `run.sh`: 一键运行与 I/O 管道回显验证脚本。

---

## 核心设计原理

1. **Mach-O 64 位头布局**：
   - `MH_MAGIC_64` (0xfeedfacf) 标识 64 位 Mach-O；
   - `CPU_TYPE_ARM64` (0x0100000c) 标识 AArch64 体系；
   - 加载命令包含 `LC_SEGMENT_64` (`__PAGEZERO` 与 `__TEXT`) 以及 `LC_UNIXTHREAD` 指定入口寄存器 `pc`。
2. **16KB 页对齐限制**：
   - Apple Silicon 架构强制要求可执行文件的物理段对齐为 16KB (`0x4000`)；
   - 机器码有效逻辑仅 140 字节，其余部分按规范以 0 填充对齐。
3. **Darwin 原生系统调用**：
   - `x16 = 3`: `sys_read(0, buf, 1024)` 从 stdin 读取；
   - `x16 = 4`: `sys_write(1, buf, len)` 写往 stdout；
   - `x16 = 1`: `sys_exit(0)` 退出程序；
   - 通过 `svc #0x80` 陷入内核。

---

## 运行验证

```bash
chmod +x run.sh decode_seed.sh
./run.sh
```
