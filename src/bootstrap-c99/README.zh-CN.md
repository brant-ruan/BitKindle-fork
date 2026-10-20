[English](README.md) · 简体中文

# BitKindle 自举工具链

本目录包含从十六进制种子构建 C1 编译器、运行库和 PE 链接器所需的源码及测试。本页面向源码阅读；环境和构建模式见[发布说明](../../README.zh-CN.md)，阶段依赖见[构建原理](../../BOOTSTRAP.zh-CN.md)，语言与库边界见[实现约定](../C89-IMPLEMENTATION.zh-CN.md)和[限制](../C89-LIMITS.zh-CN.md)。

## 构建与审计入口

以下脚本路径相对于本目录；调用时应在发布包根目录运行，以满足源码中的相对包含路径。

| 入口 | 职责 |
| --- | --- |
| 发布根目录 `build-all.ps1` | 推荐入口：默认快速构建，`-All` 加编译示例，`-Full` 执行完整发布验收，范围见发布说明。 |
| `build.ps1` | 从种子构建工具链，执行基础阶段审计、集成测试、语言与库测试，以及最终两代编译器回归。默认输出到本目录 `build/`。 |
| `audit.ps1` | 核对构建日志、测试结果及产物。单独调用须指定对应的 `-BuildDirectory` 和 `-VerificationLog`；完整审计不使用 `-Smoke`。 |
| `build-toolchain.ps1` | 从 `-BaseBuild` 中的基础 C1 构建后续阶段，输出到 `-BuildDirectory`；默认执行集成测试，`-SkipTests` 跳过测试但保留自举一致性比较。 |
| `tools/build-fast.ps1` | 供默认发布构建调用：跳过回归和集成测试，构建工具链及 C89/C99 用户程序运行库。 |
| `compile-program.ps1` | 编译单个用户源文件，选择运行库并调用链接器；参数见[使用手册](../../USAGE.zh-CN.md)。 |

本目录的 `build.ps1` 不代替完整发布验收：总审计、三套补充测试及示例验收由根目录 `build-all.ps1 -Full` 组织。逐阶段操作和自定义目录构建见 [WALKTHROUGH.zh-CN.md](../../WALKTHROUGH.zh-CN.md)。

## 源码目录

下表覆盖全部 15 个一级源码目录。早期编译器、组件和测试均是构建或验证输入，不属于生成产物。

| 目录 | 用途 |
| --- | --- |
| `base/` | 种子、A0/A1、C0、桥接编译器和基础 C1，以及配套头文件、基础库、构建脚本和测试。 |
| `identifier/` | 扩展标识符处理的 C1，由基础 C1 构建并自编译。 |
| `object/` | 支持自制 `.c1o` 输出的 C1；构建链接器及最终 C1 第零代所需目标文件。 |
| `host/` | 最终 C1 的主体 `compiler.c` 和 `object-output.c`；与运行库分别编译后链接。 |
| `linker/` | 读取 `.c1o`、合并节、解析符号与重定位、生成 DLL 导入及 Windows x64 PE，支持控制台和 GUI 子系统。 |
| `runtime/` | 完整、C89 和初始自举运行库的组合入口，Windows 导入清单 `imports.json`，以及 `lib/` 基础实现。 |
| `components/` | 按功能组织的运行库组件、配套头文件及导入声明。 |
| `complex/` | 复数与实数数学扩展、泛型数学头文件、测试及参考数据。 |
| `include/` | 公共标准库及内部配套头文件；驱动另加入部分组件目录和 `base/include/`。 |
| `features/` | 语言和标准库测试入口；`library/` 组织库测试，`run-full.ps1` 运行最终编译器回归。 |
| `diagnostics/` | 非法输入、预处理和语言模式诊断测试及审计。 |
| `namespace/` | 公共头文件、符号命名空间、C89/C99 接口约定及 locale、数值、编码、VLA 等边界测试；生成 C89 用户运行库。 |
| `integration/` | 多目标文件链接、运行库、DLL 导入和 PE 子系统集成测试。 |
| `examples/` | hello、命令行参数等最小示例；minicurl 和 Mario 位于发布包的 `src/examples/`。 |
| `tools/` | 编译进程与二进制流传输、快速构建、源码路径检查及产物审计脚本。 |

## 最终 EXE 的源码组成

