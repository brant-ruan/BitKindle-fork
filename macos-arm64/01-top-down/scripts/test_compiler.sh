#!/usr/bin/env bash
#
# test_compiler.sh
# 验证阶段 3：ARM64 C 语言编译器 (c1-arm64) 的端到端编译与链接执行
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

mkdir -p build

echo "==> 1. 编译 c1link-macho 链接器与 c1-arm64 编译器..."
clang -Wall -Wextra -Werror src/linker/link.c -o build/c1link-macho
clang -Wall -Wextra -Werror src/compiler/c1_arm64.c -o build/c1-arm64

echo "==> 2. 准备测试 C 源码 (包含循环、算术、多参数函数调用)..."
cat << 'EOF' > build/test_calc.c
int add_numbers(int a, int b) {
    return a + b;
}

int compute_sum(int n) {
    int sum = 0;
    int i = 1;
    while (i <= n) {
        sum = add_numbers(sum, i);
        i = i + 1;
    }
    return sum;
}

int main() {
    int total = compute_sum(10); /* 55 */
    if (total > 50) {
        return total - 13; /* 55 - 13 = 42 */
    } else {
        return 0;
    }
}
EOF

echo "==> 3. 使用 c1-arm64 编译 C 源码..."
./build/c1-arm64 -o build/test_calc.c1o build/test_calc.c

echo "==> 4. 使用自制 c1link-macho 链接目标文件生成 Mach-O ARM64 可执行程序..."
./build/c1link-macho build/test_calc_app main build/test_calc.c1o

echo "==> 5. 检查生成的可执行程序元数据与反汇编节..."
file build/test_calc_app
otool -tv build/test_calc_app | head -n 45

echo "==> 6. 执行编译生成的二进制文件并检验退出码..."
set +e
./build/test_calc_app
RET_VAL=$?
set -e

echo "==> 程序运行结束，退出码 (Exit Code): $RET_VAL"
if [ $RET_VAL -eq 42 ]; then
    echo "[PASS] 完美！c1-arm64 编译器正确解析并生成 ARM64 机器指令，运行计算结果精确为 42！阶段 3 验证完全成功！"
else
    echo "[FAIL] 计算结果异常: 预期 42，实际 $RET_VAL"
    exit 1
fi
