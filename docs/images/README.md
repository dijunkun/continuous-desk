# README 截图 / Screenshot notes

[中文 README](../../README.md) · [English README](../../README_EN.md)

## 来源 / Sources

- 生成日期：2026-09-07。
- 桌面 UI 源代码：`abc8f907`（`ios-support` 分支），Slint 1.17.1，macOS arm64；截图工具增加了可选的文档演示数据。
- 桌面图片来自 [`slint_ui_smoke_test`](../../apps/desktop/tests/slint_ui_smoke_test.cpp) 的 UI 捕获模式，直接渲染仓库中的 Slint 组件；不是旧版本截图或重新绘制的界面。
- 主窗口展示“已连接服务器”和三台在线设备的完整示例状态。ID `123 456 789`、密码 `123456`、连接状态及设备记录均为演示数据，不是真实设备凭据或真实远程会话。示例设置不代表应用运行时默认值。
- 设备缩略图使用可公开的页面截图：`fixtures/source.png` 来自 [CrossDesk 的 Xmake 源码页](https://github.com/kunkundi/crossdesk/blob/ios-support/xmake.lua)，`fixtures/website.png` 来自 [CrossDesk 官网](https://www.crossdesk.cn/)，第三张复用 `web-client.png`。它们用于展示缩略图效果，不代表这些页面正在对应的远程电脑上运行。
- `web-client.png`：2026-09-07 在 [web.crossdesk.cn](https://web.crossdesk.cn/) 截取的连接首页，视口为 1280 × 720，没有发起远程连接。

Desktop images render the actual Slint components with an illustrative connected state and three populated recent-device cards. The thumbnails are public browser-page captures, not private remote desktops or evidence of live sessions. IDs, device names, statuses, and settings are demonstration data. The Web image captures the public connection page. Native iOS screenshots are not included because this update did not capture a signed app running on a physical device.

## 文件 / Files

| 文件 | 页面 | 语言 |
| --- | --- | --- |
| `desktop-main-zh.png` / `desktop-main-en.png` | Main window / 主窗口 | 中文 / English |
| `desktop-settings-zh.png` / `desktop-settings-en.png` | Settings / 设置 | 中文 / English |
| `desktop-self-hosted-zh.png` / `desktop-self-hosted-en.png` | Self-hosting / 自托管设置 | 中文 / English |
| `web-client.png` | Web connection page / Web 连接首页 | 中文 |

桌面图片为 1280 × 900 PNG，使用 2× 缩放保持文字清晰。Markdown 只设置展示宽度，保持原始比例。设置列表可滚动，截图仅展示首屏。演示配置显示硬件编解码器可用；实际可用性由平台、设备与构建决定。

## 重新生成 / Regenerate

以下命令在仓库根目录、已完成依赖配置的 macOS 开发环境运行。`sips` 仅将 Slint 导出的 PPM 无损转换为 PNG，不修改 UI。其他平台可以使用支持 PPM 的图片格式转换工具。

```bash
set -e
xmake b -y slint_ui_smoke_test
mkdir -p build/readme-capture docs/images

for locale in zh en; do
  language=0
  if [ "$locale" = en ]; then language=1; fi
  for page in main settings self-hosted; do
    snapshot="$PWD/build/readme-capture/desktop-$page-$locale.ppm"
    SLINT_BACKEND=winit-software SLINT_SCALE_FACTOR=2 \
      CROSSDESK_UI_CAPTURE=1 \
      CROSSDESK_UI_CAPTURE_DEMO_ASSETS="$PWD/docs/images" \
      CROSSDESK_UI_CAPTURE_PAGE="$page" \
      CROSSDESK_UI_CAPTURE_LANGUAGE="$language" \
      CROSSDESK_UI_CAPTURE_SNAPSHOT="$snapshot" \
      xmake r slint_ui_smoke_test
    sips -s format png "$snapshot" \
      --out "docs/images/desktop-$page-$locale.png"
  done
done
```

`CROSSDESK_UI_CAPTURE_DEMO_ASSETS` 必须指向包含上述图片的目录。缺失图片会让捕获命令失败，避免再次导出空白缩略图；不设置该变量时，仍保留原有测试状态。

捕获模式兼容 `/tmp/crossdesk-ui-capture-page` 和 `/tmp/crossdesk-ui-capture-language` 两个旧覆盖文件；如存在，它们会优先于环境变量，运行前请检查。更新后逐张检查文字、布局、敏感信息和文档引用，并同步修改本页日期与源码版本。