`build-toolchain.ps1` 组织最终编译器的构建：

```text
host/compiler.c（包含 host/object-output.c） → compiler2.c1o ─┐
runtime/runtime.c → runtime2.c1o ────────────────────────────┼→ host/c1-stage2.exe
runtime/imports.json（Windows DLL 导入配置） ────────────────┘
```

完整运行库经 `components/wide-input/runtime.c` → `wide-output/runtime.c` → `wide-io/runtime.c` 接入基础库和宽字符等组件，并包含 `complex/lib/all.c` 与 `complex/lib/real-extra.c`，因此最终编译器的实现不只在 `host/` 中。

第零代使用 `runtime/bootstrap-runtime.c`，后两代使用 `runtime/runtime.c`。`runtime/runtime-c89.c` 为用户程序选择 C89 数学、数值和 locale 策略。各入口仅包含所需文件，不能将 `runtime/lib/` 全部视为实际编入的代码。

最终链接器由目标文件阶段的 C1 直接编译 `linker/link.c` 为 EXE。该入口包含 `linker/` 的其他实现，以及 `base/lib/memory.c`、`string.c`、`errno.c`、`stdio.c`；其中 `stdio.c` 还包含 `process.c`。它使用 `base/include/` 的头文件和直接 PE 输出方式生成的启动结构，没有另行链接最终编译器使用的 `runtime2.c1o`，也没有把 `host/`、`components/` 或 `complex/` 的实现作为链接器源码包含进来。

`identifier/` 和 `object/` 提供前置工具，其编译器源码不直接并入最终 EXE；测试代码也不编入最终工具。源码依赖以入口的包含关系和构建脚本为准。

## components 子目录

组件通过源码包含关系组合，无须逐个手工编译。`object-runtime/` 提供导入声明，其余组件有实现经完整运行库接入最终 C1；这些组件不属于最终链接器的实现。

| 子目录 | 用途 |
| --- | --- |
| `object-runtime/` | 保存目标文件链接所需的 Windows 导入符号声明，供编译器和运行库引用。 |
| `wide-io/` | 基础运行库、启动入口及宽字符文件 I/O；其 `lib/` 还包含内存、字符串、普通 stdio、locale、信号等基础实现，并不只处理宽字符。 |
| `wide-output/` | 宽字符格式化输出及十进制、十六进制数值格式化，接续 `wide-io` 的运行库。 |
| `wide-input/` | 宽字符格式化输入及浮点扫描，接续 `wide-output`；其 `runtime.c` 是最终运行库组合使用的组件入口。 |
| `wchar-string/` | 宽字符串操作函数及配套声明。 |
| `restartable/` | 带转换状态的、可重启的多字节字符与宽字符转换。 |
| `wide-integer/` | 宽字符串到整数的转换。 |
| `wide-float/` | 宽字符串到浮点数的转换与数值转换核心。 |
| `wide-time/` | 宽字符日期时间格式化。 |
| `wctype/` | 宽字符分类、大小写转换及相关描述符接口。 |
| `inttypes/` | 最大宽度整数接口、相关数值转换与 `<inttypes.h>` 声明。 |

## complex 的实现与测试

- `include/`：复数及泛型数学等配套头文件。
- `lib/`：复数运算、复数数学函数与实数数学扩展实现；`all.c`、`real-extra.c` 将相应实现组合进运行库。
- 顶层的 `*-client.c`、`*-reference.c`、`*-cases.ps1`：功能测试程序、参考数据与测试用例描述。
- `make-*-reference.ps1`：生成用于测试的 C 参考数据文本，不生成编译产物。
- `test-*.ps1`、`run.ps1`、`audit.ps1`、`SUITES.json`：单项测试、套件组织、结果审计与套件清单。

设计和算法说明见[技术文档](../../docs/README.zh-CN.md)。

## 构建后出现的目录

发布入口输出到本目录的 `build-from-seed/`；直接调用 `build.ps1` 默认输出到 `build/`。两者保存生成工具、目标文件及相应日志。

根目录 `clean-all.ps1` 清理 `build-from-seed/` 等[默认输出位置](../../README.zh-CN.md#清理与重新构建)，不清理本目录的 `build/` 或自定义目录。源码目录和 `SOURCES.json` 应保留；该清单记录构建与测试输入的路径，不固定文件哈希，不是生成产物清单。
