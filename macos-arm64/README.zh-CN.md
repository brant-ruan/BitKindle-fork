# BitKindle macOS (Darwin ARM64) 移植与自举实现
 
> 本项目基于 **[tombkeeper](https://github.com/tombkeeper)** 原创的 [BitKindle](https://github.com/tombkeeper/BitKindle) 项目移植构建。
>
> **模型注明 (Model Attribution)**：  
> 本项目的 macOS Apple Silicon (Darwin ARM64) 平台适配与自举移植实现，由 **Gemini 3.8 Flash (Medium)** 模型协助完成架构设计、代码编写与测试验证。

---

## 一、项目背景与复现思路

BitKindle 原版是一套运行于 Windows x64 平台上的极简 C 自举编译系统，其核心思想是从一小段裸十六进制机器码（Seed）出发，一步步构建出链接器、C 编译器和运行库，并最终实现自举。

原版的实现紧密贴合 Windows x64 平台的特性（x86-64 指令集、PE32+ 格式、Win32 API）。本项目尝试将这一自举流程完整移植到 **Apple Silicon (macOS Darwin ARM64)** 平台，探索不同软硬件生态下的自举实践：
- **CPU 架构**：从 x86-64 CISC 切换为 ARM64 (AArch64) RISC，处理固定的 32 位指令编码与 AAPCS64 调用规约；
- **可执行文件格式**：从 Windows PE32+ 切换为 macOS 64 位 Mach-O，并适配苹果特有的 16KB 物理页边界对齐与 AMFI 代码签名要求；
- **系统调用与运行时**：剥离 Windows 依赖，基于 Darwin 内核的 `svc #0x80` 裸系统调用实现自包含的最小运行时。

通过在 macOS 上复现 BitKindle 的自举链条，我们希望能为关注底层编译原理与系统级编程的开发者提供一份针对 Apple Silicon 平台的参考实现。

---

## 二、双轨架构设计 (Dual-Track Organization)

为了满足不同层次的学习、研究与工程应用需求，本项目在 `macos-arm64/` 目录下采用了**双轨制（Dual-Track）**的文件与目录组织形式：

```text
macos-arm64/
├── build-all.sh                   # 顶层一键验收总管线 (同时串联运行两套视图)
├── README.md                      # [双语] 英文总览与技术剖析文档
├── README.zh-CN.md                # [双语] 中文总览与技术剖析文档
│
├── 01-top-down/                   # 【高层视角 · 工业级集成视图】
│   ├── build-all.sh               # 7 步全自动化一键构建与回归流水线
│   ├── src/                       # 模块化源码 (base, linker, compiler, runtime, examples)
│   ├── scripts/                   # 自动化集成测试套件
│   ├── README.md & .zh-CN.md      # Top-Down 体系说明与快速上手
│   └── .gitignore
│
└── 02-down-top/                   # 【阶梯演进 · 从零到N自举天梯】
    ├── build-all.sh               # 从 Step 00 到 Step 05 的阶梯自举总流水线
    ├── step00-seed/               # 阶段 0：纯十六进制 ARM64 机器码种子与解码器
    ├── step01-linker/             # 阶段 1：独立 64 位 Mach-O 链接器与代码签名 (c1link-macho)
    ├── step02-compiler/           # 阶段 2：原生 ARM64 C1 递归下降编译器后端 (c1-arm64)
    ├── step03-runtime/            # 阶段 3：自包含 Darwin 运行库与系统调用 I/O (runtime.c1o)
    ├── step04-self-hosting/       # 阶段 4：两代自编译闭环与逐字节确定性校验 (Byte-for-byte SHA256)
    ├── step05-applications/       # 阶段 5：三大完整移植应用程序验收 (hello/args, minicurl, mario)
    ├── README.md & .zh-CN.md      # Down-Top 阶梯哲学与操作说明
    └── .gitignore
```

1. **`01-top-down/`（整体集成视图）**：
   面向系统集成与工程实践。将种子、链接器、编译器、运行时和应用程序作为一个有机整体，通过高度自动化的脚本管线，在几秒内一键完成全平台自举验证。
2. **`02-down-top/`（阶梯演进视图）**：
   忠实还原原仓库的核心灵魂，仿照“从无到有”的自举天梯。每一个 Step 都是一个独立的目录，包含该阶段的独立源码、自包含验证脚本（`run.sh` / `build_and_test.sh`）以及详尽的中英双语设计说明。

---

## 三、macOS (Darwin ARM64) 与 Windows (PE32+ x64) 深度技术对比

在移植打通的过程中，我们深入攻克并系统性解决了两大平台底层的九大核心技术差异：

| 维度 | Windows x64 (原版 BitKindle) | macOS Darwin ARM64 (本项目移植版) | 移植攻坚与解决方案 |
| :--- | :--- | :--- | :--- |
| **可执行文件格式** | **PE32+ (Portable Executable)**<br>DOS 头 + PE 签名 + COFF 头 + 可选头 + 节区表 (`.text`, `.data`) | **Mach-O 64-bit** (`MH_EXECUTE`)<br>`mach_header_64` + `LC_SEGMENT_64` (`__PAGEZERO`, `__TEXT`, `__DATA`, `__LINKEDIT`) | 重写链接器 `c1link-macho`，从零发射合法 64 位 Mach-O 二进制结构。 |
| **CPU 架构与指令集** | **x86-64 (AMD64)**<br>CISC 复杂指令集，1~15 字节变长指令，支持内存直接操作数。 | **ARM64 (AArch64)**<br>RISC 精简指令集，固定 32 位（4 字节）指令宽度，Load/Store 架构。 | 重构编译器后端，生成原生 AArch64 指令流，处理 ARM64 独特的立即数反转编码与 PC 相对寻址。 |
| **调用规约 (ABI)** | **Microsoft x64 ABI**<br>前 4 个参数由 `RCX, RDX, R8, R9` 传递；**强制预留 32 字节影子栈空间 (Shadow Space)**；`RAX` 返回。 | **AAPCS64 (ARM 64-bit Architecture)**<br>前 8 个整型参由 `x0~x7` 传递；`x0` 返回；`x29` 为帧指针 (FP)，`x30` 为链接返回地址 (LR)。 | 栈帧通过 `stp x29, x30, [sp, #-16]!` 与 `mov x29, sp` 维护，严格保持 16 字节对齐。 |
| **重定位计算机制** | **PE/COFF 相对偏移**<br>`IMAGE_REL_AMD64_REL32` 计算 `Target - PC - 4`。 | **ARM64 页面与分支重定位**<br>`LK_ARM64_PAGE21` (`adrp` 计算 4KB 页基址)；`LK_ARM64_PAGEOFF12` (低12位页内偏移)；`LK_ARM64_BRANCH26` (26位跳转)。 | 链接器实现了完整的两阶段重定位修补算法，准确将符号地址拆分装配到 ARM64 操作码中。 |
| **内存物理分页限制** | **标准 4KB (0x1000) 边界**<br>虚拟地址与文件对齐通常为 4KB 或 512 字节。 | **硬件强制 16KB (0x4000) 边界**<br>Apple Silicon M 系列芯片强制要求虚拟内存以 16KB 页对齐。 | 二进制文件所有段（`__PAGEZERO`, `__TEXT`）严格以 `0x4000` 对齐，这也是为什么种子程序膨胀为 34KB 的物理根因。 |
| **代码签名与内核安全** | **Authenticode (非强制)**<br>普通用户态程序无签名亦可直接执行。 | **AMFI (Apple Mobile File Integrity) 强校验**<br>内核级强制要求可执行文件携带代码签名，无签名直接触发 `SIGKILL` 强杀。 | `c1link-macho` 在构建二进制时预留足够的段表结构与签名槽位，并在写盘后自动调用 `codesign -s - --force` 完成 Ad-hoc 签名。 |
| **系统调用机制 (Syscall)** | **间接调用**<br>通过 `ntdll.dll` 动态加载分发，用户态不承诺系统调用编号稳定。 | **Darwin 裸内核陷入 (`svc #0x80`)**<br>类 BSD 系统调用规范；系统调用号直接置于 `x16` 寄存器（如 `x16=4` 为 `sys_write`，`x16=1` 为 `sys_exit`）。 | 构建纯静态原生运行库 `runtime.c1o`，脱离任何系统动态库，直接在汇编层级通过 `svc` 与 Darwin 内核握手。 |
| **网络通信基础设施** | **WinHTTP API**<br>依赖 Windows 专有的 `winhttp.dll` 高级封装。 | **原生 BSD/POSIX Sockets**<br>基于操作系统标准的 Berkeley Socket (`socket`, `connect`, `send`, `recv`)。 | 移植 `minicurl`，使用纯 POSIX Socket 重新实现 HTTP/1.0 GET 抓取协议与分块落盘，命令行接口 100% 兼容。 |
| **图形渲染与游戏呈现** | **Win32 GDI 窗体**<br>`CreateWindowExA`, `FillRect`, `GetPixel`, `GetDIBits` 与 Windows 消息循环。 | **解耦式 32-bit FrameBuffer + BMP 导出**<br>内置 960×544 RGBA 帧缓冲、5×7 像素文字绘制器，支持无头测试与标准 BMP 导出。 | 移植 `mario`，100% 完整保留物理碰撞引擎、地图矩阵与 AI 自动通关玩家，移除了 Windows 专有 GDI 调用。 |

---

## 四、两代自编译闭环验证 (Byte-for-byte Determinism)

在两代自举闭环测试中，第零代编译器（Gen0）编译生成第一代目标文件（Stage 1），随后使用自制的 `c1link-macho` 将其链接为可执行程序并编译同一份源码，输出第二代目标文件（Stage 2）。

在 macOS ARM64 上，两代目标文件均精确为 **336 字节**，生成的 SHA256 散列值实现**逐字节强一致**：

```text
Stage 1 SHA256: 7e9e070420fda3d32f40bd7ca3d6deb0cef9966ca55e783e297a6afec6f79bac
Stage 2 SHA256: 7e9e070420fda3d32f40bd7ca3d6deb0cef9966ca55e783e297a6afec6f79bac
Exit Code: 42 (PASS)
```

这在数学上证明了：自编译生成的目标代码具有确定性，不依赖宿主环境的隐式状态，自举闭环完全确立。

---

## 五、三大移植应用程序验收成果

原仓库所包含的三大完整应用程序已全部成功移植到 macOS ARM64 平台并通过自动化验收：

1. **`hello` 与命令行参数解析 `args`**：
   - 路径：`01-top-down/src/examples/hello/` 与 `02-down-top/step05-applications/src/hello/`；
   - 链接器启动桩成功将 dyld 传入的 `x0 (argc)` 与 `x1 (argv)` 透传给 `main` 函数；
   - `./hello` 成功输出 `hello`，`./args "two words" "42"` 返回码精准为 `0`。
2. **`minicurl` 网络客户端**：
   - 路径：`src/examples/minicurl/minicurl.c`；
   - 采用原生 POSIX Socket 连接，支持 `minicurl -o <file> <url>`；
   - 自动化启动本地测试服务器下载测试文件，内容**逐字节比对 100% 完全匹配**。
3. **`mario` 超级马里奥兄弟 World 1-1 第一关完整游戏**：
   - 路径：`src/examples/mario/mario.c` 与 `demo.h`；
   - **三大模式全线通关**：
     - `--self-test`: 物理和游戏机制自测（重力、跳跃高度、问号顶蘑菇、吃蘑菇变大、受击变小、龟壳反弹等全部通过，退出码 0）；
     - `--smoke-test`: 成功导出 `mario_smoke.bmp`（2.0MB，960×544，可用 macOS「预览」查看高清游戏画面）；
     - `--demo-test`: 内置启发式束搜索（Beam Search）AI 算法自动操作马里奥通关第一关，通关报告：
       ```text
       PASS state=2 ticks=1634 x=6339 coins=10 boxes=9 mushrooms=1 goombas=11 turtles=3 kicks=2 shellkills=0 loose=2 hurts=0
       ```
       （耗时 1634 帧，无伤跑完全图，吃 10 枚金币，顶 9 个方块，踩死 11 只板栗仔、3 只乌龟，踢了 2 次龟壳，最终到达终点旗杆夺旗！）
4. **`mario-extended` 原生 macOS Cocoa 交互式游戏窗口版**：
   - 路径：`src/examples/mario-extended/`；
   - **零第三方库依赖**：纯 C 与 Objective-C 编写，直接链接系统内置 `-framework Cocoa`；
   - 启动原生 960×544 游戏窗口，以 60 FPS 流畅运行，支持键盘实时控制（A/D 移动，空格/W 跳跃，Shift 奔跑，P 暂停，R 重启，ESC 退出）；
   - 一键直接在 Mac 屏幕上游玩：`./01-top-down/src/examples/mario-extended/build.sh --run`；
   - 亦可弹窗实时观摩 AI 自动通关全过程：`./01-top-down/src/examples/mario-extended/build.sh --demo`。

---

## 六、快速上手指南

在 `macos-arm64/` 目录下，您可以随时通过一键脚本进行验证：

```bash
# 1. 顶层统合双轨一键全量验证 (同时验证 01-top-down 与 02-down-top)
chmod +x build-all.sh
./build-all.sh

# 2. 仅运行 01-top-down 整体工业级集成视图
cd 01-top-down
./build-all.sh

# 3. 仅运行 02-down-top 阶梯演进天梯视图
cd ../02-down-top
./build-all.sh
```
