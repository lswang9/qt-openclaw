# qt-openclaw

中文 | English

Qt 桌面壳，面向 **OpenClaw Gateway**：本地启动 Gateway 进程、远程 WS 连接（占位）、以及 **DeepSeek 直连聊天**（MVP）。

Qt desktop shell for **OpenClaw Gateway**: local Gateway process lifecycle, remote WS stub, and **DeepSeek direct chat** (MVP).

## Features / 功能（MVP）

| Feature | Notes |
|---------|--------|
| **Local Gateway** | Start/stop `openclaw gateway run --port 18789` via `QProcess` (`LocalGatewayProcess`) |
| **Remote Gateway** | URL + token fields; WS client still stub |
| **Chat** | Message list + input; calls **DeepSeek** `POST /chat/completions` directly (OpenAI-compatible). **Not** via Gateway yet |
| **Settings** | DeepSeek API key / base URL / model in `QSettings` (local only; never committed) |

Default local WS URL: `ws://127.0.0.1:18789` (OpenClaw default port).

### Chat path (important)

MVP chat uses **DeepSeek HTTP API directly** (`https://api.deepseek.com`), not OpenClaw Gateway protocol. Local Gateway mode is for process lifecycle / future WS integration.

## Build / 构建

### Prerequisites

- CMake ≥ 3.21
- C++17 compiler
- [Conan 2](https://docs.conan.io/) (`pip install conan` or package manager)

```bash
export PATH="/Users/wangyupeng/.conan2/p/cmake6cfb420416116/p/CMake.app/Contents/bin:$HOME/Library/Python/3.9/bin:$PATH"
# adjust cmake path if your Conan package hash differs

conan profile detect
conan install . --output-folder=build --build=missing -s build_type=Release
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binary (after build): `build/qt-openclaw`

> First Conan Qt build can be **very slow**.

### Optional: system Qt

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/macos -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Local Gateway runtime / 本地 Gateway 运行时

The app looks for OpenClaw in this order:

1. `OPENCLAW_RUNTIME` env → `bin/openclaw`
2. `./runtime/bin/openclaw` (next to binary / repo)
3. `openclaw` on `PATH`
4. `node` + `runtime/node_modules/openclaw/dist/entry.js`

### Populate `runtime/` (China mirror)

官方 npm 源在国内常不可用。请用 **npmmirror**：

```bash
./scripts/setup-runtime.sh
# default registry: https://registry.npmmirror.com
# override: NPM_MIRROR=https://registry.npmmirror.com ./scripts/setup-runtime.sh
```

Or manually:

```bash
mkdir -p runtime && cd runtime
npm install openclaw@latest --registry=https://registry.npmmirror.com --prefix .
```

Do **not** commit `runtime/node_modules/`. See `runtime/README.md`.

If you already have a global install (`openclaw` on PATH), Local mode works without `runtime/`.

Foreground command used by the app:

```bash
openclaw gateway run --port 18789 --verbose --allow-unconfigured
```

## DeepSeek settings / 模型配置

1. Open **Settings…** in the app (first send also prompts if key missing).
2. Paste API key from https://platform.deepseek.com
3. Defaults: base URL `https://api.deepseek.com`, model `deepseek-flash`
4. Stored in `QSettings` org/app `qt-openclaw` — **never commit secrets**

## Layout

| Path | Role |
|------|------|
| `conanfile.py` | Conan 2 deps (`qt/6.7.3`) |
| `CMakeLists.txt` | Qt Widgets app |
| `src/MainWindow.*` | Mode, gateway controls, chat, log |
| `src/LocalGatewayProcess.*` | `QProcess` launcher for local Gateway |
| `src/GatewayClient.*` | Stub WS client |
| `src/DeepSeekClient.*` | Direct chat completions HTTP client |
| `src/SettingsDialog.*` | DeepSeek key / URL / model |
| `scripts/setup-runtime.sh` | npm install OpenClaw via npmmirror |
| `runtime/` | Optional bundled CLI (node_modules gitignored) |

## License

MIT © 2026 lswang9 / 王立盛
