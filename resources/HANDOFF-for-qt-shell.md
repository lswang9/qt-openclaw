# 器灵视觉 → Qt 壳交接清单（v0.6）

给学习导师 / 器灵壳工程师接入 `qt-openclaw`。

## 口径（必须）
- **产品名**：器灵 Agent（仅窗口标题/关于页）
- **角色自称**：巴巴塔 / 银月（禁止自称「器灵」「助手」「AI」）
- **欢迎语示例**：`你好，我是巴巴塔～` / `你好，我是银月～`
- 非技术主界面：白话「已就绪」、聊天、换角色；技术细节进二级设置

## 根目录
`/cursor/stores/store-9d0a5cdd-04f9-4bd4-84ad-bbe652ffb280/qiling-design/`

## 巴巴塔（优先接入）
| 用途 | 路径 |
|------|------|
| 头像 | `characters/babata/avatar.jpg` |
| 整屏背景 | `characters/babata/bg-fullscreen.jpg` |
| 聊天壳示意（参考布局+文案） | `ui/hifi/chat-shell-babata.jpg` |

## 银月
| 用途 | 路径 |
|------|------|
| 头像 | `characters/yinyue/avatar.jpg` |
| 整屏背景 | `characters/yinyue/bg-fullscreen.jpg` |
| 聊天壳示意 | `ui/hifi/chat-shell-yinyue.jpg` |

## 换角色页
`ui/hifi/simple-characters-avatars.jpg`

## 图标（定形不定色，可选本周）
- 单色 template：`logo/glyph/glyph-template.png` / `glyph-template-white.png`
- macOS icns：`logo/exports/QiLing.icns`
- 多尺寸 PNG：`logo/exports/icon-16.png` … `icon-1024.png`

## 设计说明
- 命名口径：`brief/08-naming-and-official-art.md`
- 本周交付说明：`DELIVERY-v0.6.md`
- UI 受众：`brief/05-ui-audience.md`
- 色板/tokens（壳主题参考）：`ui/tokens-qml.md`

## 建议资源目录映射（Qt）
```
resources/characters/babata/{avatar,bg}.jpg
resources/characters/yinyue/{avatar,bg}.jpg
resources/app/icon.icns  ← 自 QiLing.icns
```
