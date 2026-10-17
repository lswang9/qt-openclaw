# 器灵 Agent（qt-openclaw）

Qt 桌面壳：启停本地 OpenClaw Gateway + 聊天 UI + 换角色展示。

## 壳职责边界

| 壳做 | 壳不做 |
|------|--------|
| 启停 Gateway（`openclaw --profile qiling`，状态 **`~/.openclaw-qiling`**，端口 **19889**） | 手写 `openclaw.json` / `.env` / `IDENTITY.md` / `auth-profiles`；不把 `OPENCLAW_HOME` 指到默认 `~/.openclaw` |
| 聊天 UI → `POST http://127.0.0.1:19889/v1/chat/completions` | 在壳内注入 system/人设/工具/会话引擎逻辑 |
| 换角色展示（头像/背景/欢迎语，读 `qiling-agents`） | 旁路实现引擎能力；知识包组装（见配置仓 `PLUGIN.md`） |
| Settings 调官方 CLI `models auth paste-api-key` | 直连 `api.deepseek.com` |

人设与知识包：由 **OpenClaw 插件 / Gateway** 挂载 `third_party/qiling-agents`（contract-v2）。壳只读 UI 字段。

## 与官方隔离

| 项 | 官方默认 | 本壳 |
|----|----------|------|
| 端口 | 18789 | **19889** |
| 目录 | `~/.openclaw`（LaunchAgent `ai.openclaw.gateway`） | **`~/.openclaw-qiling`**（`--profile qiling`） |

## Submodule

```bash
git submodule update --init
# 钉点：v0.3.1 / contract-v2（ed0f3a6）
```

## 首屏

启动为设计稿「splash-babata-ready」：整屏背景 + 圆头像 +「已就绪」+ 欢迎语 +「开始聊天」+ 右上齿轮。点「开始聊天」进入聊天壳。

## 试用三步

1. **填 Key**：打开 App →「设置」→ 粘贴 DeepSeek Key → 保存  
   （内部：`openclaw --profile qiling models auth paste-api-key --provider deepseek`）  
   与已安装 `ai.openclaw.gateway`（`~/.openclaw`）隔离，避免 divergent state。
2. **连上**：点「连上」→ `openclaw --profile qiling gateway run --port 19889`。
3. **说话**：输入消息发送；点「换角色」切换巴巴塔/银月展示。

## Build

```bash
export PATH="$HOME/.conan2/p/cmake6cfb420416116/p/CMake.app/Contents/bin:$HOME/Library/Python/3.9/bin:$PATH"
conan install . --output-folder=build --build=missing -s build_type=Release
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
open "build/器灵 Agent.app"
```

产物（可双击）：`/Users/wangyupeng/Desktop/dev/qt-openclaw/build/器灵 Agent.app`  
（构建后自动跑 `macdeployqt` 打包 Qt 依赖。）

Runtime（国内）：`./scripts/setup-runtime.sh`

## License

MIT © 2026 lswang9
