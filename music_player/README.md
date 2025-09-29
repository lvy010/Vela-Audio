# Vela Audio

Vela Audio是一个嵌入式音乐播放器，基于 OpenVela 系统设计。提供现代化的 UI 界面，支持音频播放、智能播放列表管理、启动页面、音量控制等功能。采用模块化架构设计，易于扩展和维护。

## 目录

- [功能特性](#功能特性)
- [系统要求](#系统要求)
- [项目结构](#项目结构)
- [启动指南](#启动指南)
- [使用指南](#使用指南)
- [自定义指南](#自定义指南)
- [故障排除](TROUBLESHOOTING.md)
- [技术文档](TECHNICAL.md)
- [贡献指南](#贡献指南)
- [更新日志](CHANGELOG.md)

## 功能特性

### 核心功能
- 音频播放控制（播放/暂停/停止）
- 上一首/下一首切换
- 音量调节控制
- 播放列表管理
- 实时播放进度显示
- 顶部状态栏（时间、日期、Wi-Fi、电池）

### 界面功能
- 启动页面：Logo 动画和加载效果
- 长方形适配：为模拟器长方形屏幕优化
- 音量条：可视化音量控制
- 滚动歌曲信息：长标题自动滚动显示

### 系统功能
- Wi-Fi 网络连接管理
- 完整文件系统集成
- JSON 配置文件管理
- 系统资源实时监控
- 模块化架构设计

## 系统要求

### 硬件要求
支持 ARM 架构的嵌入式设备，具备音频输出和显示功能。

### 软件要求
- **操作系统**：OpenVela
- **图形库**：LVGL 8.x
- **音频库**：NuttX Audio 框架 / 模拟器音频控制器
- **网络**：Wi-Fi 驱动支持

### 开发环境
- **编译器**：arm-none-eabi-gcc
- **构建系统**：Make / NuttX 构建系统
- **调试工具**：ADB
- **主机系统**：Linux (推荐 Ubuntu 22.04)
- **模拟器**：QEMU (用于测试)

## 项目结构

```
music_player/
├── 核心模块/                      # 主要应用代码
│   ├── music_player.c           # 主应用逻辑和UI界面
│   ├── music_player.h           # 主应用头文件和数据结构
│   ├── music_player_main.c      # 应用程序入口点
│   ├── splash_screen.c          # 启动页面模块（独立）
│   ├── playlist_manager.c       # 播放列表管理器（独立）
│   └── playlist_manager.h       # 播放列表管理器头文件
├── 音频处理模块/                  # 音频相关代码
│   ├── audio_ctl.c              # 音频控制
│   └── audio_ctl.h              # 音频控制头文件
├── 字体配置模块/                  # 字体系统
│   ├── font_config.c            # 字体配置实现
│   └── font_config.h            # 字体配置头文件
├── 网络模块/                      # 网络连接代码
│   ├── wifi.c                   # Wi-Fi 管理模块
│   └── wifi.h                   # Wi-Fi 管理头文件
├── res/                         # 资源文件目录
│   ├── fonts/                   # 字体文件
│   ├── icons/                   # 图标文件
│   ├── musics/                  # 音乐文件目录
│   └── config.json             # 全局配置文件（Wi-Fi等）
├── 构建配置/                      # 构建系统文件
│   ├── Kconfig                 # 内核配置选项
│   ├── Makefile                # 主构建文件
│   └── Make.defs               # 构建定义
└── README.md                    # 项目文档（本文件）
```

## 启动指南

### 环境准备
```bash
# 安装必要工具
sudo apt update && sudo apt install -y android-tools-adb build-essential git

# 克隆 OpenVela 仓库
git clone <openvela-repo-url> && cd vela_code
```

### 配置与构建
```bash
# 配置音乐播放器
echo "CONFIG_LVX_USE_DEMO_MUSIC_PLAYER=y" >> vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig
echo 'CONFIG_LVX_MUSIC_PLAYER_DATA_ROOT="/data"' >> vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig

# 构建项目
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap distclean -j8
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j8
```

### 启动与部署
```bash
# 启动模拟器
cd nuttx && ln -sf vela_ap.elf nuttx && cd ..
./emulator.sh vela

# 等待启动并连接 ADB
sleep 15 && adb connect 127.0.0.1:5555

# 部署资源文件
adb -s emulator-5554 push apps/packages/demos/music_player/res /data/

# 启动音乐播放器
adb -s emulator-5554 shell "music_player &"
```

### 配置文件说明

#### config.json - Wi-Fi配置
```json
{
  "wifi": {
    "ssid": "vela_network",
    "pswd": "vela123456"
  }
}
```

#### manifest.json - 音乐配置
```json
{
  "musics": [
    {
      "path": "test.mp3",
      "name": "Test Song",
      "artist": "Demo Artist", 
      "cover": "test.jpg",
      "total_time": 180000,
      "color": "#4A90E2"
    }
  ]
}
```

## 使用指南

### 基本操作
1. **播放/暂停**: 点击中央播放按钮
2. **上一首/下一首**: 点击左右箭头按钮
3. **音量控制**: 点击音量按钮显示音量条
4. **打开播放列表**: 点击播放列表按钮
5. **选择歌曲**: 在列表中点击想要播放的歌曲

### 调试模式
在编译时添加 `-DDEBUG` 标志启用调试模式：
```bash
make CFLAGS="-DDEBUG"
```

## 自定义指南

### 添加新音乐

#### 1. 准备音频文件
```bash
# 确保音频文件为支持的格式（MP3/WAV）
# 准备专辑封面（推荐 300x300，支持 JPG/PNG 格式）
convert cover.jpg -resize 300x300 cover.jpg
```

#### 2. 更新配置文件
```json
{
  "musics": [
    {
      "path": "new_song.mp3",
      "name": "New Song Title",
      "artist": "Artist Name",
      "cover": "new_cover.jpg",
      "total_time": 240000,
      "color": "#FF5722"
    }
  ]
}
```

#### 3. 部署文件
```bash
# 复制文件到资源目录
cp new_song.mp3 res/musics/
cp new_cover.jpg res/musics/

# 推送到设备
adb push res/musics/ /data/res/musics/
```

### 自定义界面

#### 修改主题颜色
```c
// 在 music_player.c 中修改
#define MODERN_PRIMARY_COLOR        lv_color_hex(0x00BFFF)
#define MODERN_SECONDARY_COLOR      lv_color_hex(0xFF6B6B)
#define MODERN_BACKGROUND_COLOR     lv_color_hex(0x121212)
```

#### 调整界面布局
```c
// 修改分辨率适配
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 800

// 调整控件大小
#define COVER_SIZE 200
#define BUTTON_SIZE 60
```

## 贡献指南

### 开发流程
1. Fork 项目
2. 创建功能分支
3. 开发和测试
4. 提交更改
5. 创建 Pull Request

### 代码规范
- 函数命名：snake_case
- 变量命名：snake_case
- 常量命名：UPPER_CASE
- 结构体命名：snake_case_t

### 提交信息规范
- feat: 新功能
- fix: 修复bug
- docs: 文档更新
- style: 代码格式调整
- refactor: 重构
- test: 测试相关

## 许可证

本项目采用 Apache License 2.0 开源许可证。

## 致谢

- **OpenVela**：提供优秀的嵌入式操作系统平台
- **LVGL 社区**：提供图形库和UI组件支持

---

*最后更新时间：2025-09-29*  
*当前版本：v2.2.0*