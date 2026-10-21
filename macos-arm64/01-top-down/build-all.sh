#!/usr/bin/env bash
#
# build-all.sh
# BitKindle macOS (Darwin arm64) 一键总构建与全流程自举验证入口
# 对应原版 Windows 下的 build-all.ps1
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "================================================================="
echo "        BitKindle for macOS (Apple Silicon ARM64) 一键总构建"
echo "================================================================="

# 1. 环境检查
echo ""
echo "[Step 1/7] 执行 macOS Apple Silicon 环境检查..."
./scripts/check_env.sh

# 2. 从纯机器码种子解码生成种子程序
echo ""
echo "[Step 2/7] 从纯十六进制机器码种子 (seed-arm64.hex) 解码构建..."
./scripts/decode_seed.sh src/base/seed-arm64.hex build/seed-arm64
echo "48 65 6c 6c 6f 20 42 69 74 4b 69 6e 64 6c 65 21 0a" | ./build/seed-arm64

# 3. 构建核心工具链
echo ""
echo "[Step 3/7] 构建自制工具链 (c1link-macho 链接器 + c1-arm64 编译器)..."
mkdir -p build
clang -Wall -Wextra -Werror src/linker/link.c -o build/c1link-macho
clang -Wall -Wextra -Werror src/compiler/c1_arm64.c -o build/c1-arm64
echo "[SUCCESS] 工具链编译就绪: build/c1link-macho, build/c1-arm64"

# 4. 构建原生 Darwin 运行库
echo ""
echo "[Step 4/7] 生成 macOS 原生 Darwin 运行时 (runtime.c1o)..."
clang -Wall -Wextra src/runtime/build_runtime.c -o build/build_runtime
./build/build_runtime build/runtime.c1o

# 5. 执行各阶段单元与集成验收测试
echo ""
echo "[Step 5/7] 运行全套阶段自动化回归测试..."
echo "--- [5.1] Mach-O 链接器测试 (test_linker.sh) ---"
./scripts/test_linker.sh >/dev/null && echo "[PASS] 链接器测试通过"

echo "--- [5.2] 编译器后端指令生成测试 (test_compiler.sh) ---"
./scripts/test_compiler.sh >/dev/null && echo "[PASS] 编译器测试通过"

echo "--- [5.3] 运行时与控制台标准 I/O 应用程序测试 (test_runtime.sh) ---"
./scripts/test_runtime.sh

# 6. 两代自编译一致性校验
echo ""
echo "[Step 6/7] 执行自编译闭环 (Self-Hosting) 与两代目标文件逐字节一致性校验..."
./scripts/test_self_host.sh

# 7. 移植应用程序全套验收 (hello, minicurl, mario)
echo ""
echo "[Step 7/7] 执行三套完整移植应用程序自动化测试 (hello/args, minicurl, mario)..."
./scripts/test_all_examples.sh

echo ""
echo "================================================================="
echo " [SUCCESS] 恭喜！BitKindle macOS (Darwin ARM64) 移植全流程验收成功！"
echo " 1. 纯十六进制机器码种子解码运行正常"
echo " 2. 原生 64 位 Mach-O 链接器 (c1link-macho) 运行正常"
echo " 3. 原生 ARM64 C 语言编译器 (c1-arm64) 运行正常"
echo " 4. 自包含 Darwin 运行时 (runtime-darwin) 与控制台 I/O 正常"
echo " 5. 两代目标文件逐字节一致性 (Byte-for-byte Determinism) 校验通过"
echo " 6. 原仓库三大完整应用程序 (hello/args, minicurl, mario) 全数通过测试"
echo "================================================================="
