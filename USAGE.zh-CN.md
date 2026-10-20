[English](USAGE.md) · 简体中文

# 使用 C1 编译器与 PE 链接器

单源文件程序可使用驱动脚本；多源文件或额外 Windows DLL 导入可直接调用编译器和链接器。

## 准备工具

在发布包根目录的 **Windows PowerShell 5.1** 中执行以下示例，先完成 `build-all.ps1`（任一模式）。源码包不含预编译程序，构建说明见 [README.zh-CN.md](README.zh-CN.md)。

```powershell
$ErrorActionPreference = 'Stop'
$snapshot = (Resolve-Path './src/bootstrap-c99').Path
$build = Join-Path $snapshot 'build-from-seed'
$compiler = Join-Path $build 'host/c1-stage2.exe'
$linker = Join-Path $build 'link-stage2.exe'
foreach ($tool in $compiler, $linker) {
    if (!(Test-Path -LiteralPath $tool)) { throw "请先完成构建：$tool" }
}
New-Item -ItemType Directory -Force './work/usage' | Out-Null
```

后续代码块在同一会话中沿用这些变量。重复执行会覆盖同名示例源码和产物。

## 用驱动脚本编译一个程序

`compile-program.ps1` 配置头文件、编译目标文件、选择运行库并链接：

```powershell
@'
#include <stdio.h>
int main(void) {
    puts("Hello from C1!");
    return 0;
}
'@ | Set-Content -LiteralPath './work/usage/hello.c' -Encoding ascii

& "$snapshot/compile-program.ps1" `
    -Source './work/usage/hello.c' `
    -Output './work/usage/hello.exe' `
    -BuildDirectory $build -Standard c89
if (!$?) { throw '编译或链接失败' }
& './work/usage/hello.exe'
if ($LASTEXITCODE -ne 0) { throw "程序退出码：$LASTEXITCODE" }
```

预期打印 `Hello from C1!`。

| 驱动参数 | 含义 |
| --- | --- |
| `-Source` | 必填，单个 C 源文件路径 |
| `-Output` | 必填，目标路径，扩展名必须为 `.exe` |
| `-BuildDirectory` | 已构建工具链的目录；本页使用 `build-from-seed`。脚本自身默认是源码目录下的 `build`，因此这里显式指定 |
| `-Standard c89` 或 `c99` | 语言模式，驱动默认 `c89`；选择 `c99` 不代表支持全部 C99 特性 |
| `-MainArguments` | 为 `int main(int argc, char **argv)` 构建相应启动运行库 |
| `-Subsystem console` 或 `windows` | PE 子系统，默认 `console`；GUI 程序选择 `windows`，启动时不自动分配控制台 |
| `-Snapshot` | 源码与头文件目录，通常不需要设置，默认是驱动脚本所在目录 |

例如编译包内的命令行参数示例：

```powershell
& "$snapshot/compile-program.ps1" `
    -Source "$snapshot/examples/args.c" `
    -Output './work/usage/args.exe' `
    -BuildDirectory $build -Standard c89 -MainArguments
if (!$?) { throw '编译失败' }
& './work/usage/args.exe' 'two words' 42
```

驱动在输出程序旁创建 `.c1-build-*` 目录，保留中间源码和目标文件，用后可清理。额外 DLL 导入须直接调用链接器。

## 编译器 EXE 的接口

`host/c1-stage2.exe` 从**标准输入读取 C 源码**，向**标准输出写二进制**，诊断写入标准错误。它不接受源码文件名或 GCC 风格的 `-c`、`-o`、`-I`、`-std` 参数；未关闭输入流时会等待。

| 环境变量 | 接受的值及用途 |
| --- | --- |
| `C1_STANDARD` | `c89` 或 `c99`。未设置时为内部默认模式，不应当作明确的标准选择；用户程序建议始终设置 |
| `C1_OUTPUT` | `object` 输出自制 `.c1o`；未设置时直接输出 PE。其他非空值会报错，不能设置为 `exe` |
| `C1_INCLUDE` | 用分号分隔的头文件目录列表，建议使用绝对路径 |
| `C1_RUNTIME` | 普通应用程序应清空。`c89` 仅用于运行库构建的导出策略，且要求 `C1_OUTPUT=object`，不是语言模式开关 |

