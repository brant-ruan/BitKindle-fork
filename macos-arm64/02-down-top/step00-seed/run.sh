#!/usr/bin/env bash
#
# step00-seed/run.sh
# 验证阶段 0：纯十六进制机器码种子解码与回显验证
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

mkdir -p build

echo "==> [Step 00] 解码机器码种子 src/seed-arm64.hex..."
./decode_seed.sh src/seed-arm64.hex build/seed-arm64

echo "==> 运行种子程序并测试十六进制解码管道..."
# 传入 "Hello BitKindle!" 的十六进制编码: 48 65 6c 6c 6f 20 42 69 74 4b 69 6e 64 6c 65 21 0a
OUT=$(echo "48 65 6c 6c 6f 20 42 69 74 4b 69 6e 64 6c 65 21 0a" | ./build/seed-arm64)
echo "种子解码输出: $OUT"

if [ "$OUT" = "Hello BitKindle!" ]; then
    echo "[PASS] Step 00 纯机器码种子验证完全通过！"
else
    echo "[FAIL] 种子输出不匹配！"
    exit 1
fi
