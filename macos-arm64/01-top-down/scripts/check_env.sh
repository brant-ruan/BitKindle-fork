#!/usr/bin/env bash
#
# check_env.sh
# 检查当前 macOS 环境是否满足 BitKindle ARM64 移植与构建要求
#

set -e

echo "=== BitKindle macOS (Darwin ARM64) 环境检查 ==="

# 1. 检查操作系统与架构
OS="$(uname -s)"
ARCH="$(uname -m)"
echo "[-] 操作系统: $OS"
echo "[-] CPU 架构: $ARCH"

if [ "$OS" != "Darwin" ]; then
    echo "[!] 错误: 该移植分支仅支持 macOS (Darwin)。当前为: $OS"
    exit 1
fi

if [ "$ARCH" != "arm64" ]; then
    echo "[!] 警告: 当前架构不是 arm64 (可能是 Intel x86_64 或 Rosetta 仿真环境)。"
    echo "    若在 Apple Silicon 终端上运行，请确保未勾选 '使用 Rosetta 打开'。"
fi

# 2. 检查基础宿主编译工具链（用于路线 B 前期阶段构建自举编译器）
echo -n "[-] 检查 clang: "
if command -v clang >/dev/null 2>&1; then
    CLANG_VER=$(clang --version | head -n 1)
    echo "OK ($CLANG_VER)"
else
    echo "MISSING (请安装 Xcode Command Line Tools: xcode-select --install)"
    exit 1
fi

# 3. 检查代码签名工具 codesign
echo -n "[-] 检查 codesign: "
if command -v codesign >/dev/null 2>&1; then
    echo "OK"
else
    echo "MISSING"
    exit 1
fi

# 4. 检查 Mach-O 分析工具 otool
echo -n "[-] 检查 otool: "
if command -v otool >/dev/null 2>&1; then
    echo "OK"
else
    echo "MISSING"
    exit 1
fi

echo ""
echo "[PASS] 宿主环境检查通过，可以进行 macos-arm64 工具链移植与构建！"
