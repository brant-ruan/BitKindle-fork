# Super Mario Bros World 1-1 Native macOS Interactive Game (Mario Extended)

## Overview

`mario-extended` provides the **native interactive GUI window edition** of the Super Mario Bros World 1-1 game engine on macOS Apple Silicon (Darwin ARM64).

To preserve the 100% headless automated test capability of the baseline `mario/` example, the interactive window implementation is maintained independently within `mario-extended/`:
- **Zero Third-Party Dependencies**: Written purely in C and Objective-C linking system-native `-framework Cocoa` (no SDL, GLFW, or external packages required);
- **Smooth 60 FPS Native Rendering**: Dispatches frame ticks through an `NSTimer` and renders the internal framebuffer directly to an `NSWindow` via CoreGraphics;
- **Full Keyboard Controls**: High-fidelity key tracking with responsive retro platformer physics;
- **Dual Visual Modes**: Play interactively with the keyboard, or launch a live window to watch the heuristic beam-search AI clear the entire level.

---

## Keyboard Controls

| Key | Action |
| :--- | :--- |
| **`A`** or **`←` (Left Arrow)** | Move Left |
| **`D`** or **`→` (Right Arrow)** | Move Right |
| **`Space`** or **`W`** or **`↑` (Up Arrow)** | Jump |
| **`Shift`** (Hold) | Dash / Run |
| **`P`** | Pause / Resume |
| **`R`** | Restart Level |
| **`ESC`** or close window button | Exit Game |

---

## Build & Run Guide

From within this directory, use the provided runner script:

```bash
# 1. Interactive Keyboard Play (Launches native macOS window)
./build.sh --run

# 2. Watch AI Live Demo (Launches window and lets AI play through World 1-1)
./build.sh --demo

# 3. Compile Only
./build.sh
./build/mario_extended          # Play interactively
./build/mario_extended --demo   # Watch AI
```

---

## Headless CLI Options

`mario_extended` retains all headless automated test features from the baseline version:

```bash
./build/mario_extended --self-test                 # Headless physics & collision self-test
./build/mario_extended --smoke-test preview.bmp    # Render & export BMP screenshot
./build/mario_extended --demo-test report.txt      # Run AI completion and export text report
```
