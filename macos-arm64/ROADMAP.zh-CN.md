# BitKindle macOS (Darwin ARM64) 移植行动路线图

本项目旨在将 BitKindle 从 Windows x64 自举编译工具链完整移植到 Apple Silicon (macOS Darwin ARM64)。按照“自上而下验证，自下而上闭环”（路线 B）渐进式推进，已全线达成 100% 成功交付。

---

## 阶段演进总览

```text
阶段 1: 基础环境与原型验证 ──► 阶段 2: Mach-O 链接器 ──► 阶段 3: ARM64 编译器后端
                                                                    │
阶段 6: 三大应用程序移植 ◄── 阶段 5: 两代自编译确定性闭环 ◄── 阶段 4: Darwin 自包含运行库
         │
         ▼
阶段 7: 双轨架构重组 (Top-Down / Down-Top) 与全套双语技术文档
         │
         ▼
阶段 8: 原生 macOS Cocoa 交互式游戏窗口版 (mario-extended)
```

---

## 详细阶段达成记录

### 阶段 1：基础设施与最小 Mach-O 生成验证
- [x] 创建 `macos-arm64/` 独立工程脚手架与环境探测（`check_env.sh`）。
- [x] 开发 macOS 原生构建与验证脚本（POSIX / Zsh）。
- [x] 实现并验证最小独立 Mach-O + ARM64 二进制生成器原型（证明纯字节流拼装在 Apple Silicon 上的可执行性与代码签名机制）。

### 阶段 2：Mach-O 目标文件链接器 (`c1link-macho`)
- [x] 移植并适配 `.c1o` 目标文件读取器与写入器（支持 ARM64 `0xAA64`）。
- [x] 编写 `macho.c`，实现原生 64 位 Mach-O 段表与节区生成（`__PAGEZERO`, `__TEXT`, `__DATA`, `__LINKEDIT`）。
- [x] 适配 Apple Silicon 硬件强制的 16KB 页边界物理对齐（`0x4000`）。
- [x] 支持 AAPCS64 重定位类型（`LK_ARM64_BRANCH26`, `LK_ARM64_PAGE21`, `LK_ARM64_PAGEOFF12`）。
- [x] 自动接入苹果现代内核 AMFI 代码签名（`codesign -s - --force`）并建立标准 ARM64 启动桩。
- [x] 通过全自动端到端集成测试，目标文件生成、链接与执行验证通过（Exit Code: 42）。

### 阶段 3：编译器后端 ARM64 代码生成器 (`c1-arm64`)
- [x] 开发独立的 C 编译器前端与 ARM64 代码生成器（`c1_arm64.c`）。
- [x] 实现 AAPCS64 标准的栈帧控制（`stp/ldp x29, x30`，严格保持 16 字节栈对齐）。
- [x] 实现 ARM64 整数运算（`add`, `sub`, `mul`, `sdiv`）、比较条件置位（`cset`）与控制流分支（`cbz`, `b`, `bl`）。
- [x] 实现遵循 Apple Silicon ABI 的多参数函数调用与寄存器传递（X0-X7）。
- [x] 成功打通 `C 源码 -> c1-arm64 -> .c1o 目标文件 -> c1link-macho -> Mach-O 可执行文件 -> 运行验证` 全流程（循环与算术计算精确返回 42）。

### 阶段 4：Darwin 运行时与标准库适配 (`runtime-darwin`)
- [x] 开发 Darwin 纯自包含运行时模块（`build_runtime.c`），生成 `runtime.c1o`。
- [x] 基于 Darwin 原生裸系统调用（`svc #0x80`）实现系统 I/O 与退出（`sys_write`, `sys_exit`，`x16` 传递调用号）。
- [x] 实现并导出基础标准库 I/O 函数（`putchar`, `print`, `puts`, `print_int`）。
- [x] 在 `c1-arm64` 编译器中新增支持字符串字面量常量池与外部未定义符号导出（`LK_UNDEFINED`）。
- [x] 完成全自动跨模块静态链接与控制台标准输出端到端验收（输出格式化算术结果，返回码 42）。

### 阶段 5：自举闭环（Self-Hosting）与反推极简种子
- [x] 手工编写并生成 Darwin ARM64 极简机器码种子（`seed-arm64.hex`）。
- [x] 编写纯 POSIX 种子解码与签名驱动（`decode_seed.sh`），验证从纯十六进制解码出可运行的原生种子程序并回显。
- [x] 实现自编译闭环（`bootstrap_core.c`），完成 Gen1 与 Gen2 目标文件的生成。
- [x] 执行两代目标文件逐字节一致性校验（`test_self_host.sh`），目标文件均为 336 字节，SHA256 哈希 100% 逐字节完全相同（`7e9e070420...`），退出码 42。

### 阶段 6：原仓库三大应用程序全量移植
- [x] **`hello` 与命令行参数 `args`**：升级链接器启动桩通过栈保存并透传 dyld 传入的 `x0 (argc)` 与 `x1 (argv)`，C1 编译器支持多级指针语法，测试退出码 0。
- [x] **`minicurl` 网络客户端**：以原生 BSD/POSIX Sockets (`socket`, `connect`, `send`, `recv`) 替换 Windows WinHTTP API，实现 HTTP/1.0 GET 抓取与分块落盘，本地测试内容 100% 匹配。
- [x] **`mario` 超级马里奥兄弟 World 1-1**：100% 保留原版物理碰撞引擎、地图矩阵与敌人 AI，解耦 Win32 GDI 依赖，内置 32 位内存 FrameBuffer 与 BMP 导出，通过 `--self-test`、`--smoke-test` 与 `--demo-test`（AI 自动通关第一关）。

### 阶段 7：双轨架构重组与全套中英双语文档
- [x] **`01-top-down/`（高层工业集成视图）**：整合全量编译器、链接器、运行时与应用套件，提供 7 步一键自动化流水线（`build-all.sh`）。
- [x] **`02-down-top/`（阶梯演进天梯视图）**：仿照原版从 0 到 N 阶梯哲学，设立 `step00-seed` 至 `step05-applications` 独立子目录，配备单步独立测试与总阶梯流水线。
- [x] **顶层中英双语技术文档**：编写 `README.md` 与 `README.zh-CN.md`，深度剖析 macOS Darwin ARM64 与 Windows PE32+ x64 底层 9 大技术差异，明确注明由 **Gemini 3.8 Flash (Medium)** 模型独立设计并打通。

### 阶段 8：原生 macOS Cocoa 交互式游戏窗口版 (`mario-extended`)
- [x] 在 `examples/mario-extended/` 下独立实现原生 macOS Cocoa 游戏窗口（不污染原版无头测试代码）。
- [x] 零第三方库依赖，纯 C 与 Objective-C 原生驱动，60 FPS 流畅渲染与全键盘交互（移动、跳跃、冲刺、暂停、重启、退出）。
- [x] 消除绘制反转矩阵，保证游戏画面正向（HUD 在上、马里奥与地面在下、天空在上方）。
- [x] 支持玩家亲自键盘游玩（`build.sh --run`）与弹窗观摩 AI 自动通关演示（`build.sh --demo`）。
