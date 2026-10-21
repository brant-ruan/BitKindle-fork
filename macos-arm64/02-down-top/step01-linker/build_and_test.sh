#!/usr/bin/env bash
#
# step01-linker/build_and_test.sh
# 构建 Mach-O 独立链接器并验证 ARM64 重定位与代码签名
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

mkdir -p build

echo "==> [Step 01] 编译自制 64 位 Mach-O 链接器 (c1link-macho)..."
clang -Wall -Wextra -Werror src/link.c -o build/c1link-macho

echo "==> 编译目标文件测试生成器 (make_test_c1o)..."
clang -Wall -Wextra test_fixture/make_test_c1o.c -o build/make_test_c1o

echo "==> 生成包含 ARM64 重定位与函数调用的测试目标文件 (test_func.c1o)..."
./build/make_test_c1o build/test_func.c1o

echo "==> 使用 c1link-macho 链接生成最终 Mach-O 原生可执行文件..."
./build/c1link-macho build/linked_test_app my_entry build/test_func.c1o

echo "==> 检查二进制文件格式与代码签名..."
file build/linked_test_app
codesign -v build/linked_test_app && echo "代码签名校验: VALID"

echo "==> 运行链接生成的可执行文件..."
set +e
./build/linked_test_app
RET_VAL=$?
set -e

echo "程序退出码: $RET_VAL"
if [ $RET_VAL -eq 42 ]; then
    echo "[PASS] Step 01 Mach-O ARM64 链接器验证完全通过！"
else
    echo "[FAIL] 执行结果不符合预期 (预期 42，实际 $RET_VAL)"
    exit 1
fi
