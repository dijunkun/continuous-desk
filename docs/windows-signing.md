# Windows 签名与 Release 测试

GitHub Actions 的 `Build and Release` 工作流构建 CrossDesk，调用 SignPath 签名，再发布已验证的 Windows 文件。

## 触发方式

| 触发方式 | 签名策略 | 输出 |
| --- | --- | --- |
| 普通分支 push，或未勾选签名的手动运行 | 无 | 开发构建工件 |
| 手动运行，勾选 `sign_windows_test` | `test-signing` | 测试证书签名的 Actions 工件 |
| 手动运行，勾选 `sign_windows_release` | `release-signing` | 正式证书签名的 Actions 工件 |
| push `v*` tag | `release-signing` | 验证通过后发布 GitHub Release 和下载站 |

两种手动签名选项不能同时启用。手动运行可勾选 `windows_only` 跳过其他平台；即使选择 tag 手动运行，也不会发布 Release、更新 `latest` 或同步下载站。

## SignPath 配置

GitHub 仓库需要 Secret `SIGNPATH_API_TOKEN` 和 Variable `SIGNPATH_ORGANIZATION_ID`。SignPath 项目 slug 为 `crossdesk`，已关联 GitHub.com Trusted Build System。

该工作流明确使用以下 Artifact Configuration：

| Slug | XML 文件 | 用途 |
| --- | --- | --- |
| `initial` | `apps/desktop/scripts/windows/signpath-installer.xml` | 现有安装包配置，只签外层 NSIS EXE |
| `windows-binaries` | `apps/desktop/scripts/windows/signpath-binaries.xml` | 签 ZIP 内四个 CrossDesk 自有二进制 |

`crossdesk` 后台已添加并验证 `windows-binaries`。新环境需要在项目的 Artifact Configurations 页面创建同名配置，使用仓库内对应的完整 XML。保留 `initial` 的默认配置和现有审批、来源验证要求。XML 文件仅提交到 GitHub 不会自动同步到 SignPath 后台。

两种策略都必须允许提交这些配置。正式策略需要有效的 Release 证书、提交者权限和审批人。若证书仍为 `CSR PENDING`，需要 SignPath 完成签发/导入；切换工作流参数不会使证书生效。后台的来源限制也必须允许当前测试分支，若不允许，应按项目原有发布审批流程处理。

## 签名顺序

1. 构建安装版，将 `crossdesk.exe`、`crossdesk_service.exe`、`crossdesk_session_helper.exe`、`wgc_plugin.dll` 打成 `installer-binaries.zip`。
2. 使用 `windows-binaries` 签名，验证四个文件后替换构建输出。
3. NSIS 打包已签名的文件，使用 `initial` 签名安装器，验证后替换安装器。
4. 构建便携版，将四个文件打成 `portable-binaries.zip`，同样签名并验证，然后生成包含已签名文件的便携 ZIP。
5. 上传最终安装器与便携 ZIP。tag 发布等待 Windows 签名和其他平台构建全部成功。

第三方 DLL 原样打包，不提交签名。外层 NSIS 签名不为内部文件添加独立签名；本流程在打包前签名自有文件。NSIS 生成的 `uninstall.exe` 尚未配置独立签名。

每次运行会按上述顺序提交三个签名请求。正式签名需要在 SignPath 中逐个审批，每个请求最多等待 60 分钟，整个 Windows job 最多 240 分钟。请求链接由官方 Action 输出，并在可用时写入运行摘要。

## 启动正式证书测试

在 GitHub → Actions → Build and Release → Run workflow 中选择测试分支：

- `sign_windows_release`: true
- `sign_windows_test`: false
- `windows_only`: true

也可在分支推送后运行：

```sh
gh workflow run build.yml --ref signpath-release-signing \
  -f sign_windows_release=true -f sign_windows_test=false -f windows_only=true
```

正式验证要求 Authenticode 状态为 `Valid`、存在时间戳，并且 `signtool verify /pa /all /v` 返回成功。测试证书允许 `NotTrusted`，但拒绝无签名、哈希不匹配等状态。任一签名或验证失败均停止后续发布；不会回退到未签名发布。

原始签名输入保留为中间 Actions 工件。手动测试成功后，下载名为 `crossdesk-win-x64-...` 和 `crossdesk-win-x64-portable-...` 的最终工件，不要使用 `installer-binaries.zip`、`portable-binaries.zip` 等原始签名输入。

## 本地校验

```sh
pwsh -NoProfile -File apps/desktop/tests/windows_signing_test.ps1
actionlint .github/workflows/build.yml
```

PowerShell 测试使用模拟证书状态检查归档范围、还原顺序及失败行为，不能替代 Windows 上的真实 Authenticode 验证。

参考：[SignPath GitHub 集成](https://docs.signpath.io/trusted-build-systems/github)、[Artifact Configuration](https://docs.signpath.io/artifact-configuration/examples)、[SignTool 验证](https://learn.microsoft.com/en-us/windows/win32/seccrypto/using-signtool-to-verify-a-file-signature)。
