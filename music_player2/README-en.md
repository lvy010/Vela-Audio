# Vela Audio

Vela Audio is an embedded music player designed based on the openvela system. It provides a modern UI interface with support for audio playback, intelligent playlist management, splash screen, volume control, and other features. Built with a modular architecture design for easy extension and maintenance.

## Table of Contents

- [Features](#features)
- [System Requirements](#system-requirements)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [User Guide](#user-guide)
- [Customization Guide](#customization-guide)
- [Contributing](#contributing)

## 📚 Detailed Documentation

| Document Type | 中文文档 | English Documentation |
|---------------|----------|----------------------|
| 🔧 Troubleshooting | [故障排除指南](docs/ch/TROUBLESHOOTING.md) | [Troubleshooting Guide](docs/en/TROUBLESHOOTING.md) |
| 🏗️ Technical Architecture | [技术文档](docs/ch/TECHNICAL.md) | [Technical Documentation](docs/en/TECHNICAL.md) |
| 📋 Version History | [更新日志](docs/ch/CHANGELOG.md) | [Changelog](docs/en/CHANGELOG.md) |

## Features

### Core Features
- Audio playback control (play/pause/stop)
- Previous/next track switching
- Volume adjustment control
- Playlist management
- Real-time playback progress display
- Top status bar (time, date, Wi-Fi, battery)

### Interface Features
- Splash screen: Logo animation and loading effects
- Rectangle adaptation: Optimized for simulator rectangular screen
- Volume bar: Visual volume control
- Scrolling song information: Long titles automatically scroll

### System Features
- Wi-Fi network connection management
- Complete file system integration
- JSON configuration file management
- Real-time system resource monitoring
- Modular architecture design

## System Requirements

### Hardware Requirements
ARM architecture embedded devices with audio output and display capabilities.

### Software Requirements
- **Operating System**: openvela
- **Graphics Library**: LVGL 9.x
- **Audio Library**: NuttX Audio Framework / Simulator Audio Controller
- **Network**: Wi-Fi driver support

### Development Environment
- **Compiler**: arm-none-eabi-gcc
- **Build System**: Make / NuttX Build System
- **Debug Tools**: ADB
- **Host System**: Linux (Ubuntu 22.04 recommended)
- **Emulator**: QEMU (for testing)

## Project Structure

```
music_player2/
├── Core Modules/
│   ├── music_player2.c
│   ├── music_player2.h
│   ├── music_player2_main.c
│   ├── splash_screen.c
│   ├── playlist_manager.c
│   └── playlist_manager.h
├── Audio Processing Module/
│   ├── audio_ctl.c
│   └── audio_ctl.h
├── Font Configuration Module/
│   ├── font_config.c
│   └── font_config.h
├── Network Module/
│   ├── wifi.c
│   └── wifi.h
├── res/
│   ├── fonts/
│   ├── icons/
│   ├── musics/
│   └── config.json
├── Build Configuration/
│   ├── Kconfig
│   ├── Makefile
│   └── Make.defs
└── README.md
```

## Getting Started

### Environment Setup
```bash
# Install necessary tools
sudo apt update && sudo apt install -y android-tools-adb build-essential git

# Clone openvela repository
git clone <openvela-repo-url> && cd vela_code
```

### Configuration and Build
```bash
# Configure music player
echo "CONFIG_LVX_USE_DEMO_MUSIC_PLAYER=y" >> vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig
echo 'CONFIG_LVX_MUSIC_PLAYER_DATA_ROOT="/data"' >> vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig

# Build project
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap distclean -j8
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j8
```

### Startup and Deployment
```bash
# Start emulator
cd nuttx && ln -sf vela_ap.elf nuttx && cd ..
./emulator.sh vela

# Wait for startup and connect ADB
sleep 15 && adb connect 127.0.0.1:5555

# Deploy resource files
adb -s emulator-5554 push apps/packages/demos/music_player2/res /data/

# Start music player
adb -s emulator-5554 shell "music_player2 &"
```

### Configuration File Description

#### config.json - Wi-Fi Configuration
```json
{
  "wifi": {
    "ssid": "vela_network",
    "pswd": "vela123456"
  }
}
```

#### manifest.json - Music Configuration
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

## User Guide

### Basic Operations
1. **Play/Pause**: Click the central play button
2. **Previous/Next**: Click the left/right arrow buttons
3. **Volume Control**: Click the volume button to show volume bar
4. **Open Playlist**: Click the playlist button
5. **Select Song**: Click the desired song in the list

### Debug Mode
Enable debug mode by adding the `-DDEBUG` flag during compilation:
```bash
make CFLAGS="-DDEBUG"
```

## Customization Guide

### Adding New Music

#### 1. Prepare Audio Files
```bash
# Ensure audio files are in supported format (MP3/WAV)
# Prepare album cover (recommended 300x300, supports JPG/PNG format)
convert cover.jpg -resize 300x300 cover.jpg
```

#### 2. Update Configuration File
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

#### 3. Deploy Files
```bash
# Copy files to resource directory
cp new_song.mp3 res/musics/
cp new_cover.jpg res/musics/

# Push to device
adb push res/musics/ /data/res/musics/
```

### Interface Customization

#### Modify Theme Colors
```c
// Modify in music_player2.c
#define MODERN_PRIMARY_COLOR        lv_color_hex(0x00BFFF)
#define MODERN_SECONDARY_COLOR      lv_color_hex(0xFF6B6B)
#define MODERN_BACKGROUND_COLOR     lv_color_hex(0x121212)
```

#### Adjust Interface Layout
```c
// Modify resolution adaptation
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 800

// Adjust component sizes
#define COVER_SIZE 200
#define BUTTON_SIZE 60
```

## Contributing

### Development Process
1. Fork the project
2. Create feature branch
3. Develop and test
4. Submit changes
5. Create Pull Request

### Code Standards
- Function naming: snake_case
- Variable naming: snake_case
- Constant naming: UPPER_CASE
- Structure naming: snake_case_t

### Commit Message Standards
- feat: New feature
- fix: Bug fix
- docs: Documentation update
- style: Code formatting adjustment
- refactor: Refactoring
- test: Test related

## License

This project is licensed under the Apache License 2.0 open source license.

## Acknowledgments

- **openvela**: Providing excellent embedded operating system platform
- **LVGL 9.x Community**: Providing graphics library and UI component support

---

*Last updated: 2025-09-29*  
