[English](WALKTHROUGH.md) · 简体中文

# 逐阶段构建

本页提供逐阶段构建命令；原理与依赖关系见 [BOOTSTRAP.zh-CN.md](BOOTSTRAP.zh-CN.md)。

在发布包根目录的 **Windows PowerShell 5.1** 中依次执行第 1–7 步，每步成功后再继续。产物位于 `work/walkthrough/`；下文 `base/`、`host/` 等源码路径相对于 `src/bootstrap-c99/`。完整验收可直接执行第 8 步，无须先走分阶段路线。

如果默认执行策略禁止运行脚本，可先运行 `powershell.exe -NoProfile -ExecutionPolicy Bypass` 启动新的 PowerShell 会话，再执行本页命令。这只影响该会话，不修改持久执行策略。

## 自举产物基准

分阶段脚本比较各工具的自举产物，并固定校验 A0 种子和最终 C1 第二代的以下 SHA256。其余代次通过一致性比较校验；链接器仅比较两代产物，不校验固定哈希。基准不能替代功能测试。

| 产物 | SHA256 |
| --- | --- |
| A0 种子及两代 A0 | `BA0F84742B07A4A5340D0641B676EE18269943191139B570667D9984A98465E5` |
| 最终 C1 两代编译器 | `DB92844256433C7020D0C399E8F32D9CB9BE4954CC4063EAC0DFEAD0A4570ADD` |

## 1. 解码种子

输入：`src/bootstrap-c99/base/seed.hex`。该文件是完整 PE 程序的字面十六进制文本，注释和空白不参与内容。

```powershell
./build-stage.ps1 -Stage Seed
```

脚本调用系统 PowerShell 5.1，移除注释、校验十六进制字节对，并用 `WriteAllBytes` 写出 3,584 字节的 `work/walkthrough/base/a0-seed.exe`。标签解析、指令及 PE 结构均不由解码脚本生成。

成功标志：`PASS walkthrough stage Seed`，并匹配本页基准哈希表中的种子 SHA256。

也可单独运行解码脚本：

```powershell
New-Item -ItemType Directory -Force work | Out-Null
& "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File ./src/bootstrap-c99/base/decode_seed.ps1 `
    -Source ./src/bootstrap-c99/base/seed.hex -Output ./work/a0-seed.exe
