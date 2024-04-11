## Switch本地媒体播放器

非常感谢[xfangfang](https://github.com/xfangfang)贡献的[wiliwili](https://github.com/xfangfang/wiliwili)开源项目。本项目源码大部分基于该项目，如果有遇到跨平台编译问题请参考wiliwili项目的CMake文件。
目前仅支持Switch。

## 安装

#### NintendoSwitch

1. [nxplayer-release](https://github.com/xfangfang/wiliwili/releases)下载对应版本`NRO`文件.
2. 放置`SD卡/switch`目录下.
3. 在主页打开任意游戏按住`R`键直至进入`hbmenu`,运行`nxplayer`

## 编译

推荐使用`Docker`编译。 至于为什么有两个驱动版本请参考[wiliwili](https://github.com/xfangfang/wiliwili/wiki#%E7%A1%AC%E4%BB%B6%E8%A7%A3%E7%A0%81) wiki下的硬件解码部分。

```bash

git clone --recursive https://github.com/anonymous5l/nxplayer.git

cd nxplayer

## OpenGL版本
docker run --platform linux/amd64 --rm -v $(pwd):/data devkitpro/devkita64:20240324 \
  bash -c "/data/scripts/build_switch.sh"
  
## deko3d版本
docker run --platform linux/amd64 --rm -v $(pwd):/data devkitpro/devkita64:20240324 \
  bash -c "/data/scripts/build_switch_deko3d.sh"
```

## MPV配置

Switch下可以将字幕字体文件放入`SD卡/config/mpv/`目录下，没有的话自己创建。字体文件名为`subfont.ttf`，具体配置目录参考[mpv](https://mpv.io/manual/master/#FILES)下的`Files`段。
不放置的话可能会导致`MPV`播放视频中文字体乱码。