直接 PE 输出用于自举和自包含程序，不自动链接完整标准库。使用标准库时须将 `.c1o` 与运行库链接。

Windows PowerShell 5.1 的文本重定向和管道（包括 `>`、`Out-File`、`Set-Content`）不能保存二进制输出。示例中的 `Set-Content` 仅写 C 文本；编译使用按原始字节流传输的 `Compile` 函数：

```powershell
. "$snapshot/tools/compile.ps1"
# 接口：Compile 工具完整路径 源码路径 输出路径
# 调用失败会抛出异常；不要用 $LASTEXITCODE 判断这个函数的结果。
```

输入源码和输出文件不能使用同一路径。失败时可能留下空文件或不完整文件，应以成功退出为准。

## 分别编译和链接：一个完整示例

以下示例分别编译两个 C 文件，再链接 C89 运行库；编译环境变量在结束时恢复。

```powershell
@'
int add(int a, int b) { return a + b; }
'@ | Set-Content './work/usage/add.c' -Encoding ascii
@'
#include <stdio.h>
int add(int a, int b);
int main(void) {
    printf("sum = %d\n", add(20, 22));
    return 0;
}
'@ | Set-Content './work/usage/main.c' -Encoding ascii

. "$snapshot/tools/compile.ps1"
$out = (Resolve-Path './work/usage').Path
$saved = @{}
foreach ($name in 'C1_INCLUDE','C1_STANDARD','C1_OUTPUT','C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_INCLUDE = ((@(
        'include','components/wide-input','components/wide-io',
        'components/wctype','components/inttypes','base/include'
    ) | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null

    Compile $compiler "$out/add.c" "$out/add.c1o"
    Compile $compiler "$out/main.c" "$out/main.c1o"

    $runtime = Join-Path $build 'namespace/runtime-policy2/runtime.c1o'
    if (!(Test-Path -LiteralPath $runtime)) {
        throw '缺少 C89 运行库，请完成 build-all.ps1'
    }
    $linkArgs = @(
        "$out/sum.exe", '__c1_start',
        "$out/main.c1o", "$out/add.c1o", $runtime
    )
    foreach ($api in (Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json)) {
        $linkArgs += @('--import', ('__c1_win_' + $api), 'KERNEL32.dll', $api)
    }
    & $linker @linkArgs
    if ($LASTEXITCODE -ne 0) { throw "链接失败：$LASTEXITCODE" }
} finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
& "$out/sum.exe"
if ($LASTEXITCODE -ne 0) { throw "程序退出码：$LASTEXITCODE" }
```

预期输出 `sum = 42`。跨文件函数通过声明引用，由链接器解析符号。

改用 C99 时，将应用编译模式设为 `c99`，并使用 `$build/host/runtime2.c1o`。上述预构建运行库用于不接收 `argc/argv` 的示例；需要命令行参数时，使用驱动的 `-MainArguments`，或参考 minicurl 的构建脚本重新编译相应启动运行库。

分阶段教程的 Toolchain 阶段生成 `host/runtime2.c1o`，但不生成本页 C89 示例所需的 `namespace/runtime-policy2/runtime.c1o`。minicurl 构建脚本会自行编译所需运行库，因此可直接使用该阶段的工具；本页示例则以 `build-all.ps1` 产物为准。

## 链接器 EXE 的接口

调用形式：

```text
link-stage2.exe output.exe entry-symbol input.c1o ... [--subsystem console|windows] [--import symbol DLL export|#ordinal]
```