if ($LASTEXITCODE -ne 0) { throw '种子解码失败' }
Get-FileHash ./work/a0-seed.exe -Algorithm SHA256
```

此命令输出到 `work/a0-seed.exe`，用于单独检查种子；继续本教程仍须执行 `-Stage Seed`，以生成所需路径下的产物。

## 2. A0 装配自己

```powershell
./build-stage.ps1 -Stage A0
```

输入 `base/a0.a0`；种子生成 `a0-stage1.exe`，第一代生成 `a0-stage2.exe`。验证三者哈希一致，并执行 A0 功能和错误处理测试。

## 3. 用 A0 构建 A1

```powershell
./build-stage.ps1 -Stage A1
```

输入 `base/a1.a0`；A0 生成 `a1-stage0.exe`，A1 再生成自身两次。验证三代一致并执行 A1 测试。

## 4. 用 A1 构建 C0

```powershell
./build-stage.ps1 -Stage C0
```

输入 `base/c0.a1`；A1 生成 `c0.exe` 并执行 C0 测试。C0 支持桥接所需的 C 子集，输出 A1 装配文本。

## 5. 用 C0 编译 C 写的词法分析器

```powershell
./build-stage.ps1 -Stage Lexer
```

输入 `base/c1/lexer.c`；C0 两次编译得到相同的 A1 文本，再由 A1 得到 `c1-lexer.exe` 并测试。该步骤保留后续自编译词法分析器的对照数据。

## 6. 桥接到能自编译的基础 C1

```powershell
./build-stage.ps1 -Stage C1
```

这里运行 `base/build-c1.ps1`：

1. C0 + A1 编译 `base/c1/bootstrap.c`，得到 `c1-bridge.exe`。
2. 桥接编译器编译容量桥接源码，得到 `c1-capacity.exe`。
3. 容量桥接编译器编译基础 C1 源码，得到 `c1-stage0.exe`。
4. C1 第零代→第一代→第二代，验证后两代逐字节一致。
5. 自制工具重建数学表，执行两代功能测试，并重建词法分析器作对照。

日志：`work/walkthrough/C1.log`；尾部应包含自编译与两代功能测试通过标记。

## 7. 最终工具链

```powershell
./build-stage.ps1 -Stage Toolchain
```

脚本 `build-toolchain.ps1` 依次构建标识符扩展编译器、目标文件输出编译器、自制链接器，再构建 `host/c1-stage0.exe`、`host/c1-stage1.exe`、`host/c1-stage2.exe`。运行库随相应一代编译器重新编译，编译器和运行库目标文件的两代产物均作逐字节比较，并执行集成测试。

最终工具在 `work/walkthrough/host/c1-stage2.exe` 和 `work/walkthrough/link-stage2.exe`。看到 `PASS walkthrough stage Toolchain` 后可运行示例：

```powershell
./src/examples/minicurl/build.ps1 -BuildDirectory ./work/walkthrough
if (!$?) { throw '示例编译失败' }
./src/examples/minicurl/minicurl.exe --help
./src/examples/minicurl/test.ps1
```

## 8. 完整交付回归与审计

分阶段路线不包含全部最终语言、标准库回归及补充验收。完整验收使用：

```powershell
./build-all.ps1 -Full
```

此命令在 `src/bootstrap-c99/build-from-seed/` 中重新从种子构建，不复用 walkthrough 产物。该目录与 `work/release-run/` 均须不存在，重建方法见 [README.zh-CN.md](README.zh-CN.md#清理与重新构建)。两代最终编译器各须通过 4,992 项回归，并通过总审计、三个补充包及 README 所列示例验收。结果保存在 `work/release-run/RESULT.json`。

## 查看进度与处理失败

在另一个 PowerShell 窗口按当前构建方式选择日志：

```powershell
# 分阶段 C1
Get-Content ./work/walkthrough/C1.log -Tail 20
# 默认或 -All 的工具链构建
Get-Content ./work/release-run/01-fast-build.txt -Tail 20
# -Full 的基础工具构建
Get-Content ./src/bootstrap-c99/build-from-seed/BASE-VERIFICATION.txt -Tail 20
```

脚本不覆盖已有的分阶段日志。失败时先检查并保存日志；重试前可用 `clean-all.ps1` 清理默认构建和逐阶段产物（范围见 README），或使用未构建的发布包副本。不要用旧 EXE 补齐输出。

拒绝测试可能输出预期的 `C1 error`。是否通过以脚本退出状态和最终 PASS 标志为准，不能仅凭生成了 EXE 或单条诊断判断。

## 附录：自定义构建目录并分别运行审计

以下命令分别执行构建、总审计及三个补充套件，须使用新的构建和测试目录。它不包含 `-Full` 的 hello、参数示例和 minicurl 验收。

```powershell
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force work | Out-Null
$Snapshot='src/bootstrap-c99'
$Build='work/reproduce-build'
$Tests='work/reproduce-supplemental'
if(Test-Path $Build){throw '请选择未存在的构建目录'}
if(Test-Path $Tests){throw '请选择未存在的补充测试目录'}
foreach ($path in 'work/reproduce-implementation','work/reproduce-language') {
    if(Test-Path $path){throw "请选择未存在的测试目录：$path"}
}

& "$Snapshot/build.ps1" -BuildDirectory $Build *> work/reproduce-build.log
if(!$?){throw '种子构建未通过'}

& "$Snapshot/audit.ps1" -BuildDirectory $Build -VerificationLog work/reproduce-build.log *> work/reproduce-audit.log
if(!$?){throw '完整构建审计未通过'}

& src/c89-supplemental-tests/run.ps1 -Snapshot $Snapshot -BuildDirectory $Build -ScratchDirectory $Tests *> work/reproduce-supplemental.log
if(!$?){throw '补充测试未通过'}

& src/c89-supplemental-tests/audit.ps1 -ScratchDirectory $Tests -VerificationLog work/reproduce-supplemental.log *> work/reproduce-supplemental-audit.log
if(!$?){throw '补充测试审计未通过'}

& src/c89-implementation-tests/run.ps1 -Snapshot $Snapshot -BuildDirectory $Build -ScratchDirectory work/reproduce-implementation *> work/reproduce-implementation.log
if(!$?){throw '实现约定探针未通过'}

& src/c89-implementation-tests/audit.ps1 -ScratchDirectory work/reproduce-implementation -VerificationLog work/reproduce-implementation.log *> work/reproduce-implementation-audit.log
if(!$?){throw '实现约定探针审计未通过'}
& src/language-regression-tests/run.ps1 -Snapshot $Snapshot -BuildDirectory $Build -ScratchDirectory work/reproduce-language *> work/reproduce-language.log
if(!$?){throw '语言回归探针未通过'}

& src/language-regression-tests/audit.ps1 -ScratchDirectory work/reproduce-language -VerificationLog work/reproduce-language.log *> work/reproduce-language-audit.log
if(!$?){throw '语言回归审计未通过'}
```

总审计不要传入 `-Smoke`。它检查完成标志、测试结果及产物；补充套件还审计测试源码和产物一致性。此路线生成 USAGE 示例所需的 C89/C99 运行库；使用 [USAGE.zh-CN.md](USAGE.zh-CN.md) 时，将其初始化代码中的 `$build` 改为本例构建目录的绝对路径，再设置 `$compiler` 和 `$linker`。上述自定义目录与日志不在 `clean-all.ps1` 的清理范围内。
