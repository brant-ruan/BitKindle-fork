#!/usr/bin/env bash
#
# decode_seed.sh
# 将纯文本十六进制机器码种子解码为原生可执行二进制，并执行本地代码签名。
# 对应原版 Windows 上的 base/decode_seed.ps1。
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HEX_FILE="${1:-$DIR/src/base/seed-arm64.hex}"
OUT_FILE="${2:-$DIR/build/seed-arm64}"

mkdir -p "$(dirname "$OUT_FILE")"

echo "[-] 解码机器码种子: $HEX_FILE -> $OUT_FILE"

# 过滤注释与空白，解码十六进制字节
grep -v '^#' "$HEX_FILE" | tr -d ' \n\r\t' | xxd -r -p > "$OUT_FILE"

chmod +x "$OUT_FILE"

# 对生成的原生 ARM64 二进制进行本地签名
codesign -s - --force "$OUT_FILE" >/dev/null 2>&1 || true

echo "[SUCCESS] 成功从纯十六进制种子构建原生种子程序: $OUT_FILE ($(wc -c < "$OUT_FILE" | tr -d ' ') 字节)"
