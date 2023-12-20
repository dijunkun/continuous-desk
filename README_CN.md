# Continuous Desk

#### ��ֹԶ������

----
[English](README.md) / [����](README_CN.md)

![sup-example](https://github.com/dijunkun/continuous-desk/assets/29698109/0a466cfa-776a-4339-91ec-cab7aeee5743)

## ���

Continuous Desk ��һ���������Ŀ�ƽ̨Զ��������������������û���ͬһʱ��Զ�̲ٿ�ͬһ̨���ԡ�������ͼ�����⣬����֧�ֶ˵��˵��������䣬��Զ������������ṩ�����Э��������

Continuous Desk �� [Projectx](https://github.com/dijunkun/projectx) ʵʱ����Ƶ������ʵ����Ӧ�á�Projectx ��һ���������Ŀ�ƽ̨ʵʱ����Ƶ����⡣����������͸����[RFC5245](https://datatracker.ietf.org/doc/html/rfc5245)������Ƶ��Ӳ����루H264������Ƶ����루[Opus](https://github.com/xiph/opus)���������������ӵ�����ƣ�[TCP over UDP](https://libnice.freedesktop.org/)���Ȼ���������


## ʹ��

�ڲ˵�����REMOTE ID��������Զ�������ID�������Connect�����ɷ���Զ�����ӡ�

![usage1](https://github.com/dijunkun/continuous-desk/assets/29698109/2ad59e6d-bdba-46d0-90cf-cbc9c06c2278)

���Զ�������������������룬�򱾶�����д��ȷ������������ܳɹ�����Զ�����ӡ��������ʱ��״̬������֡�Incorrect password���澯��ʾ��

![incorrect password](https://github.com/dijunkun/continuous-desk/assets/29698109/cb05501c-ec4e-4adf-952d-7a55ef770a97)

���ӳɹ�������״̬�����С�ClientConnected�����������

![success](https://github.com/dijunkun/continuous-desk/assets/29698109/0cca21f7-48fe-44a5-b83d-eafeb8a81eb1)

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