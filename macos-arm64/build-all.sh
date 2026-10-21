#!/usr/bin/env bash
#
# macos-arm64/build-all.sh
# BitKindle macOS (Darwin ARM64) 顶层统合构建与双轨全流程验收入口
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "=========================================================================="
echo "          BitKindle macOS (Apple Silicon ARM64) 顶层一键验收总管线"
echo "               Model Attribution: Gemini 3.8 Flash (Medium)"
echo "=========================================================================="
echo ""

echo ">>> [Track 1/2] 运行 01-top-down 工业级自举工具链与应用整体集成测试..."
bash 01-top-down/build-all.sh

echo ""
echo ">>> [Track 2/2] 运行 02-down-top 从零到N阶梯自举演进全流程测试..."
bash 02-down-top/build-all.sh

echo ""
echo "=========================================================================="
echo " [SUCCESS] 恭喜！BitKindle macOS ARM64 双轨体系全线告捷，全部验收通过！"
echo " 1. 01-top-down: 完整工业级编译器、链接器与应用全量流水线通过"
echo " 2. 02-down-top: Step 00 至 Step 05 阶梯演进天梯全部点亮通过"
echo "=========================================================================="
