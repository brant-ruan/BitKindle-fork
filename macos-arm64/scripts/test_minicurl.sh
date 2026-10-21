#!/usr/bin/env bash
#
# test_minicurl.sh
# 验证移植版 minicurl 命令行客户端在本地 HTTP 服务器上的抓取与文件保存功能
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

mkdir -p build/examples/test_server

echo "==> 1. 编译 minicurl 原生程序..."
clang -Wall -Wextra src/examples/minicurl/minicurl.c -o build/examples/minicurl

echo "==> 2. 准备本地测试数据并启动临时 HTTP 服务器..."
PORT=18432
TEST_CONTENT="Hello BitKindle HTTP Transport! This is a test file for minicurl on macOS ARM64."
echo "$TEST_CONTENT" > build/examples/test_server/test.txt

# 启动后台 Python HTTP 服务器
python3 -m http.server $PORT --directory build/examples/test_server >/dev/null 2>&1 &
SERVER_PID=$!

cleanup() {
    kill $SERVER_PID >/dev/null 2>&1 || true
}
trap cleanup EXIT

# 稍等服务器启动
sleep 1

echo "==> 3. 使用 minicurl 下载网页内容并保存到文件 (-o)..."
./build/examples/minicurl -o build/examples/output.txt "http://127.0.0.1:$PORT/test.txt"

echo "==> 4. 校验下载的文件内容..."
DOWNLOADED=$(cat build/examples/output.txt)
echo "下载内容: $DOWNLOADED"

if [ "$DOWNLOADED" = "$TEST_CONTENT" ]; then
    echo "[PASS] 完美！minicurl 抓取与保存内容与源数据完全一致！"
else
    echo "[FAIL] 内容不匹配！"
    exit 1
fi
