[English](README.md) · 简体中文

# minicurl：使用自制 C 编译器抓取网页

Windows x64 命令行 GET 客户端，使用系统 WinHTTP 实现 HTTP/HTTPS。支持重定向、查询参数、分块响应、正文标准输出、`-o` 保存原始字节及显式 HTTP 代理。源码为 `minicurl.c`，产物为本目录的 `minicurl.exe`。

## 编译与运行

先在发布包根目录运行 `build-all.ps1`（任一模式）构建工具链，详见 [构建说明](../../../README.zh-CN.md#快速开始)。`-All` 已编译本例，`-Full` 还会执行本机 HTTP 测试；这两种情况可跳过下面的编译步骤。

以下命令在本示例目录的 **Windows PowerShell 5.1** 中执行：

```powershell
# 使用已构建的自制工具链编译示例。
./build.ps1
if (!$?) { throw '构建失败' }

# 将网页正文写到终端；HTTP 状态写到标准错误。
./minicurl.exe https://example.com/

# 保存网页。下载文件建议使用 -o，避免 PowerShell 5.1 的文本重定向转码。
./minicurl.exe -o example.html https://example.com/
$LASTEXITCODE

# 参数包含 & 等 shell 特殊字符时加引号。
./minicurl.exe -o result.html 'https://example.com/?a=1&b=2'

# 如有 HTTP 代理，填写实际主机与端口；不带 http:// 前缀。
./minicurl.exe --proxy 127.0.0.1:8080 -o example.html https://example.com/

./minicurl.exe --help
```

`build.ps1` 默认使用相对于脚本目录的 `../../bootstrap-c99/build-from-seed`。也可使用分阶段构建的工具链：

```powershell
./build.ps1 -BuildDirectory '../../../work/walkthrough'
if (!$?) { throw '构建失败' }
```

该目录须包含 `host/c1-stage2.exe` 和 `link-stage2.exe`；脚本自行编译所需运行库。自定义工具链不会改变示例输出位置。

## 构建过程

1. 将 `C1_STANDARD=c89`、`C1_OUTPUT=object` 及 `C1_INCLUDE` 设置为本项目的标准头文件目录。
2. 自制 `host/c1-stage2.exe` 从标准输入读取 `minicurl.c`，向标准输出写出 `build/minicurl.c1o`。脚本沿用项目 `Compile` 函数无变换转运这些字节。
3. 脚本生成很短的运行库包装源码，定义 `C1_MAIN_ARGS=1` 后包含 `runtime-c89.c`，再由同一个编译器生成 `build/runtime.c1o`，支持 `main(int, char **)`。
4. 自制 `link-stage2.exe` 将两个目标文件链接为 PE，以 `__c1_start` 为入口；`--import 符号 DLL 导出名` 指定 Windows 导入。例如 `--import WinHttpOpen WINHTTP.dll WinHttpOpen`。

所有目标文件和 EXE 均由自制工具生成。程序显式导入 KERNEL32.dll 和 WINHTTP.dll，不依赖 Windows SDK 头文件或 `.lib` 导入库。

`minicurl.c` 声明所需 Windows ABI，并在运行时检查 DWORD、WCHAR、指针和 URL 结构布局。

`build/provenance.json` 记录编译器、链接器和生成程序的 SHA256。根目录 `clean-all.ps1` 会删除本例的 `build/` 和 `minicurl.exe`，不会删除示例命令保存的 `example.html` 或 `result.html`。编译器与链接器接口见 [USAGE.zh-CN.md](../../../USAGE.zh-CN.md)。

## 测试

```powershell
# 本机 HTTP 测试；测试服务器只绑定 127.0.0.1 的随机端口。
./test.ps1

# 增加公开网页 HTTPS 测试；若设置 HTTPS_PROXY，测试显式传入该 HTTP 代理。
./test.ps1 -Internet
```

先完成示例构建。默认测试包含 12 个用例：帮助、参数错误、非法协议、嵌入凭据拒绝、HTTP、重定向、chunked、查询参数/片段、404、二进制文件与标准输出、输出失败。响应正文逐字节核对；二进制用例为 51,200 字节，覆盖全部 256 种字节值并跨多个读取缓冲区。

每个用例超时 60 秒。全部通过后写入 `build/tests/RESULTS.txt`，各用例标准输出和诊断保存在同目录；失败以当前脚本错误为准，不应将旧结果文件视为本次通过。`-Internet` 额外下载 HTTPS 页面到 `build/tests/example.html` 并检查内容。

可选 HTTPS 测试需要可用的网络与系统证书环境；若设置 `HTTPS_PROXY`，其值须为不含凭据的 `http://HOST:PORT`。代理并非必需。PowerShell/.NET 仅提供测试服务器和数据检查。

## 行为与范围

- 退出码：0 成功；2 参数或 URL 错误；3 网络/TLS 错误；4 输出错误；22 HTTP 状态 >= 400。HTTP 错误正文仍会输出。
- 默认使用 Windows 自动代理配置；`--proxy HOST:PORT` 指定 HTTP 代理。程序不会自动读取 curl 的代理环境变量。
- 证书验证和重定向使用 WinHTTP 默认策略，程序没有跳过证书校验或调整重定向策略的选项。
- 域名应使用 ASCII/punycode，非 ASCII 路径应先做 UTF-8 百分号编码。URL 最长 8,189 字节，不支持 URL 内嵌账号密码。
- 每个网络阶段设置 15 秒超时，非整个下载的总时限。正文流式读取，不把整个网页装入内存。
- `-o` 会覆盖指定文件；中途失败可能留下部分内容。标准输出写原始响应字节，终端如何显示取决于终端编码。
- 本例专注 GET，不实现 POST、Cookie 管理、自定义头、压缩内容解压或完整 curl 选项，也不会执行网页 JavaScript。

接口参考：[WinHTTP 请求流程](https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview)、[URL_COMPONENTS 布局](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/ns-winhttp-url_components)、[WinHttpCrackUrl](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpcrackurl)。
