# 阶段 1：独立 64 位 Mach-O 链接器与代码签名 (Step 01 - Linker)

## 概述

在从零构建自举编译器之前，**必须先拥有一个能够将目标文件组装为可执行程序的链接器**。

原版 BitKindle 的链接器是针对 Windows PE32+ (x64) 格式设计的。在本阶段中，我们彻底重写了链接后端，构建了面向 Apple Silicon 的纯原生 64 位 Mach-O 链接器 `c1link-macho`。

---

## 文件结构

- `src/`: 链接器核心源码实现：
  - `link.c`: 链接器命令行主入口、启动桩（Startup Stub）生成与两阶段链接调度；
  - `macho.c`: 64 位 Mach-O 段表、节表与 Load Commands 发射；
  - `relocate.c`: ARM64 指令重定位修补（`PAGE21`、`PAGEOFF12`、`BRANCH26`）；
  - `merge.c`: 多目标文件（`.c1o`）节区合并算法；
  - `symbols.c`: 全局与局部符号导出、解析与冲突绑定；
  - `sections.c`: 内存布局与 16KB 页边界对齐计算；
  - `fixups.c`: 符号引用修复；
  - `object.c` / `writer.c` / `files.c`: 目标文件 I/O 与二进制流序列化；
- `test_fixture/`: 目标文件与 Mach-O 原型测试生成器；
- `build_and_test.sh`: 链接器一键编译、模拟目标文件生成、跨模块链接与执行验收脚本。

---

## 核心技术突破

1. **AAPCS64 重定位类型支持**：
   - `LK_ARM64_BRANCH26`: `b` / `bl` 26 位相对跳转重定位；
   - `LK_ARM64_PAGE21`: `adrp` 指令计算目标符号相对于当前 PC 的 4KB 页面基地址（立即数 `immhi:immlo` 拼接）；
   - `LK_ARM64_PAGEOFF12`: `add` / `ldr` 指令提取低 12 位页内偏移。
2. **苹果内核 AMFI 与强制代码签名**：
   - Apple Silicon 架构强制内核级 AMFI 校验，未签名的可执行程序会被直接触发 `SIGKILL (COREDUMP)` 强杀；
   - `c1link-macho` 在生成 Mach-O 时，预留足够的头空间和段结构，并在输出后自动调用 `codesign -s - --force` 完成 Ad-hoc 签名。

---

## 运行验证

```bash
chmod +x build_and_test.sh
./build_and_test.sh
```
