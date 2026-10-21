# BitKindle macOS (Darwin ARM64) - 顶层整体自举工具链 (01-top-down)

本项目是原版基于 Windows x64 的 BitKindle 极简 C 自举编译工具链在 Apple Silicon (macOS Darwin ARM64) 架构上的原生移植与打通实现。

`01-top-down` 目录提供了**工程化、高内聚、端到端一体化**的顶层视图：整个自举编译器、链接器、运行时以及三大应用程序紧密协同，构成完整的系统级生产管线。

---

## 目录结构

```text
01-top-down/
├── build-all.sh               # 一键总构建流水线 (Step 1 ~ Step 7 全自动回归验证)
├── scripts/                   # 各模块单测与端到端测试脚本
│   ├── check_env.sh           # 宿主系统环境检查 (Darwin, arm64, clang, codesign)
│   ├── decode_seed.sh         # 十六进制种子解码工具
│   ├── test_linker.sh         # Mach-O 链接器重定位与代码签名验证
│   ├── test_compiler.sh       # c1-arm64 递归下降代码发射与循环算术验证
│   ├── test_runtime.sh        # 自包含运行时、系统调用与控制台 I/O 验证
│   ├── test_self_host.sh      # 两代自举目标文件逐字节确定性校验 (Byte-for-byte SHA256)
│   ├── test_minicurl.sh       # POSIX Socket 网络客户端抓取验证
│   └── test_all_examples.sh   # 3 大应用程序综合验收套件
└── src/                       # 核心源代码
    ├── base/                  # ARM64 极简机器码种子 (seed-arm64.hex)
    ├── linker/                # 独立 64 位 Mach-O 链接器 (link.c, macho.c, etc.)
    ├── compiler/              # ARM64 原生 C 语言编译器后端 (c1_arm64.c)
    ├── runtime/               # Darwin 自包含运行库生成器 (build_runtime.c)
    └── examples/              # 3 款移植应用程序
        ├── hello/             # hello.c 与命令行参数 args.c
        ├── minicurl/          # 原生 POSIX HTTP/1.0 客户端 (minicurl.c)
        └── mario/             # 超级马里奥兄弟 World 1-1 游戏引擎 (mario.c, demo.h)
```

---

## 快速开始

在当前目录下执行一键构建脚本即可跑通从硬件架构检查到三款应用程序运行的全部流程：

```bash
./build-all.sh
```

流水线会自动执行以下 7 个关键步骤：
1. **[Step 1/7] 环境检查**：验证 Darwin arm64 内核、Apple Clang 与签名工具链；
2. **[Step 2/7] 纯机器码种子解码**：将 `seed-arm64.hex` 解码为原生 Mach-O 可执行种子，并验证标准 I/O 管道回显；
3. **[Step 3/7] 自制工具链编译**：生成 `build/c1link-macho` 与 `build/c1-arm64`；
4. **[Step 4/7] Darwin 运行库构建**：发射 Darwin 原生系统调用实现（`sys_write`、`sys_exit`、`print_int` 等），生成 `build/runtime.c1o`；
5. **[Step 5/7] 阶段回归测试**：分别验证链接器符号重定位、编译器循环/算术发射与标准控制台 I/O 应用程序；
6. **[Step 6/7] 两代自编译确定性闭环**：Gen0 编译 Stage1 目标文件，Stage1 自编译生成 Stage2，逐字节校验 SHA256 哈希值完全一致；
7. **[Step 7/7] 三大移植应用程序验收**：
   - `hello` & `args`：由自研 `c1-arm64` 与 `c1link-macho` 编译链接运行，验证 dyld 参数透传；
   - `minicurl`：启动本地 HTTP 服务，通过 POSIX Socket 下载数据并校验内容；
   - `mario`：运行物理自测（`--self-test`）、导出 BMP 游戏截图（`--smoke-test`）与 AI 启发式自动通关第一关（`--demo-test`）。

---

## 核心设计要点

1. **Mach-O ARM64 链接与签名**：
   - 适配了 Apple Silicon 的 16KB 页边界对齐（`0x4000`）；
   - 正确处理 AAPCS64 重定位类型（`PAGE21`、`PAGEOFF12` 以及 `BRANCH26`）；
   - 预留段表并集成 `codesign -s -`，彻底解决 macOS 内核 AMFI 强校验导致的 `SIGKILL (COREDUMP)`。
2. **AAPCS64 栈帧规约**：
   - 严格维护 16 字节栈对齐；
   - 通过 `stp x29, x30, [sp, #-16]!` 与 `mov x29, sp` 维护调用栈；
   - 前 8 个整型参数经由 `x0` ~ `x7` 传递，返回值使用 `x0`。
3. **原生系统调用**：
   - 使用 Darwin `svc #0x80` 系统调用触发指令；
   - 寄存器 `x16` 存放类 BSD 系统调用编号（`x16=4` 为 `sys_write`，`x16=1` 为 `sys_exit`）。
