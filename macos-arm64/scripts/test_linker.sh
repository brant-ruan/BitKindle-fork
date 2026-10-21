#!/usr/bin/env bash
#
# test_linker.sh
# 验证阶段 2：Mach-O 链接器 (c1link-macho) 对 .c1o 目标文件的链接与运行
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

mkdir -p build

echo "==> 1. 编译 c1link-macho 链接器..."
clang -Wall -Wextra -Werror src/linker/link.c -o build/c1link-macho

echo "==> 2. 编译测试目标文件生成器..."
clang -Wall -Wextra src/prototype/make_test_c1o.c -o build/make_test_c1o

echo "==> 3. 生成测试 ARM64 .c1o 目标文件..."
./build/make_test_c1o build/test_func.c1o

echo "==> 4. 使用自制 c1link-macho 链接目标文件生成原生 Mach-O 可执行程序..."
./build/c1link-macho build/linked_test_app my_entry build/test_func.c1o

echo "==> 5. 检查生成的 Mach-O 二进制信息..."
file build/linked_test_app
otool -hv build/linked_test_app
otool -lv build/linked_test_app | head -n 35

echo "==> 6. 运行链接出的可执行程序并验证其返回值..."
set +e
./build/linked_test_app
RET_VAL=$?
set -e

echo "==> 程序运行结束，返回值 (Exit Code): $RET_VAL"
if [ $RET_VAL -eq 42 ]; then
    echo "[PASS] 完美！目标函数 my_entry 正确返回了 42，Mach-O 链接器阶段 2 验证完全成功！"
else
    echo "[FAIL] 返回值异常: 预期 42，实际 $RET_VAL"
    exit 1
fi
