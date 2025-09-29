# Vela Audio

<div align="center">

**基于 OpenVela 系统的现代化音乐播放器**

[![OpenVela](https://img.shields.io/badge/Platform-OpenVela-blue.svg)](https://openvela.org)
[![LVGL](https://img.shields.io/badge/GUI-LVGL%209.x-green.svg)](https://lvgl.io)
[![License](https://img.shields.io/badge/License-Apache%202.0-orange.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-v2.2.0-red.svg)](music_player/docs/ch/CHANGELOG.md)

<img src="png/player.gif" alt="操作演示" width="500"/>

</div>

## 快速开始

```bash
# 一键启动（推荐）
./scripts/run_music_player_gui.sh

# 手动构建
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j8
./emulator.sh vela && adb push music_player/res /data/ && adb shell "music_player &"
```

## 技术特性

- **系统平台**：OpenVela/NuttX 嵌入式系统
- **图形界面**：LVGL 9.x，32位色深渲染
- **架构设计**：模块化设计，6个独立模块，事件驱动
- **核心功能**：音频播放、播放列表、音量控制、启动页面

## 文档导航

| 语言 | 完整文档 | 技术架构 | 故障排除 | 更新日志 |
|------|----------|----------|----------|----------|
| 🇨🇳 中文 | [README](music_player/README.md) | [架构设计](music_player/docs/ch/TECHNICAL.md) | [问题解决](music_player/docs/ch/TROUBLESHOOTING.md) | [版本历史](music_player/docs/ch/CHANGELOG.md) |
| 🇺🇸 English | [README](music_player/README-en.md) | [Architecture](music_player/docs/en/TECHNICAL.md) | [Troubleshooting](music_player/docs/en/TROUBLESHOOTING.md) | [Changelog](music_player/docs/en/CHANGELOG.md) |

## 开源协议

本项目采用 [Apache License 2.0](LICENSE) 开源协议  
欢迎提交 Issue 和 Pull Request 贡献代码