| 参数 | 含义 |
| --- | --- |
| `output.exe` | 输出 PE 文件路径；链接器自己写文件，不通过标准输出传输 EXE |
| `entry-symbol` | 目标文件中有定义的全局入口符号；配合本项目运行库通常为 `__c1_start` |
| `input.c1o ...` | 一个或多个本项目目标文件，包含程序和所需运行库 |
| `--subsystem console` 或 `windows` | 选择控制台或 GUI 子系统；省略时为 `console`。只能指定一次，名称区分大小写 |
| `--import symbol DLL export` | 将目标文件中的外部符号 `symbol` 绑定到指定 DLL 的导出函数名 |
| `--import symbol DLL #ordinal` | 按导出序号导入；序号范围为 1–65535 |

`--import` 可重复使用。PowerShell 中序号参数应写成字符串，例如 `'#123'`，避免被解释为注释。路径含空格时使用引号或像完整示例那样使用参数数组。

运行库入口 `__c1_start` 负责程序启动，不能用 `main` 直接替代。运行库所需的 `__c1_win_*` 导入之外，应用还须导入自身调用的 Windows API。

## 调用 Windows DLL

应用声明 Windows API 函数，再显式导入。例如目标文件引用 `GetCurrentProcessId` 时，在链接前追加：

```powershell
$linkArgs += @('--import', 'GetCurrentProcessId', 'KERNEL32.dll', 'GetCurrentProcessId')
```

对应声明为 `unsigned int GetCurrentProcessId(void);`。导入只绑定符号；函数原型、参数、返回值和结构布局须符合目标 API。

[minicurl](src/examples/minicurl/README.zh-CN.md) 的 `build.ps1` 展示了命令行参数运行库和 WINHTTP.dll 等导入的完整用法。

工具使用自制 `.c1o`，不接收 COFF `.obj` 或 SDK `.lib`，不提供通用 `windows.h` 或 GCC/MSVC 命令兼容层。链接器仅生成 EXE，不支持生成 DLL。

## 诊断与常见问题

| 情况 | 检查方法 |
| --- | --- |
| 提示找不到编译器或运行库 | 确认 `build-all.ps1`（任一模式）成功，且 `-BuildDirectory` 指向正确目录 |
| 编译器一直等待 | 检查是否提供标准输入并关闭输入流；推荐用 `Compile` |
| EXE 无法运行、目标文件无效 | 检查是否用文本重定向保存过二进制，以及编译是否成功 |
| 找不到头文件 | 检查 `C1_INCLUDE`；顶层源码从标准输入进入，应在正确工作目录运行或明确加入项目头文件目录 |
| `C1_OUTPUT must be object or unset` | 将变量设为 `object` 或清空，不能设为 `exe` |
| `C1_RUNTIME=c89 requires object output` | 普通应用清空该变量；运行库策略要求目标文件模式 |
| `cannot read valid object` | 检查输入是否为本工具生成的完整 `.c1o` 文件 |
| `entry symbol has no global definition` | 检查入口拼写及是否包含提供该入口的目标文件 |
| `invalid or duplicate symbol definition` | 检查重复定义、重复加入的运行库及导入符号冲突 |

编译器和链接器成功返回 0，失败返回非零。直接运行 EXE 后检查 `$LASTEXITCODE`；驱动与 `Compile` 的失败则通过 PowerShell 错误报告。链接失败时不要运行旧的同名 EXE，以免误把旧产物当成新结果。

语言、标准库及实现容量边界见 [C89-IMPLEMENTATION.zh-CN.md](src/C89-IMPLEMENTATION.zh-CN.md) 和 [C89-LIMITS.zh-CN.md](src/C89-LIMITS.zh-CN.md)。

## GUI 子系统

GUI 程序可向驱动传入 `-Subsystem windows`，或向链接器传入 `--subsystem windows`。链接器在生成 PE 时写入 Subsystem 字段：Windows GUI 为 2，控制台为 3；参见 [Microsoft PE 格式规范](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#windows-subsystem)。入口仍可为本项目的 `__c1_start` 并调用 `main`，不需要为子系统选项改成 WinMain。

子系统选项不会创建窗口或改变标准库功能；GUI 程序须自行创建窗口，不能依赖自动分配的控制台输出 stdout/stderr。[Mario 示例](src/examples/mario/README.zh-CN.md) 展示固定步长游戏循环与 GDI 双缓冲。
