#!/usr/bin/env bash
#
# test_all_examples.sh
# 一键编译并验证所有三大移植应用程序 (macOS Apple Silicon ARM64):
#   1. hello & args (基于自举自研 c1-arm64 编译器 + c1link-macho 链接器 + runtime)
#   2. minicurl (原生 POSIX Socket 网络抓取与文件落盘客户端)
#   3. mario (完整第一关马里奥游戏：自测、渲染烟雾测试导出BMP、AI全关卡自动通关)
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

mkdir -p build/examples

echo "========================================================================"
echo "           BitKindle macOS ARM64 - 应用程序全套回归验收测试"
echo "========================================================================"
echo ""

# ----------------------------------------------------------------------
# 1. Hello & Args (C1 自举工具链原生编译与运行)
# ----------------------------------------------------------------------
echo ">>> [1/3] 测试基于自研 C1-ARM64 编译的应用程序: hello & args ..."

echo "  -> 编译并链接 hello.c ..."
build/c1-arm64 src/examples/hello/hello.c build/examples/hello.c1o >/dev/null
build/c1link-macho build/examples/hello main build/runtime.c1o build/examples/hello.c1o >/dev/null

HELLO_OUT=$(./build/examples/hello)
if [ "$HELLO_OUT" != "hello" ]; then
    echo "[FAIL] hello 输出异常: $HELLO_OUT"
    exit 1
fi
echo "     [PASS] hello 输出: $HELLO_OUT"

echo "  -> 编译并链接 args.c (命令行参数解析) ..."
build/c1-arm64 src/examples/hello/args.c build/examples/args.c1o >/dev/null
build/c1link-macho build/examples/args main build/runtime.c1o build/examples/args.c1o >/dev/null

./build/examples/args "two words" "42"
echo "     [PASS] args 参数传递与逻辑校验通过 (返回码 0)"
echo ""

# ----------------------------------------------------------------------
# 2. minicurl (原生 POSIX 网络客户端)
# ----------------------------------------------------------------------
echo ">>> [2/3] 测试网络客户端应用程序: minicurl ..."
bash scripts/test_minicurl.sh
echo ""

# ----------------------------------------------------------------------
# 3. mario (超级马里奥兄弟 World 1-1 游戏引擎)
# ----------------------------------------------------------------------
echo ">>> [3/3] 测试独立游戏引擎: mario (超级马里奥兄弟 1-1) ..."

echo "  -> 编译原生马里奥引擎..."
clang -Wall -Wextra -I src/examples/mario src/examples/mario/mario.c -o build/examples/mario

echo "  -> [3.1] 运行 --self-test (跳跃、物理、碰撞、敌人、伤害、乌龟反弹)..."
./build/examples/mario --self-test

echo "  -> [3.2] 运行 --smoke-test (帧缓冲区渲染、状态机并导出 BMP 截图)..."
./build/examples/mario --smoke-test build/examples/mario_smoke.bmp

echo "  -> [3.3] 运行 --demo-test (AI 启发式寻路自动操作马里奥通关第一关)..."
./build/examples/mario --demo-test build/examples/mario_report.txt

FINAL_REPORT=$(tail -n 1 build/examples/mario_report.txt)
echo "     通关总结: $FINAL_REPORT"

echo ""
echo "========================================================================"
echo "  [SUCCESS] 恭喜！原仓库包含的全部三大应用程序移植完成并全数通过测试！"
echo "========================================================================"
