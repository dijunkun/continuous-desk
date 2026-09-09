# Windows 更新与安全软件兼容性

Windows 的更新按钮在后台下载发布信息中指定版本的安装包，完成校验及
Windows 附件安全检查后打开安装向导。客户端使用 WinHTTP，保留系统的
HTTPS 证书检查和代理配置，并用 `CrossDesk/<version> WindowsUpdater`
标识请求。

安装包保存在 `%LOCALAPPDATA%\CrossDesk\Updates` 下的独立目录中。
下载过程写入 `.part` 文件；HTTP 响应、文件长度、Windows 可执行文件头
均通过检查后才生成最终安装包。

客户端通过 Windows Attachment Services 的 `Save` 记录下载来源并请求
系统安全检查，再通过 `Execute` 打开安装包。这些 API 会按 Windows 策略
调用扫描、信任检查和必要的提示。安装程序由自身的管理员权限清单请求
UAC 授权。安全检查失败时停止流程，不回退到直接启动，也不删除来源标记。

## 发行版签名

应使用受信任的正式代码签名证书签署发布的主程序、服务、辅助程序、DLL
和最终安装包。只签安装包不能建立已安装主程序的发布者身份。
SignPath 的 `test-signing` 用于测试，不能代替公开发行所需的受信任签名。
当前流水线中的测试签名配置仍保持为可选项；正式策略需在取得证书及
SignPath 授权后配置。

检查实际分发文件的签名：

```powershell
Get-AuthenticodeSignature .\crossdesk.exe
Get-AuthenticodeSignature .\crossdesk-win-x64-<version>.exe
```

## 网络防护告警的处理

2026-09-09 的天擎日志记录了在域名解析阶段对
`downloads.crossdesk.cn` 和 `101.132.143.203` 的阻断。
该记录不能单独证明 HTTP 库、提权方式或文件内容是命中原因。
切换到标准 Windows API 也不能保证域名/IP 信誉告警消失。

出现同类告警时，保留告警时间、策略名称、域名/IP、程序版本及签名状态，
由安全管理员向厂商核查命中原因，同时检查服务器是否存在异常活动。
确认误报后按厂商流程纠正信誉或企业策略。应在策略允许的测试环境中
验证下载、取消、不完整文件、安全检查失败及安装向导启动流程。

相关 API：

- [WinHttpOpen](https://learn.microsoft.com/windows/win32/api/winhttp/nf-winhttp-winhttpopen)
- [IAttachmentExecute::Save](https://learn.microsoft.com/windows/win32/api/shobjidl_core/nf-shobjidl_core-iattachmentexecute-save)
- [IAttachmentExecute::Execute](https://learn.microsoft.com/windows/win32/api/shobjidl_core/nf-shobjidl_core-iattachmentexecute-execute)
