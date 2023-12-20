# Continuous Desk

#### ��ֹԶ������

----
[English](README.md) / [����](README_CN.md)

![example](https://github.com/dijunkun/continuous-desk/assets/29698109/f3153e28-751e-477c-b4e3-ac2384d3370f)

## ���

Continuous Desk ��һ���������Ŀ�ƽ̨Զ��������������������û���ͬһʱ��Զ�̲ٿ�ͬһ̨���ԡ�������ͼ�����⣬����֧�ֶ˵��˵��������䣬��Զ������������ṩ�����Э��������

Continuous Desk �� [Projectx](https://github.com/dijunkun/projectx) ʵʱ����Ƶ������ʵ����Ӧ�á�Projectx ��һ���������Ŀ�ƽ̨ʵʱ����Ƶ����⡣����������͸����[RFC5245](https://datatracker.ietf.org/doc/html/rfc5245)������Ƶ��Ӳ����루H264������Ƶ����루[Opus](https://github.com/xiph/opus)���������������ӵ�����ƣ�[TCP over UDP](https://libnice.freedesktop.org/)���Ȼ���������


## ʹ��

�ڲ˵�����REMOTE ID��������Զ�������ID�������Connect�����ɷ���Զ�����ӡ�

![usage1](https://github.com/dijunkun/continuous-desk/assets/29698109/80099485-f2db-4f09-9fb2-e811d87265dc)

���Զ�������������������룬�򱾶�����д��ȷ������������ܳɹ�����Զ�����ӡ��������ʱ��״̬������֡�Incorrect password���澯��ʾ��

![incorrect password](https://github.com/dijunkun/continuous-desk/assets/29698109/0b16d21a-baa6-49c1-9d87-5e80978dd345)

���ӳɹ�������״̬�����С�ClientConnected�����������

![success](https://github.com/dijunkun/continuous-desk/assets/29698109/9c4f6604-2b1b-47b6-9c73-21c5026d1f26)

## ����

������
- [xmake](https://xmake.io/#/guide/installation)
- [cmake](https://cmake.org/download/)
- [vcpkg](https://vcpkg.io/en/getting-started)

Linux�������谲װ���°���

```
sudo apt-get install -y nvidia-cuda-toolkit libxcb-randr0-dev libxcb-xtest0-dev libxcb-xinerama0-dev libxcb-shape0-dev libxcb-xkb-dev libxcb-xfixes0-dev libxcb-shm0-dev libxv-dev libasound2-dev libsndio-dev libasound2-dev libpulse-dev
```

��������
```
git clone https://github.com/dijunkun/continuous-desk

cd continuous-desk

git submodule init 

git submodule update

xmake b remote_desk
```
����
```
# Windows/MacOS
xmake r remote_desk

# Linux����ʹ��rootȨ������
./remote_desk
```

## ���֤

Continuous Desk ʹ�� MIT ���֤������ʹ�õ��ĵ�����������������֤���зַ���