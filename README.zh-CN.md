<div align="center">

# BitKindle

[English](README.md) · 简体中文

### 从 3,584 字节机器码种子自举的 Windows x64 C 编译器

[![Platform: Windows x64](https://img.shields.io/badge/platform-Windows_x64-0078D4?style=flat-square)](#快速开始)
[![Seed: 3584 bytes](https://img.shields.io/badge/seed-3%2C584_bytes-EA580C?style=flat-square)](BOOTSTRAP.zh-CN.md)
[![Bootstrap: self-hosted](https://img.shields.io/badge/bootstrap-self--hosted-16A34A?style=flat-square)](BOOTSTRAP.zh-CN.md)
[![License: MIT](https://img.shields.io/badge/license-MIT-64748B?style=flat-square)](LICENSE)

[快速开始](#快速开始) · [自举原理](BOOTSTRAP.zh-CN.md) · [逐阶段教程](WALKTHROUGH.zh-CN.md) · [使用手册](USAGE.zh-CN.md) · [示例程序](#示例程序)

</div>
<p align="center">
  <img src="docs/media/mario-demo.gif" alt="BitKindle 自制工具链构建的 Mario" width="800" />
</p>

<p align="center">
  <strong>使用 BitKindle 工具链构建的 Mario 示例</strong><br/>
  实际游戏录屏 · 内置自动玩家完整通关 · 预览为 2 倍速<br/>
  <a href="src/examples/mario/README.zh-CN.md">构建与运行说明</a>
</p>

---

## 项目概述

BitKindle 包含 A0/A1 装配器、C0 过渡编译器、用 C 编写的自举编译器 **C1**、运行库和 PE 链接器的完整源码。

**构建不依赖 MSVC、GCC、Clang、Windows SDK 或外部链接器，无需联网下载依赖。** PowerShell 5.1 负责种子解码与构建调度；解码之后的所有机器码、目标文件和 EXE 均由已构建的自制工具生成。

## 工具链自举流程

```mermaid
flowchart LR
    S["十六进制种子<br/>3,584 字节"] --> A["A0 → A1<br/>自制装配器"]
    A --> C["C0 → 桥接阶段<br/>过渡编译器"]
    C --> O["C1 逐级扩展<br/>支持目标文件"]
    O --> L["自制 PE 链接器"]
    O --> H["最终 C1 + 运行库"]
    L --> H
    H --> G1["自编译第一代"]
    G1 --> G2["自编译第二代"]
    G1 -. "逐字节一致" .-> G2
```

阶段依赖见 [BOOTSTRAP.zh-CN.md](BOOTSTRAP.zh-CN.md)；使用 `build-stage.ps1` 逐阶段构建的步骤见 [WALKTHROUGH.zh-CN.md](WALKTHROUGH.zh-CN.md)。

## 快速开始

需要 **Windows x64 + Windows PowerShell 5.1**（`powershell.exe`）；已验证环境为 Windows 10 x64。将完整源码目录放到可写的本地短路径，保留 `src/` 的名称和层级。完整测试建议预留 **5 GiB** 磁盘空间。

在发布包根目录执行：

```powershell
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File .\build-all.ps1
```

默认构建编译器、链接器和 C89/C99 运行库，**不运行回归测试**。成功后，编译并运行 hello 示例：

```powershell
$snapshot = './src/bootstrap-c99'
& "$snapshot/compile-program.ps1" -Source "$snapshot/examples/hello.c" `
    -Output './work/my-hello.exe' -BuildDirectory "$snapshot/build-from-seed" -Standard c89
if (!$?) { throw '编译失败' }
& './work/my-hello.exe'
```

预期输出 `hello`。

### 构建模式

在构建命令后追加参数选择模式。三种模式均从种子构建，并校验自举一致性与编译器基准；`-All` 与 `-Full` 不能同时使用。

| 参数 | 构建与测试范围 |
| :--- | :--- |
| 无参数 | 工具链和运行库，不运行回归测试 |
| `-All` | 默认模式加编译 hello、参数示例、minicurl 和 Mario；不运行示例或回归测试 |
| `-Full` | 各阶段回归、完整审计、三套补充验收，以及 hello、参数示例和 minicurl 本机 HTTP 测试；Mario 的构建与测试使用其独立入口 |

日志与结果位于 `work/release-run/`。默认与 `-All` 的 `RESULT.json` 为 `Status=BUILT`、`TestsRun=false`；`-Full` 成功时为 `Status=PASS`、`TestsRun=true`，终端显示 `PASS release build and verification`。输出目录存在不代表构建成功。

`build-all.ps1` 在 `src/bootstrap-c99/build-from-seed/` 或 `work/release-run/` 已存在时拒绝运行。重新构建或切换模式前，先[清理默认产物](#清理与重新构建)，或使用新的源码副本。

## 示例程序

| 示例 | 功能与接口 | 文档 |
| :--- | :--- | :--- |
| **Hello / 命令行参数** | C 运行库、程序启动、`argc/argv` | [编译和使用](USAGE.zh-CN.md) |
| **minicurl** | 通过系统 WinHTTP 提供 HTTP/TLS，抓取内容并保存到文件 | [源码与用法](src/examples/minicurl/README.zh-CN.md) |
| **Mario 第一关** | 横向卷轴、键盘输入、碰撞、计时器与 GDI 双缓冲；画面由 C 代码绘制 | [构建与操作](src/examples/mario/README.zh-CN.md) |

已有工具链时，可按各示例文档单独构建。构建成功后手动运行：

```powershell
# GUI 平台游戏
& './src/examples/mario/build/mario.exe'

# 可选联网演示，需要正常的 Windows 网络与证书环境
& './src/examples/minicurl/minicurl.exe' -o './work/example.html' 'https://example.com/'
```

Mario 为教学用途的玩法重现，功能差异见其说明。

## 构建产物与接口

主要工具位于：

```text
src/bootstrap-c99/build-from-seed/
├── host/c1-stage2.exe    # C 编译器
└── link-stage2.exe       # 自制 Windows x64 PE 链接器
```

C1 从标准输入读取 C 源码，向标准输出写 PE 或 `.c1o` 目标文件。普通程序使用 `compile-program.ps1` 配置头文件、运行库和链接参数。

多源文件编译、命令行参数、Windows DLL 导入和 GUI 子系统用法见 [USAGE.zh-CN.md](USAGE.zh-CN.md)。

## 支持范围

- **C89 与多项 C99 特性**：包括 VLA、复数、泛型数学、通用字符名、inline、复合字面量、指定初始化、变参宏、`long long`、`_Bool` 和十六进制浮点。语言与库行为见[实现约定](src/C89-IMPLEMENTATION.zh-CN.md)。
- **目标文件与链接器**：使用自制 `.c1o` 格式，生成 Windows x64 PE，支持显式 DLL 导入及控制台 / GUI 子系统。
- **兼容边界**：不宣称完整 C99 符合性，不承诺全部 C99 翻译容量或可选浮点 Annex F/G；`long double` 使用 binary64。通用 Windows SDK 头文件、COFF `.obj/.lib` 和外部工具链兼容不在当前范围内，现有开源 C 项目可能需要适配。

自举一致性与功能测试不构成完整正确性证明或标准认证。具体容量与限制见[限制说明](src/C89-LIMITS.zh-CN.md)。

## 技术文档

| 主题 | 文档 |
| :--- | :--- |
| 种子解码与逐级自举原理 | [自举原理](BOOTSTRAP.zh-CN.md) |
| 各阶段构建命令与验证步骤 | [逐阶段教程](WALKTHROUGH.zh-CN.md) |
| 应用程序编译与链接 | [使用手册](USAGE.zh-CN.md) |
| 源码组织与工具链组成 | [工具链源码导览](src/bootstrap-c99/README.zh-CN.md) |
| `.c1o` 符号与重定位格式 | [目标文件格式](docs/C1O-FORMAT.zh-CN.md) |
| 数学函数与内部算法 | [技术文档导览](docs/README.zh-CN.md) |

## 清理与重新构建

先关闭示例程序并等待构建、测试结束，备份需要保留的产物与日志，再执行：

```powershell
# 先预览，再清理，最后重新从种子构建
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File .\clean-all.ps1 -WhatIf
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File .\clean-all.ps1
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File .\build-all.ps1
```

清理范围仅限以下默认输出位置；不会删除源码、文档或修改系统设置：

- `src/bootstrap-c99/build-from-seed/`
- `work/release-run/`
- `work/walkthrough/`
- `src/examples/minicurl/build/`
- `src/examples/minicurl/minicurl.exe`
- `src/examples/mario/build/`

请勿在这些位置保存手写文件。上述目录会整体删除，包括内部的 `.c1-build-*`。清单外的自定义构建目录、USAGE 示例和 `.c1-build-*` 需自行管理；脚本不会清空整个 `work/`。

脚本检查清理路径，并使用 `SOURCES.json` 中的路径防止误删源码，不检查源码哈希。遇到越界或符号链接 / 目录联接时停止。不要与构建并发执行；若文件占用导致部分清理失败，解除占用后可重试。

## 许可证

源码及随附项目文档采用 [MIT License](LICENSE)。Windows、PowerShell 与系统 DLL 由使用者的系统提供，不属于本项目 MIT 许可的源码。
