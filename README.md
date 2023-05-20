# Starry 

<img src="./src/resources/starry.svg" width=64 height=64>

以划词弹窗的形式调用其他软件的命令行指令。如果你喜欢本软件，请给我一个免费的Star，谢谢～

![Last commit](https://img.shields.io/github/last-commit/ccslykx/starry?logo=git&logoColor=success&color=success&style=for-the-badge)
![Star](https://img.shields.io/github/stars/ccslykx/Starry?logo=github&logoColor=black&color=white&style=for-the-badge)

![Email](https://img.shields.io/badge/Outlook-ccslykx@outlook.com-0078D4?logo=microsoftoutlook&logoColor=0078D4&style=for-the-badge)


## 适用环境

![Qt](https://img.shields.io/badge/-Qt-brightgreen?logo=qt&logoColor=white)
![CMake](https://img.shields.io/badge/-CMake-064F8C?logo=cmake&logoColor=white)
![Linux](https://img.shields.io/badge/-Linux-orange?logo=linux&logoColor=white)

本软件基于`Qt6`开发，使用`CMake`构建，适用于基于`X11`的Linux桌面环境。


## 手动编译

### 安装依赖项（Debian 11 bullseye）

```bash
apt update
apt install git cmake g++ qt6-base-dev libqt6core6 libqt6widgets6 libqt6concurrent6 libqt6gui6 libx11-dev libxtst-dev
```

编辑`/usr/include/X11/extensions/record.h`文件

```
vim /usr/include/X11/extensions/record.h
```

找到`#include <X11/extensions/recordconst.h>`，在下面一行添加`#include <X11/Xlib.h>`

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

在“执行脚本”处填`pot popclip $PLAINTEXT`。


## Todo

- [ ] 插件排序
- [ ] 插件图标
- [ ] 增加软件细节设置
- [ ] 快捷键
- [ ] 增加默认插件功能
- [ ] 需要时间执行的脚本，转圈等待
- [ ] 完善异常处理


## 致谢

- [Qt6](https://www.qt.io/product/qt6)
- [WHLUG/xrecord-example](https://github.com/WHLUG/xrecord-example)


## 反馈&交流

- [新建Issue](https://github.com/ccslykx/Starry/issues/new)
- 邮件：ccslykx@outlook.com