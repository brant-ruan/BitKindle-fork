# BitKindle for macOS (Apple Silicon ARM64)

BitKindle 移植到 macOS (Darwin arm64) 的独立工程分支。

---

## 移植理念与原则

本移植遵循**“路线 B：自上而下验证，自下而上闭环”**：
1. **指令集转换**：从 x86_64 迁移到 AArch64 (ARM64)，遵从 Apple Silicon 寄存器约定与 AAPCS64 栈帧规范。
2. **二进制格式转换**：从 Windows PE32+ (`.exe`) 迁移到 macOS 原生 64 位 Mach-O。
3. **系统调用与动态链接**：从 Windows `KERNEL32.dll` 迁移到 macOS 动态链接器 `dyld` 与 `/usr/lib/libSystem.B.dylib`（等价于 macOS 上的基础 C 运行库）。
4. **安全合规**：生成的 Mach-O 二进制集成 Apple Silicon 强制的代码签名（Code Signing）机制。

---

## 目录结构

```text
macos-arm64/
├── README.md               # 移植概览与使用文档
├── ROADMAP.md              # 详细阶段行动计划与进度跟踪
├── scripts/                # 构建、测试与环境检查脚本
│   ├── check_env.sh        # 环境检查脚本（OS/ARCH/clang/codesign/otool）
│   └── test_prototype.sh  # 原型构建与验证脚本
├── src/                    # 源码骨架
│   ├── prototype/          # 原型验证代码（Mach-O 裸格式生成与探路）
│   ├── linker/             # 即将移植的 Mach-O 目标文件链接器 (c1link-macho)
│   ├── compiler/           # 即将移植的 ARM64 C1 编译器 (c1-arm64)
│   ├── runtime/            # macOS 原生 Darwin 运行时适配代码
│   └── include/            # 平台头文件
└── build/                  # 中间文件与最终构建产物目录
```

---

## 快速上手与验证

在当前目录下执行环境检查：

```bash
chmod +x scripts/*.sh
./scripts/check_env.sh
```

---

## 模块移植规划

- **目标文件链接器 (`src/linker/`)**：
  复用原有平台的 `.c1o` 目标文件解码器，用 `macho.c` 替代原先的 Windows `pe.c`，生成包含 `__PAGEZERO`、`__TEXT`、`__LINKEDIT` 的原生 Mach-O。
- **编译器代码生成 (`src/compiler/`)**：
  复用 `src/bootstrap-c99/host/compiler.c` 的通用前端（递归下降解析、词法扫描、大数浮点转换），替换其后端代码生成器，输出 32 位定长的 ARM64 指令。
- **运行库 (`src/runtime/`)**：
  提供 Darwin 平台的 `sys_write`, `sys_read`, `sys_mmap`, `sys_exit` 映射，并无缝对接 libSystem。
