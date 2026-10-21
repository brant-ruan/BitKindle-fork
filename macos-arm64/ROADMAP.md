# BitKindle macOS (Darwin ARM64) 移植行动路线图

本项目旨在将 BitKindle 从 Windows x64 自举工具链完整移植到 Apple Silicon (macOS Darwin ARM64)。按照“自上而下验证，自下而上闭环”（路线 B）渐进式推进。

---

## 阶段规划

### 阶段 1：基础设施与最小 Mach-O 生成验证（当前阶段）
- [x] 创建 `macos-arm64/` 独立工程脚手架。
- [x] 开发 macOS 原生构建与验证脚本（POSIX / Zsh）。
- [x] 实现并验证最小独立 Mach-O + ARM64 二进制生成器原型（证明纯字节流拼装在 Apple Silicon 上的可执行性与代码签名机制）。

### 阶段 2：Mach-O 目标文件链接器 (`c1link-macho`)
- [ ] 移植并适配 `.c1o` 目标文件读取器。
- [ ] 编写 `linker/macho.c`，实现原生 64 位 Mach-O 生成（`__PAGEZERO`, `__TEXT`, `__DATA`, `__LINKEDIT`）。
- [ ] 支持 ARM64 重定位类型（`ARM64_RELOC_BRANCH26`, `ARM64_RELOC_PAGE21`, `ARM64_RELOC_PAGEOFF12`）。
- [ ] 实现针对 Darwin 的基础动态导入或系统调用桩。

### 阶段 3：编译器后端 ARM64 代码生成器 (`c1-arm64`)
- [ ] 复用原 C1 编译器的平台无关前端（词法、预处理、类型系统、语法解析、浮点转换）。
- [ ] 实现针对 ARM64 的寄存器约定与栈帧分配（AAPCS64 Darwin 变种）。
- [ ] 实现基本算术、逻辑、内存访问（LDR/STR）、分支（B/BL/CBZ/RET）指令生成。
- [ ] 打通 `c1-arm64 -> .c1o -> c1link-macho -> 运行` 的单源文件链路。

### 阶段 4：Darwin 运行时与标准库适配 (`runtime-darwin`)
- [ ] 替换原项目的 `KERNEL32.dll` 依赖，重构底层系统接口（`sys_read`, `sys_write`, `sys_mmap`, `sys_exit`）。
- [ ] 接入并调试 C 标准库基础功能（`stdio`, `stdlib`, `string`, `math`）。
- [ ] 验证语言功能与标准库回归用例。

### 阶段 5：自举闭环（Self-Hosting）与反推极简种子
- [ ] 用新工具链自编译自身源码，完成第一代与第二代逐字节一致性校验。
- [ ] 逆向简化构造出 ARM64 汇编过渡层（A0/A1-arm64、C0-arm64）。
- [ ] 生成最终的 3KB 级 `seed-arm64.hex` 机器码种子，达成纯原生种子自举。
