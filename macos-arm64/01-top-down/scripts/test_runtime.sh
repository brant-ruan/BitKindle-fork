#!/usr/bin/env bash
#
# test_runtime.sh
# 验证阶段 4：Darwin 原生运行时模块 (runtime-darwin) 与跨模块链接控制台输出
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

mkdir -p build

echo "==> 1. 编译构建阶段 4 原生 Darwin 运行时 (runtime.c1o)..."
clang -Wall -Wextra src/runtime/build_runtime.c -o build/build_runtime
./build/build_runtime build/runtime.c1o

echo "==> 2. 使用 c1-arm64 编译 C 语言标准 I/O 应用程序 (hello.c)..."
./build/c1-arm64 -o build/hello.c1o src/runtime/hello.c

echo "==> 3. 使用自制 c1link-macho 跨模块链接 hello.c1o 与 runtime.c1o..."
./build/c1link-macho build/hello_app main build/hello.c1o build/runtime.c1o

echo "==> 4. 运行生成的可执行程序..."
echo "--------------------------------------------------"
set +e
OUTPUT=$(./build/hello_app)
RET_VAL=$?
set -e
echo "$OUTPUT"
echo "--------------------------------------------------"
echo "==> 应用程序执行结束，退出码 (Exit Code): $RET_VAL"

# 检查关键输出内容与退出码
if [ $RET_VAL -eq 42 ] && echo "$OUTPUT" | grep -q "80235"; then
    echo "[PASS] 完美！字符串打印、数值格式化与系统调用完全正常，退出码精确为 42！阶段 4 验收完全成功！"
else
    echo "[FAIL] 输出或返回值不符合预期！"
    exit 1
fi
