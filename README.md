# qt-openclaw

中文 | English

这是 **OpenClaw Gateway** 的 Qt 桌面壳（Widgets 客户端骨架）。Agent / harness 仍外置；本仓库只负责 UI 与 Gateway WebSocket 连接层的占位实现。

This is a Qt shell for the **OpenClaw Gateway**. The agent harness stays external; this repo scaffolds the desktop UI and a stub Gateway WebSocket client.

## Build / 构建

### Prerequisites

- CMake ≥ 3.21
- C++17 compiler
- [Conan 2](https://docs.conan.io/) (`pip install conan` or package manager)

```bash
conan profile detect
conan install . --output-folder=build --build=missing -s build_type=Release
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

> **Note:** The first Conan Qt build is **very slow** (building Qt from source if no binary package matches). Plan accordingly.

### Optional: system Qt

If you already have Qt 6 from the Qt Online Installer, you can skip Conan and point CMake at it:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/macos -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Layout

| Path | Role |
|------|------|
| `conanfile.py` | Conan 2 deps (`qt/6.7.3`, shared/LGPL-friendly) |
| `CMakeLists.txt` | Qt Widgets app |
| `src/MainWindow.*` | Gateway URL, token, connect, log pane |
| `src/GatewayClient.*` | Stub WS client (log-only for now) |

## License

MIT © 2026 lswang9 / 王立盛
