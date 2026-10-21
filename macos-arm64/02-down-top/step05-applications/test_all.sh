#!/usr/bin/env bash
#
# step05-applications/test_all.sh
# 验证三大移植应用程序：hello/args、minicurl、mario (Darwin ARM64)
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

mkdir -p build/test_server

echo "==> [Step 05] 确保前序自举工具链与运行时就绪..."
if [ ! -f ../step01-linker/build/c1link-macho ]; then
    bash ../step01-linker/build_and_test.sh >/dev/null
fi
if [ ! -f ../step02-compiler/build/c1-arm64 ]; then
    bash ../step02-compiler/build_and_test.sh >/dev/null
fi
if [ ! -f ../step03-runtime/build/runtime.c1o ]; then
    bash ../step03-runtime/build_and_test.sh >/dev/null
fi

# ----------------------------------------------------------------------
# 1. hello & args (C1 自举工具链原生编译与运行)
# ----------------------------------------------------------------------
echo ">>> [1/3] 验证自制 C1 工具链原生应用程序: hello & args ..."
../step02-compiler/build/c1-arm64 -o build/hello.c1o src/hello/hello.c >/dev/null
../step01-linker/build/c1link-macho build/hello main ../step03-runtime/build/runtime.c1o build/hello.c1o >/dev/null
HELLO_OUT=$(./build/hello)
echo "    hello 输出: $HELLO_OUT"

../step02-compiler/build/c1-arm64 -o build/args.c1o src/hello/args.c >/dev/null
../step01-linker/build/c1link-macho build/args main ../step03-runtime/build/runtime.c1o build/args.c1o >/dev/null
./build/args "two words" "42"
echo "    [PASS] hello & args 运行与命令行参数解析完全正常！"

# ----------------------------------------------------------------------
# 2. minicurl (原生 POSIX Socket 网络客户端)
# ----------------------------------------------------------------------
echo ">>> [2/3] 验证网络客户端: minicurl ..."
clang -Wall -Wextra src/minicurl/minicurl.c -o build/minicurl
PORT=18520
TEST_MSG="BitKindle POSIX Socket Network Transfer OK on Darwin ARM64!"
echo "$TEST_MSG" > build/test_server/data.txt

python3 -m http.server $PORT --directory build/test_server >/dev/null 2>&1 &
SERVER_PID=$!
cleanup() { kill $SERVER_PID >/dev/null 2>&1 || true; }
trap cleanup EXIT
sleep 1

./build/minicurl -o build/downloaded.txt "http://127.0.0.1:$PORT/data.txt"
DOWNLOADED=$(cat build/downloaded.txt)
echo "    minicurl 下载内容: $DOWNLOADED"
if [ "$DOWNLOADED" = "$TEST_MSG" ]; then
    echo "    [PASS] minicurl 抓取与保存内容完全一致！"
else
    echo "    [FAIL] 内容不匹配！"
    exit 1
fi

# ----------------------------------------------------------------------
# 3. mario (超级马里奥兄弟 World 1-1 游戏引擎)
# ----------------------------------------------------------------------
echo ">>> [3/3] 验证游戏引擎: mario (超级马里奥兄弟 World 1-1) ..."
clang -Wall -Wextra -I src/mario src/mario/mario.c -o build/mario

echo "    -> 运行物理引擎与碰撞自测 (--self-test)..."
./build/mario --self-test

echo "    -> 运行 FrameBuffer 渲染与 BMP 导出 (--smoke-test)..."
./build/mario --smoke-test build/mario_smoke.bmp

echo "    -> 运行 AI 寻路通关演示 (--demo-test)..."
./build/mario --demo-test build/mario_report.txt

REPORT=$(tail -n 1 build/mario_report.txt)
echo "    通关报告: $REPORT"

echo ""
echo "[PASS] Step 05 全部三大移植应用程序测试通过！"
