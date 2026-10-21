# BitKindle macOS (Darwin ARM64) 移植行动路线图

本项目旨在将 BitKindle 从 Windows x64 自举工具链完整移植到 Apple Silicon (macOS Darwin ARM64)。按照“自上而下验证，自下而上闭环”（路线 B）渐进式推进。

---

## 阶段规划

### 阶段 1：基础设施与最小 Mach-O 生成验证（当前阶段）
- [x] 创建 `macos-arm64/` 独立工程脚手架。
- [x] 开发 macOS 原生构建与验证脚本（POSIX / Zsh）。
- [x] 实现并验证最小独立 Mach-O + ARM64 二进制生成器原型（证明纯字节流拼装在 Apple Silicon 上的可执行性与代码签名机制）。

### 阶段 2：Mach-O 目标文件链接器 (`c1link-macho`)
- [x] 移植并适配 `.c1o` 目标文件读取器与写入器（支持 ARM64 `0xAA64`）。
- [x] 编写 `linker/macho.c`，实现原生 64 位 Mach-O 生成（`__PAGEZERO`, `__TEXT`, `__DATA`, `__LINKEDIT`）。
- [x] 支持 ARM64 重定位类型（`LK_ARM64_BRANCH26`, `LK_ARM64_PAGE21`, `LK_ARM64_PAGEOFF12`）。
- [x] 自动接入 Apple Silicon 现代代码签名（`codesign`）并建立标准 ARM64 启动桩。
- [x] 通过全自动端到端集成测试（`test_linker.sh`），目标文件生成、链接与执行验证通过（Exit Code: 42）。

### 阶段 3：编译器后端 ARM64 代码生成器 (`c1-arm64`)
- [x] 开发独立的 C 编译器前端与 ARM64 代码生成器（[`c1_arm64.c`](file:///Users/rambo/pjts/BitKindle-fork/macos-arm64/src/compiler/c1_arm64.c)）。
- [x] 实现 AAPCS64 标准的栈帧控制（`stp/ldp x29, x30`，16 字节栈对齐）。
- [x] 实现 ARM64 整数运算（`add`, `sub`, `mul`, `sdiv`）、比较条件置位（`cset`）与控制流分支（`cbz`, `b`, `bl`）。
- [x] 实现了遵循 Apple Silicon ABI 的多参数函数调用与寄存器传递（X0-X7）。
- [x] 成功打通 `C 源码 -> c1-arm64 -> .c1o 目标文件 -> c1link-macho -> Mach-O 可执行文件 -> 运行验证` 全流程（斐波那契循环与算术计算返回 42）。

### 阶段 4：Darwin 运行时与标准库适配 (`runtime-darwin`)
- [ ] 替换原项目的 `KERNEL32.dll` 依赖，重构底层系统接口（`sys_read`, `sys_write`, `sys_mmap`, `sys_exit`）。
- [ ] 接入并调试 C 标准库基础功能（`stdio`, `stdlib`, `string`, `math`）。
- [ ] 验证语言功能与标准库回归用例。

### 阶段 5：自举闭环（Self-Hosting）与反推极简种子
- [ ] 用新工具链自编译自身源码，完成第一代与第二代逐字节一致性校验。
- [ ] 逆向简化构造出 ARM64 汇编过渡层（A0/A1-arm64、C0-arm64）。
- [ ] 生成最终的 3KB 级 `seed-arm64.hex` 机器码种子，达成纯原生种子自举。
