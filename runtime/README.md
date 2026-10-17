# runtime / 运行时

Place a local OpenClaw + Node layout here so **Local Gateway** mode can start without a global install.

在此目录放入 OpenClaw（及可选的 Node），以便应用以 **本地 Gateway** 模式启动，无需全局安装。

## Quick setup (China mirror) / 快速安装（国内镜像）

```bash
# from repo root — uses https://registry.npmmirror.com by default
./scripts/setup-runtime.sh

# optional override
NPM_MIRROR=https://registry.npmmirror.com ./scripts/setup-runtime.sh
```

Do **not** use the official `registry.npmjs.org` if you are in CN and it times out.
国内请勿默认使用官方源；本脚本默认走 npmmirror。

## Expected layout

```
runtime/
  bin/openclaw          # executable CLI (script or symlink)
  node_modules/openclaw # npm package (gitignored)
  bin/node              # optional bundled Node
```

The Qt launcher also accepts:

- `OPENCLAW_RUNTIME` env pointing at this folder
- `openclaw` / `node` already on `PATH`

`node_modules/` is gitignored — do not commit it.
