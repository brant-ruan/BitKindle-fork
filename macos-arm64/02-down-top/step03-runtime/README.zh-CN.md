# 阶段 3：自包含 Darwin 运行库与标准 I/O (Step 03 - Runtime)

## 概述

C 语言程序需要与宿主操作系统交互以输出字符和格式化数值。在传统工具链中，这依赖于由系统供应商提供的庞大动态库（如 macOS 的 `libSystem.dylib` 或 Linux 的 `glibc`）。

BitKindle 坚持**零外部运行时依赖**。在本阶段中，我们构建了一个纯原生、自包含的 Darwin ARM64 静态运行时模块 `runtime.c1o`。

---

## 文件结构

- `src/build_runtime.c`: 机器指令生成器，直接向目标文件发射 ARM64 裸系统调用汇编与 I/O 函数；
- `tests/hello.c`: 测试程序，验证字符串打印、换行符输出与十进制数值格式化；
- `build_and_test.sh`: 运行库构建、跨模块链接与全功能验证脚本。

---

## 核心实现机制

1. **导出函数符号列表**：
   - `sys_write(int fd, const char *buf, int len)`: 底层通过 `svc #0x80` (`x16=4`) 发起写系统调用；
   - `sys_exit(int code)`: 底层通过 `svc #0x80` (`x16=1`) 发起退出系统调用；
   - `putchar(int c)`: 写入单个 ASCII 字符到标准输出；
   - `print(const char *s)`: 遍历打印以 `\0` 结尾的字符串；
   - `puts(const char *s)`: 打印字符串并附加 `\n` 换行；
   - `print_int(int val)`: 支持负数处理的十进制整数递归格式化输出。
2. **跨模块符号解析与绑定**：
   - `c1link-macho` 将由 `c1-arm64` 生成的 `hello.c1o` 与自制的 `runtime.c1o` 自动合并，完成跨目标文件的符号引用修正与地址重定向。

---

## 运行验证

```bash
chmod +x build_and_test.sh
./build_and_test.sh
```
