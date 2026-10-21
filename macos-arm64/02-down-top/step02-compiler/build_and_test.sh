#!/usr/bin/env bash
#
# step02-compiler/build_and_test.sh
# 构建 ARM64 C1 递归下降编译器后端，编译 C 代码并运行验证
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

mkdir -p build

echo "==> [Step 02] 编译 c1-arm64 原生 ARM64 C 语言编译器..."
clang -Wall -Wextra -Werror -I ../step01-linker/src src/c1_arm64.c -o build/c1-arm64

echo "==> 确保 step01-linker 中的链接器就绪..."
if [ ! -f ../step01-linker/build/c1link-macho ]; then
    bash ../step01-linker/build_and_test.sh >/dev/null
fi

echo "==> 使用 c1-arm64 编译包含函数调用、循环与算术的 C 源码 (test_calc.c)..."
./build/c1-arm64 -o build/test_calc.c1o tests/test_calc.c

echo "==> 使用自制 c1link-macho 链接生成 Mach-O 可执行文件..."
../step01-linker/build/c1link-macho build/test_calc_app main build/test_calc.c1o

echo "==> 运行编译产物并验证计算结果..."
set +e
./build/test_calc_app
RET_VAL=$?
set -e

echo "程序退出码: $RET_VAL"
if [ $RET_VAL -eq 42 ]; then
    echo "[PASS] Step 02 ARM64 C1 编译器验证完全通过！"
else
    echo "[FAIL] 运行退出码异常 (预期 42，实际 $RET_VAL)"
    exit 1
fi
