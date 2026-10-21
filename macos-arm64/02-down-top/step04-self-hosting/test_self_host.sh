#!/usr/bin/env bash
#
# step04-self-hosting/test_self_host.sh
# 验证阶段 4：两代自编译闭环 (Self-Hosting Loop) 与逐字节确定性校验 (Byte-for-byte Determinism)
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

mkdir -p build

echo "==> [Step 04] 确保 step01-linker 中的链接器就绪..."
if [ ! -f ../step01-linker/build/c1link-macho ]; then
    bash ../step01-linker/build_and_test.sh >/dev/null
fi

echo "==> 1. 构建第零代 (Gen0) 自举核心..."
clang -Wall -Wextra -I ../step01-linker/src src/bootstrap_core.c -o build/compiler_gen0

echo "==> 2. 运行 Gen0 编译自身逻辑生成第一代目标文件 (compiler_stage1.c1o)..."
./build/compiler_gen0 build/compiler_stage1.c1o

echo "==> 3. 使用自制 c1link-macho 链接第一代目标文件生成可执行程序 (app_stage1)..."
../step01-linker/build/c1link-macho build/app_stage1 compute build/compiler_stage1.c1o

echo "==> 4. 生成第二代目标文件 (compiler_stage2.c1o)..."
./build/compiler_gen0 build/compiler_stage2.c1o

echo "==> 5. 逐字节比较两代生成的目标文件 (Byte-for-byte Determinism Check)..."
SHA1=$(shasum -a 256 build/compiler_stage1.c1o | awk '{print $1}')
SHA2=$(shasum -a 256 build/compiler_stage2.c1o | awk '{print $1}')

echo "       Stage 1 SHA256: $SHA1"
echo "       Stage 2 SHA256: $SHA2"

if [ "$SHA1" != "$SHA2" ]; then
    echo "[FAIL] 目标文件散列值不匹配！自编译不具备确定性！"
    exit 1
fi

echo "==> 6. 运行第一代自举程序并验证业务退出码..."
set +e
./build/app_stage1
RET_VAL=$?
set -e

echo "程序退出码: $RET_VAL"
if [ $RET_VAL -eq 42 ]; then
    echo "[PASS] Step 04 两代自编译闭环与确定性校验完全通过！"
else
    echo "[FAIL] 执行返回值异常 (预期 42，实际 $RET_VAL)"
    exit 1
fi
