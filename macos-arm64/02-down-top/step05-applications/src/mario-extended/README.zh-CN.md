# 超级马里奥兄弟 World 1-1 原生 macOS 交互式游戏 (Mario Extended)

## 概述

`mario-extended` 是原版马里奥游戏在 macOS Apple Silicon (Darwin ARM64) 上的**原生图形窗口交互版**。

为了确保原版 `mario/` 保持 100% 纯粹的无头自动化测试特性，本项目将原生窗体交互实现独立置于 `mario-extended/` 中：
- **零第三方库依赖**：纯 C 与 Objective-C 编写，直接链接系统内置的 `-framework Cocoa`，无需安装 SDL、GLFW 等任何外部依赖；
- **原生 60 FPS 流畅渲染**：通过 `NSTimer` 与 CoreGraphics 硬件渲染引擎直接把内存帧缓冲区绘制到 `NSWindow` 视图中；
- **全键盘实时交互**：完美捕获键盘按下与抬起事件，还原经典红白机手感；
- **双模支持**：既支持玩家键盘亲自操纵游玩，也支持弹窗实时观摩 AI 启发式自动通关。

---

## 游戏操作键位 (Controls)

| 按键 (Key) | 功能说明 (Action) |
| :--- | :--- |
| **`A`** 或 **`←` (方向键左)** | 向左移动 (Move Left) |
| **`D`** 或 **`→` (方向键右)** | 向右移动 (Move Right) |
| **`空格 (Space)`** 或 **`W`** 或 **`↑` (方向键上)** | 跳跃 (Jump) |
| **`Shift`** (按住) | 加速奔跑 (Run / Dash) |
| **`P`** | 暂停 / 继续游戏 (Pause / Resume) |
| **`R`** | 重新开始游戏 (Restart) |
| **`ESC`** 或 直接点击窗口红叉 | 退出游戏 (Quit) |

---

## 编译与启动指南

进入当前目录后，运行配套脚本即可编译并启动：

```bash
# 1. 亲自键盘游玩 (启动原生 Mac 游戏窗口)
./build.sh --run

# 2. 观看 AI 实时通关演示 (启动窗口由 AI 操纵马里奥跑完全关)
./build.sh --demo

# 3. 仅编译
./build.sh
./build/mario_extended          # 亲自游玩
./build/mario_extended --demo   # AI 自动游玩
```

---

## 命令行高级选项 (Headless Modes)

`mario_extended` 同时兼备原版的无头自动化验收功能：

```bash
./build/mario_extended --self-test                 # 运行物理与碰撞引擎无头自测
./build/mario_extended --smoke-test preview.bmp    # 无头渲染并导出游戏画面 BMP 截图
./build/mario_extended --demo-test report.txt      # 运行 AI 寻路通关并生成文本评测报告
```
