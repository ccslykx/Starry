# Starry

<img src="./src/resources/starry_1024x1024.png" width=64 height=64>

以划词弹窗的形式调用其他软件的命令行指令。如果你喜欢本软件，请给我一个免费的Star，谢谢～

![Last commit](https://img.shields.io/github/last-commit/ccslykx/starry?logo=git&logoColor=success&color=success&style=for-the-badge)
![Star](https://img.shields.io/github/stars/ccslykx/Starry?logo=github&logoColor=black&color=white&style=for-the-badge)

![Email](https://img.shields.io/badge/Outlook-ccslykx@outlook.com-0078D4?logo=microsoftoutlook&logoColor=0078D4&style=for-the-badge)

![PREVIEW](.assets/Starry_icon_preview.gif)

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
   - [x] MacOS


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
- `$PLAINTEXT`表示未经转换的选中文本，可以单独作为参数，也可以嵌入参数
- `$URLENCODED`表示经过 UTF-8 百分号编码的选中文本，拼接 URL 查询参数时应优先使用

Starry 会直接启动可执行程序并传递参数，不会通过 Shell 解释命令。因此 `|`、`>`、
`&&`、`$HOME`、`~` 和通配符等 Shell 语法不会自动生效。不要把不可信的选中文本传给
`sh -c`、`bash -c` 或其他会再次解释参数的程序。

### 示例-使用 Google 搜索

macOS：

```text
/usr/bin/open https://www.google.com/search?q=$URLENCODED
```

`open` 是 macOS 命令；Linux 和 Windows 需要使用对应平台的 URL 打开程序。

### 示例-复制到剪贴版

在“执行脚本”处填`starry copy2clipboard`。

### 示例-[Pot翻译](https://pot.pylogmon.com/)

在“执行脚本”处填`curl "127.0.0.1:60828/selection_translate"`。详见[Pot-外部调用](https://github.com/pot-app/pot-desktop?tab=readme-ov-file#%E5%A4%96%E9%83%A8%E8%B0%83%E7%94%A8)

## 调试日志

“设置 → 常规设置 → 开启调试模式”默认关闭。启用后，Starry 会在调试日志中输出选中文本和
占位符展开后的插件参数，便于排查命令配置；这些日志可能包含敏感信息，排查结束后建议关闭。

## 显示语言

可在“设置 → 常规设置 → 显示语言”中切换语言，更改会立即生效并自动保存。当前支持：

- 简体中文
- 繁體中文
- English
- Deutsch
- Français
- 日本語

首次运行会优先使用受支持的系统语言；如果系统语言不在上述列表中，则使用英语。


## 致谢

- [Qt6](https://www.qt.io/product/qt6)
- [WHLUG/xrecord-example](https://github.com/WHLUG/xrecord-example)


## 反馈&交流

- [新建Issue](https://github.com/ccslykx/Starry/issues/new)
- 邮件：ccslykx@outlook.com
