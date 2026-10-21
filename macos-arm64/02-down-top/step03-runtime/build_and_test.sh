#!/usr/bin/env bash
#
# step03-runtime/build_and_test.sh
# 生成原生 Darwin 运行时目标文件，并验证跨模块标准 I/O 应用程序
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

mkdir -p build

echo "==> [Step 03] 编译 Darwin 运行时生成器..."
clang -Wall -Wextra -I ../step01-linker/src src/build_runtime.c -o build/build_runtime

echo "==> 生成自包含运行时目标文件 (build/runtime.c1o)..."
./build/build_runtime build/runtime.c1o

echo "==> 确保前序步骤的编译器和链接器就绪..."
if [ ! -f ../step01-linker/build/c1link-macho ]; then
    bash ../step01-linker/build_and_test.sh >/dev/null
fi
if [ ! -f ../step02-compiler/build/c1-arm64 ]; then
    bash ../step02-compiler/build_and_test.sh >/dev/null
fi

echo "==> 使用自制 c1-arm64 编译 I/O 应用程序 (tests/hello.c)..."
../step02-compiler/build/c1-arm64 -o build/hello.c1o tests/hello.c

echo "==> 使用自制 c1link-macho 跨模块链接 hello.c1o 与 runtime.c1o..."
../step01-linker/build/c1link-macho build/hello_app main build/runtime.c1o build/hello.c1o

echo "==> 运行生成的可执行程序..."
echo "--------------------------------------------------"
set +e
./build/hello_app
RET_VAL=$?
set -e
echo "--------------------------------------------------"

echo "程序退出码: $RET_VAL"
if [ $RET_VAL -eq 42 ]; then
    echo "[PASS] Step 03 原生 Darwin 运行时跨模块验证完全通过！"
else
    echo "[FAIL] 运行退出码异常 (预期 42，实际 $RET_VAL)"
    exit 1
fi
