# Vela Music_player

<div align="center">

**基于 OpenVela 系统的音乐播放器**

[![OpenVela](https://img.shields.io/badge/Platform-OpenVela-blue.svg)](https://openvela.org)
[![LVGL](https://img.shields.io/badge/GUI-LVGL%208.x-green.svg)](https://lvgl.io)
[![License](https://img.shields.io/badge/License-Apache%202.0-orange.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-v2.1.0-red.svg)](CHANGELOG.md)

</div>

## 🎬 项目演示

<div align="center">
<img src="png/player.gif" alt="操作演示" width="600"/>
<br/>
</div>

## 🚀 快速启动

```bash
# 一键启动（推荐）
./scripts/run_music_player_gui.sh

# 手动构建
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j8
./emulator.sh vela
adb push music_player/res /data/
adb shell "music_player &"
```

## 🏗️ 技术栈

- **系统**：OpenVela/NuttX
- **界面**：LVGL 8.x (32位色深)
- **音频**：libmad (MP3) + 原生WAV
- **架构**：6个独立模块，事件驱动

## 📖 完整文档

### [**>> 查看完整技术文档 <<**](music_player/README.md)

包含详细的配置说明、API参考、架构设计和故障排除指南。

## 🤝 贡献 & 许可

-  **许可证**：[Apache License 2.0](LICENSE)
- **问题反馈**：欢迎提交 Issue 和 Pull Request
- **致谢**：感谢 OpenVela、LVGL 社区及所有贡献者
