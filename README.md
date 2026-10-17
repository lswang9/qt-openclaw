# qt-openclaw（器灵）

中文 | English

Qt 桌面壳 **器灵**：本地启动 OpenClaw Gateway、把 DeepSeek API Key 写入 Gateway 配置，聊天经 **本地 Gateway**（HTTP `/v1/chat/completions`），**不**从 Qt 直连 `api.deepseek.com`。

Qt desktop shell (**器灵**): start local OpenClaw Gateway, configure DeepSeek **into** the Gateway, chat via **local Gateway** HTTP — not direct DeepSeek from the client.

角色自称默认 **巴巴塔**（可选 **银月**）。「器灵」仅为产品/App 名，不进 system prompt 人设自称与欢迎语。

## Features / 功能

| Feature | Notes |
|---------|--------|
| **Local Gateway** | Start/stop `openclaw gateway run --port 18789` (`LocalGatewayProcess`); injects `DEEPSEEK_API_KEY` |
| **OpenClaw config sync** | On Settings Save / Start Gateway: write `~/.openclaw` (see below) |
| **Chat** | `GatewayClient` → `POST http://127.0.0.1:18789/v1/chat/completions` with Gateway token |
| **Persona** | Roles 巴巴塔 / 银月; welcome「你好，我是巴巴塔～」 |
| **Settings** | DeepSeek key + model + role + Gateway token in `QSettings`; key synced into OpenClaw |

Default local URL: `ws://127.0.0.1:18789` (HTTP chat on the same port).

### Chat path (important)

```
Qt UI ──HTTP──► Local OpenClaw Gateway ──► DeepSeek API
         /v1/chat/completions
```

- Qt **does not** call `https://api.deepseek.com`.
- DeepSeek key is stored for Gateway via `OpenClawConfigSync` + process env.
- Legacy `DeepSeekClient` remains in tree but is **demoted** (not on the hot path).

### How config is written (`OpenClawConfigSync`)

| Path | What |
|------|------|
| `~/.openclaw/.env` | `DEEPSEEK_API_KEY=…` |
| `~/.openclaw/agents/main/agent/auth-profiles.json` | profile `deepseek:manual` (`type: api_key`) |
| `~/.openclaw/openclaw.json` | `agents.defaults.model`, `agents.entries.main.identity.name`, `plugins.entries.deepseek.enabled`, `gateway.http.endpoints.chatCompletions.enabled`, `auth.profiles.deepseek:manual` |
| `~/.openclaw/workspace/IDENTITY.md` | Name = role (巴巴塔 / 银月) |

Also on Start Gateway: child process env gets `DEEPSEEK_API_KEY` and `OPENCLAW_HOME`.

### Self-intro copy locations

| Location | Content |
|----------|---------|
| `src/Persona.h` | `welcomeMessage`, `systemPrompt`, `identityMarkdown` |
| Chat empty/welcome bubble | MainWindow ctor → `Persona::welcomeMessage(role)` |
| Gateway agent identity | `openclaw.json` → `agents.entries.main.identity` + `IDENTITY.md` |
| Per-request system message | `GatewayClient` history system role from `Persona::systemPrompt` |

Product window title uses 「器灵」; assistant bubbles use the role name.

## Build / 构建

### Prerequisites

- CMake ≥ 3.21
- C++17 compiler
- [Conan 2](https://docs.conan.io/)

```bash
export PATH="/Users/wangyupeng/.conan2/p/cmake6cfb420416116/p/CMake.app/Contents/bin:$HOME/Library/Python/3.9/bin:$PATH"

conan profile detect
conan install . --output-folder=build --build=missing -s build_type=Release
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binary: `build/qt-openclaw`

## Local Gateway runtime

Discovery order: `OPENCLAW_RUNTIME` → `./runtime/bin/openclaw` → `openclaw` on `PATH` → `node` + `runtime/node_modules/openclaw/dist/entry.js`.

```bash
./scripts/setup-runtime.sh   # npmmirror by default
```

```bash
openclaw gateway run --port 18789 --verbose --allow-unconfigured
```

## Settings / 模型与角色

1. **Settings…** — paste DeepSeek API key (written into Gateway, not used for direct client HTTP).
2. Model default: `deepseek-flash` → stored as `deepseek/deepseek-flash` in OpenClaw.
3. Role: **巴巴塔** (default) or **银月**.
4. Optional Gateway token (else read from `openclaw.json` `gateway.auth.token`).

## Layout

| Path | Role |
|------|------|
| `src/MainWindow.*` | Mode, gateway, chat UI |
| `src/LocalGatewayProcess.*` | `QProcess` launcher + env |
| `src/GatewayClient.*` | Gateway HTTP chat (+ WS connect stub log) |
| `src/OpenClawConfigSync.*` | Write `~/.openclaw` for DeepSeek + persona |
| `src/Persona.h` | Role / welcome / system prompt |
| `src/SettingsDialog.*` | Key / model / role / token |
| `src/DeepSeekClient.*` | Legacy direct HTTP (demoted) |

## License

MIT © 2026 lswang9 / 王立盛
