# 阶段 5：三大完整移植应用程序验收 (Step 05 - Applications)

## 概述

在自举工具链与运行时系统就绪后，本阶段作为登顶之作，完成了原仓库全部三大完整应用程序在 macOS Apple Silicon (Darwin ARM64) 上的移植与回归验收。

---

## 应用程序清单与移植解析

### 1. `hello` 与命令行参数 `args`
- **路径**: `src/hello/hello.c`, `src/hello/args.c`
- **技术要点**:
  - 由自研 `c1-arm64` 编译生成目标文件，通过 `c1link-macho` 链接 `runtime.c1o`；
  - 启动桩在调用 `main(argc, argv)` 前通过栈维护传递 dyld 传入的命令行参数寄存器（`x0` 与 `x1`）。

### 2. `minicurl` 网络客户端
- **路径**: `src/minicurl/minicurl.c`
- **技术要点**:
  - 原版使用 Windows 专有 WinHTTP API；
  - 移植版采用标准 BSD/POSIX Socket 接口 (`socket`, `connect`, `send`, `recv`)；
  - 完全保持原版命令行接口（`minicurl -o <file> <url>`）与退出码规范。

### 3. `mario` 超级马里奥兄弟 World 1-1
- **路径**: `src/mario/mario.c`, `src/mario/demo.h`
- **技术要点**:
  - **100% 保持原版游戏引擎与物理常数**（砖块、问号箱、蘑菇状态、乌龟与龟壳碰撞）；
  - **解耦 Win32 GDI 依赖**，内置 960x544 32位 RGBA 内存 FrameBuffer 与 BMP 导出；
  - 支持三大模式：
    - `--self-test`: 物理和游戏机制自测（返回值 0）；
    - `--smoke-test <out.bmp>`: 渲染第一关起始画面并导出 BMP 高清截图；
    - `--demo-test <report.txt>`: 启发式束搜索 AI 自动操作马里奥通关第一关（历时 1634 帧，无伤夺旗）。

---

## 运行验证

```bash
chmod +x test_all.sh
./test_all.sh
```
