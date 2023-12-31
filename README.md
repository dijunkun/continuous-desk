# Continuous Desk

#### More than remote desktop

----
[中文](README_CN.md) / [English](README.md)

![sup_example](https://github.com/dijunkun/continuous-desk/assets/29698109/46536bc8-3ddd-438d-bf52-dccf143f1c20)

# Intro

Continuous Desk is a lightweight cross-platform remote desktop. It allows multiple users to remotely control the same computer at the same time. In addition to desktop image transmission, it also supports end-to-end voice transmission, providing collaboration capabilities on the basis of remote desktop.

Continuous Desk is an experimental application of [Projectx](https://github.com/dijunkun/projectx) real-time communications library. Projectx is a lightweight cross-platform real-time communications library. It has basic capabilities such as network traversal ([RFC5245](https://datatracker.ietf.org/doc/html/rfc5245)), video softwar/hardware encoding/decoding (H264), audio encoding/decoding ([Opus](https://github.com/xiph/opus)), signaling interaction, and network congestion control ([TCP over UDP](https://libnice.freedesktop.org/)).

## Usage

Enter the remote desktop ID in the 'REMOTE ID' field on the menu bar, and click 'Connect' button to initiate the remote connection.

![usage1](https://github.com/dijunkun/continuous-desk/assets/29698109/2ad59e6d-bdba-46d0-90cf-cbc9c06c2278)

If the remote desktop is set with a connection password, the local end needs to enter the correct password to initiate the remote connection. If the password is incorrect, an "Incorrect password" alert will appear in the status bar.

![incorrect password](https://github.com/dijunkun/continuous-desk/assets/29698109/cb05501c-ec4e-4adf-952d-7a55ef770a97)

After connection successfully established, the status bar will display the message "ClientConnected."

![success](https://github.com/dijunkun/continuous-desk/assets/29698109/0cca21f7-48fe-44a5-b83d-eafeb8a81eb1)

## How to build

Requirements:
- [xmake](https://xmake.io/#/guide/installation)
- [cmake](https://cmake.org/download/)
- [vcpkg](https://vcpkg.io/en/getting-started)

Following packages need to be installed on Linux:

```
sudo apt-get install -y nvidia-cuda-toolkit libxcb-randr0-dev libxcb-xtest0-dev libxcb-xinerama0-dev libxcb-shape0-dev libxcb-xkb-dev libxcb-xfixes0-dev libxcb-shm0-dev libxv-dev libasound2-dev libsndio-dev libasound2-dev libpulse-dev
```

Commands:
```
git clone https://github.com/dijunkun/continuous-desk

cd continuous-desk

git submodule init 

git submodule update

xmake b remote_desk
```
Run:
```
# Windows/MacOS
xmake r remote_desk

# root privileges are required on Linux
./remote_desk
```

## LICENSE

Continuous Desk is licenced under MIT, and some third-party libraries are distributed under their licenses.

