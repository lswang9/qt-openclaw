# 器灵 Agent（qt-openclaw）

Qt 桌面壳：启停本地 OpenClaw Gateway，并在窗口内用 **Qt WebEngine** 加载官方 Control UI。

## 壳职责边界

| 壳做 | 壳不做 |
|------|--------|
| 启停 Gateway（`openclaw --profile qiling`，状态 **`~/.openclaw-qiling`**，端口 **19889**） | 手写 `openclaw.json` / fork OpenClaw 源码 |
| 内嵌 Control UI → `http://127.0.0.1:19889/`（QWebEngineView） | 系统浏览器外开；自绘聊天主界面（可保留次要） |
| Settings → 官方 CLI `models auth paste-api-key` | 直连 `api.deepseek.com` |
| 连上时 `plugins install/enable` 主题包 `qiling-brand` | 旁路组装 systemPrompt |

## 试用三步

1. **填 Key**：打开 App →「设置」→ 粘贴 DeepSeek Key → 保存  
2. **连上**：点「连上」→ `openclaw --profile qiling gateway run --port 19889`  
   - 同时安装/启用 `resources/control-ui-themes`（插件 id `qiling-brand`）  
   - 成功后窗口内加载 Control UI  
3. **换主题**：Control UI → **Settings → Appearance** 选 `qiling-brand/babata` 或 `qiling-brand/yinyue`  
   - 改主题包后：`openclaw --profile qiling plugins reload qiling-brand`

## 主题包

路径：`github:lswang9/qiling-brand `（brandName=「器灵」）。详见包内 `README.md`。

## Build

需要带 **Qt6 WebEngineWidgets** 的 Qt（Conan Center `qt/6.7.3` **没有** `qtwebengine=True` 预编译包；请用 Homebrew / aqt / 官方安装器）。

```bash
# 示例：Homebrew Qt
brew install qtbase qtdeclarative qtwebchannel qtpositioning qtwebsockets qtwebengine
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix qtwebengine);$(brew --prefix qtdeclarative)" -DCMAKE_BUILD_TYPE=Release
cmake --build build
open "build/器灵 Agent.app"
```

产物：`/Users/wangyupeng/Desktop/dev/qt-openclaw/build/器灵 Agent.app`

## License

MIT © 2026 lswang9
