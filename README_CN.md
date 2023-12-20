# Continuous Desk

#### 不止远程桌面

----
[English](README.md) / [中文](README_CN.md)

![sup-example](https://github.com/dijunkun/continuous-desk/assets/29698109/0a466cfa-776a-4339-91ec-cab7aeee5743)

## 简介

Continuous Desk 是一个轻量级的跨平台远程桌面软件。它允许多个用户在同一时间远程操控同一台电脑。除桌面图像传输外，它还支持端到端的语音传输，在远程桌面基础上提供额外的协作能力。

Continuous Desk 是 [Projectx](https://github.com/dijunkun/projectx) 实时音视频传输库的实验性应用。Projectx 是一个轻量级的跨平台实时音视频传输库。它具有网络透传（[RFC5245](https://datatracker.ietf.org/doc/html/rfc5245)），视频软硬编解码（H264），音频编解码（[Opus](https://github.com/xiph/opus)），信令交互，网络拥塞控制（[TCP over UDP](https://libnice.freedesktop.org/)）等基础能力。


## 使用

在菜单栏“REMOTE ID”处输入远端桌面的ID，点击“Connect”即可发起远程连接。

![usage1](https://github.com/dijunkun/continuous-desk/assets/29698109/2ad59e6d-bdba-46d0-90cf-cbc9c06c2278)

如果远端桌面设置了连接密码，则本端需填写正确的连接密码才能成功发起远程连接。密码错误时，状态栏会出现“Incorrect password”告警提示。

![incorrect password](https://github.com/dijunkun/continuous-desk/assets/29698109/cb05501c-ec4e-4adf-952d-7a55ef770a97)

连接成功建立后，状态栏会有“ClientConnected”相关字样。

![success](https://github.com/dijunkun/continuous-desk/assets/29698109/0cca21f7-48fe-44a5-b83d-eafeb8a81eb1)

## 编译

依赖：
- [xmake](https://xmake.io/#/guide/installation)
- [cmake](https://cmake.org/download/)
- [vcpkg](https://vcpkg.io/en/getting-started)

Linux环境下需安装以下包：

```
sudo apt-get install -y nvidia-cuda-toolkit libxcb-randr0-dev libxcb-xtest0-dev libxcb-xinerama0-dev libxcb-shape0-dev libxcb-xkb-dev libxcb-xfixes0-dev libxcb-shm0-dev libxv-dev libasound2-dev libsndio-dev libasound2-dev libpulse-dev
```

编译命令
```
git clone https://github.com/dijunkun/continuous-desk

cd continuous-desk

git submodule init 

git submodule update

xmake b remote_desk
```
运行
```
# Windows/MacOS
xmake r remote_desk

# Linux下需使用root权限运行
./remote_desk
```

## 许可证

Continuous Desk 使用 MIT 许可证，其中使用到的第三方库根据自身许可证进行分发。