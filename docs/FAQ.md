# 常见问题 / FAQ

[返回 README](../README.md) · [English](#english)

## 未连接服务器，或提示 TLS 证书错误

先确认网络可用、两端使用预期的服务器配置。自托管环境检查信令端口、证书有效期、主机名匹配和系统信任；详见 [自托管指南](SELF_HOSTING.md)。这一步解决后，再排查远程设备和 P2P 连接。

## 已连接服务器，但远程设备离线

确认被控端 CrossDesk 仍在运行，双方连接同一个信令服务，并重新复制被控端当前的本机 ID。切换服务器后，原来的 ID 和连接记录可能不再适用。

## 对等连接失败

在 **☰ → 设置** 中勾选 **启用中继服务**，点击“确认”后重新连接。自托管用户还需检查 Coturn 是否正常运行，以及中继端口 TCP/UDP、媒体端口范围 UDP 是否放通。

公共服务的兼容性变更也会影响旧客户端；请核对 [Release 说明](https://github.com/kunkundi/crossdesk/releases)，尽量让两端使用兼容的近期版本。

## macOS 有连接但黑屏，或无法操作键鼠

在“系统设置 → 隐私与安全性”中检查 CrossDesk 的屏幕录制与辅助功能权限。更换应用位置或签名、更新开发构建后，可能需要重新授权并重启应用。

## Linux Wayland 无法捕获画面

Wayland 画面捕获依赖启用了 Wayland 支持的构建、宿主系统 PipeWire 0.3 和桌面门户授权；无法捕获时也可在 X11 会话中对比验证。构建选项见 [Linux 构建说明](BUILD.md#linux)。

## Windows 锁屏后无法操作

在被控电脑检查 CrossDesk Service 是否安装、是否运行，并保持 CrossDesk 客户端运行。便携版可在“设置 → 锁屏控制服务”安装。服务状态检查命令见 [Windows 服务说明](../README.md#windows-service)。

## 修改密码后连接失败

修改密码时使用 6 位数字或英文字母，并保持连接服务器。等待修改成功、客户端重新连接后，重新复制当前密码。控制端保存的旧密码会失效，按弹窗输入新密码再连接。

## 收到的文件在哪里

桌面端见 **☰ → 设置 → 文件保存路径**。iOS 原生端收到的文件保存在应用的 `Documents/Received`，可以从分享入口导出；更多说明见 [iOS 开发文档](../apps/ios/README.md)。

## 关闭主窗口后为什么还能连接

关闭主窗口会隐藏窗口，客户端仍在后台运行。需要结束程序时，从系统托盘或 macOS 菜单栏中选择退出。

## Windows 没有安装 CUDA，如何编译

可以先使用 `--USE_CUDA=false` 构建。需要 CUDA 编解码支持时，按 [构建指南](BUILD.md) 安装对应依赖；PowerShell 中设置环境变量应使用 `$env:CUDA_PATH = "实际安装目录"`。

## iOS 构建产物为什么不能直接安装

当前 CI 导出的是未签名应用。需要使用自己的开发签名 / 分发方式安装；也可用 Xcode 打开工程并选择真机运行。见 [iOS 构建要求](../apps/ios/README.md)。

---

<a id="english"></a>

## English

[Back to README](../README_EN.md)

| Symptom | What to check |
| --- | --- |
| Server disconnected / TLS error | Network access, signaling endpoint, certificate dates, hostname, and system trust. See [self-hosting](SELF_HOSTING_EN.md). |
| Remote device offline | Keep the host running, connect both sides to the same signaling server, and copy its current ID again. |
| P2P connection failed | Enable TURN relay in Settings and reconnect. For self-hosting, check Coturn and relay/media firewall ports. Review [release compatibility notes](https://github.com/kunkundi/crossdesk/releases). |
| Black screen / no input on macOS | Grant Screen Recording and Accessibility, then reopen the app. A changed app path or signature may require fresh permission grants. |
| No capture on Linux Wayland | Check the Wayland build option, host PipeWire 0.3, and desktop portal permission. Compare with an X11 session. See [Linux build instructions](BUILD_EN.md#linux). |
| Cannot control a Windows lock screen | Install/start CrossDesk Service and keep the host client running. See [service commands](../README_EN.md#windows-service). |
| Saved password no longer works | Use 6 ASCII letters or digits when changing the password, with the host connected to the server. Wait for the change and reconnection to complete, then copy the current password and enter it on the controller. |
| Cannot find received files | Check Settings → File Save Path on desktop. Native iOS stores them in `Documents/Received` and provides a share action. |
| Main window closed but app still running | Closing hides the window. Use the tray/menu-bar exit command to quit. |
| Windows build without CUDA | Build with `--USE_CUDA=false`, or follow the [CUDA build instructions](BUILD_EN.md). Use `$env:CUDA_PATH` in PowerShell. |
| iOS artifact cannot be installed directly | CI exports an unsigned app. Sign it yourself or build and run from Xcode on a physical device. See [iOS requirements](../apps/ios/README.md). |

If the issue persists, [open an issue](https://github.com/kunkundi/crossdesk/issues) with both OS versions, client versions, connection mode, and reproduction steps. Remove passwords and private device information from logs or screenshots before sharing them.
