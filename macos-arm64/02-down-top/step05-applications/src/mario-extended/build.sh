#!/usr/bin/env bash
#
# mario-extended/build.sh
# 编译并启动原生 macOS Cocoa 交互式马里奥游戏
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

mkdir -p build

echo "==> 编译原生 macOS Cocoa 马里奥游戏 (mario_extended)..."
clang -Wall -Wextra -framework Cocoa -I . mario_extended.m -o build/mario_extended
echo "[SUCCESS] 编译完成: $DIR/build/mario_extended"

if [ "$1" = "--run" ] || [ "$1" = "-r" ]; then
    echo "==> 启动原生 macOS 游戏窗口..."
    ./build/mario_extended
elif [ "$1" = "--demo" ]; then
    echo "==> 启动 AI 自动通关演示窗口..."
    ./build/mario_extended --demo
else
    echo ""
    echo "运行指南："
    echo "  - 键盘亲自游玩: ./build/mario_extended"
    echo "  - 观看 AI 演示: ./build/mario_extended --demo"
    echo "  - 一键编译并运行: ./build.sh --run"
fi
