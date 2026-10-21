#!/usr/bin/env bash
#
# 02-down-top/build-all.sh
# 阶梯式自举演进一键流水线 (Step 00 -> Step 05)
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "================================================================="
echo "   BitKindle 02-down-top 阶梯自举演进总流水线 (Step 00 ~ Step 05)"
echo "================================================================="

echo ""
echo ">>> [Step 00/05] 纯十六进制机器码种子 (step00-seed)..."
bash step00-seed/run.sh

echo ""
echo ">>> [Step 01/05] 独立 64 位 Mach-O 链接器与代码签名 (step01-linker)..."
bash step01-linker/build_and_test.sh

echo ""
echo ">>> [Step 02/05] 原生 ARM64 C1 递归下降编译器后端 (step02-compiler)..."
bash step02-compiler/build_and_test.sh

echo ""
echo ">>> [Step 03/05] 自包含 Darwin 运行库与标准 I/O (step03-runtime)..."
bash step03-runtime/build_and_test.sh

echo ""
echo ">>> [Step 04/05] 两代自编译闭环与确定性校验 (step04-self-hosting)..."
bash step04-self-hosting/test_self_host.sh

echo ""
echo ">>> [Step 05/05] 三大完整移植应用程序验收 (step05-applications)..."
bash step05-applications/test_all.sh

echo ""
echo "================================================================="
echo " [SUCCESS] 恭喜！02-down-top 全套阶梯式演进验证顺利通关！"
echo " 从纯机器码种子一步步跃迁至完整应用程序，自举天梯全部点亮！"
echo "================================================================="
