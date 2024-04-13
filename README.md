# Starry 

<img src="./src/resources/starry_1024x1024.png" width=64 height=64>

以划词弹窗的形式调用其他软件的命令行指令。如果你喜欢本软件，请给我一个免费的Star，谢谢～

![Last commit](https://img.shields.io/github/last-commit/ccslykx/starry?logo=git&logoColor=success&color=success&style=for-the-badge)
![Star](https://img.shields.io/github/stars/ccslykx/Starry?logo=github&logoColor=black&color=white&style=for-the-badge)

![Email](https://img.shields.io/badge/Outlook-ccslykx@outlook.com-0078D4?logo=microsoftoutlook&logoColor=0078D4&style=for-the-badge)

![PREVIEW](.assets/Starry_icon_preview.gif)

## 关于dev分支

dev分支实现了Windows、Mac、Wayland环境下的部分功能，因实现不完美，故一直没有同步到主分支。

## 最新重要更新

- 2023.7.27 图标功能基本完成
- 2023.7.24 代码重构

## Todo

1. 软件功能
    - [ ] 插件排序
    - [x] **插件图标**（2023.7.27）
    - [ ] 软件设置
    - [ ] 快捷键
    - [ ] 需要时间执行的脚本，转圈等待
    - [ ] 完善异常处理

2. 其他系统/环境支持
    - [ ] Wayland
    - [ ] Windows
    - [ ] MacOS


## 适用环境

![Qt](https://img.shields.io/badge/-Qt-brightgreen?logo=qt&logoColor=white)
![CMake](https://img.shields.io/badge/-CMake-064F8C?logo=cmake&logoColor=white)
![Linux](https://img.shields.io/badge/-Linux-orange?logo=linux&logoColor=white)

本软件基于`Qt6`开发，使用`CMake`构建，适用于基于`X11`的Linux桌面环境。


## 手动编译

### 安装依赖项（Debian 11 bullseye）

```bash
sudo apt update
sudo apt install git cmake g++ qt6-base-dev libqt6core6 libqt6widgets6 libqt6concurrent6 libqt6gui6 libx11-dev libxtst-dev
```

**Ubuntu 22.04 用户还需要安装以下依赖项**

```bash
sudo apt install libgl1-mesa-dev
```

### 编译Starry

```bash
git clone https://github.com/ccslykx/Starry.git

cd Starry && mkdir build && cd build

cmake .. && make
```


## 插件示例

- 创建新插件的办法：托盘菜单-设置-插件-创建新插件
- `$PLAINTEXT`表示选中的文本（在执行脚本处作为参数）

### 示例-复制到剪贴版

在“执行脚本”处填`starry copy2clipboard`。

### 示例-[Pot翻译](https://pot.pylogmon.com/)

在“执行脚本”处填`curl "127.0.0.1:60828/selection_translate"`。详见[Pot-外部调用](https://github.com/pot-app/pot-desktop?tab=readme-ov-file#%E5%A4%96%E9%83%A8%E8%B0%83%E7%94%A8)


## 致谢

- [Qt6](https://www.qt.io/product/qt6)
- [WHLUG/xrecord-example](https://github.com/WHLUG/xrecord-example)


## 反馈&交流

- [新建Issue](https://github.com/ccslykx/Starry/issues/new)
- 邮件：ccslykx@outlook.com
