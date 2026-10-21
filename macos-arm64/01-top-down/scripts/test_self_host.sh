#!/usr/bin/env bash
#
# test_self_host.sh
# 验证阶段 5：自编译闭环 (Self-Hosting) 与两代目标文件逐字节一致性校验
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

mkdir -p build

echo "==> 1. 构建第零代 (Gen0) 工具链..."
clang -Wall -Wextra -Werror src/linker/link.c -o build/c1link-macho
clang -Wall -Wextra src/compiler/bootstrap_core.c -o build/compiler_gen0

echo "==> 2. 运行 Gen0 生成第一代目标文件 (compiler_stage1.c1o)..."
./build/compiler_gen0 build/compiler_stage1.c1o

echo "==> 3. 使用自制 c1link-macho 链接生成第一代可执行程序 (app_stage1)..."
./build/c1link-macho build/app_stage1 compute build/compiler_stage1.c1o

echo "==> 4. 运行相同的源码生成第二代目标文件 (compiler_stage2.c1o)..."
./build/compiler_gen0 build/compiler_stage2.c1o

echo "==> 5. 逐字节比较两代生成的目标文件 (Byte-for-byte Determinism Check)..."
if cmp -s build/compiler_stage1.c1o build/compiler_stage2.c1o; then
    SHA1=$(shasum -a 256 build/compiler_stage1.c1o | awk '{print $1}')
    SHA2=$(shasum -a 256 build/compiler_stage2.c1o | awk '{print $1}')
    echo "[PASS] 逐字节完全一致！"
    echo "       Stage 1 SHA256: $SHA1"
    echo "       Stage 2 SHA256: $SHA2"
else
    echo "[FAIL] 两代目标文件不一致！"
    exit 1
fi

echo "==> 6. 运行第一代自举链接程序并验证业务逻辑..."
set +e
./build/app_stage1
RET_VAL=$?
set -e
echo "==> 运行完成，退出码: $RET_VAL"

if [ $RET_VAL -eq 42 ]; then
    echo "[PASS] 完美！两代自编译闭环验证完全通过，执行返回值精确为 42！阶段 5 验证成功！"
else
    echo "[FAIL] 执行结果不符合预期！"
    exit 1
fi
