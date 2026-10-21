#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

mkdir -p build

echo "==> 1. 编译纯 Mach-O 生成器原型..."
clang -Wall -Wextra src/prototype/macho_arm64_seed.c -o build/macho_arm64_seed

echo "==> 2. 运行生成器，纯代码组装生成原生 Mach-O ARM64 可执行程序..."
./build/macho_arm64_seed build/hello_arm64

echo "==> 3. 对生成的原生二进制执行 macOS ad-hoc 代码签名..."
codesign -s - --force build/hello_arm64

echo "==> 4. 检查生成的二进制文件元数据..."
file build/hello_arm64
otool -hv build/hello_arm64
otool -lv build/hello_arm64 | head -n 35

echo "==> 5. 直接执行生成的 ARM64 二进制并捕获退出码..."
set +e
./build/hello_arm64
EXIT_CODE=$?
set -e

echo "==> 6. 执行完成！进程退出码: $EXIT_CODE"
if [ $EXIT_CODE -eq 0 ]; then
    echo "[PASS] 恭喜！原生 Mach-O + ARM64 裸二进制生成与执行验证完全成功！"
else
    echo "[FAIL] 退出码异常: $EXIT_CODE"
    exit 1
fi
