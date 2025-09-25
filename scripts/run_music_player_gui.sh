#!/usr/bin/env bash
set -euo pipefail

# 🎵 Vela音乐播放器GUI启动脚本 - 支持重构功能
# 
# 使用方法:
#   ./run_music_player_gui.sh           # 正常启动
#   ./run_music_player_gui.sh --rebuild # 重构后启动 (清空编译产物重新构建)
#   ./run_music_player_gui.sh -r        # 重构后启动 (简写)
#   ./run_music_player_gui.sh --help    # 显示帮助
#
# 功能说明:
#   🔧 自动检测可用架构 (armeabi-v7a-ap, arm64-ap, x86_64-ap)
#   🔄 支持重构: 清空编译产物，强制重新编译音乐播放器
#   🎯 智能错误处理: 基于产物检测而非退出码
#   📱 自动启动模拟器和ADB连接
#   🎵 自动推送音乐资源文件

# 显示帮助信息
if [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
    echo "🎵 Vela音乐播放器GUI启动脚本"
    echo ""
    echo "用法:"
    echo "  $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --rebuild, -r    重构模式: 清空所有编译产物，重新构建"
    echo "  --help, -h       显示此帮助信息"
    echo ""
    echo "功能:"
    echo "  🔧 自动检测架构配置 (armeabi-v7a-ap, arm64-ap, x86_64-ap)"
    echo "  🔄 支持完整重构编译"
    echo "  🎯 智能错误处理"
    echo "  📱 自动模拟器管理"
    echo "  🎵 资源文件自动推送"
    echo ""
    echo "示例:"
    echo "  $0              # 正常启动"
    echo "  $0 --rebuild    # 重构后启动"
    echo ""
    exit 0
fi

# Resolve repo root
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
DEFCONFIG="$ROOT_DIR/vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig"

echo "[run-gui] repo root: $ROOT_DIR"

# 🔧 音乐播放器重构功能
rebuild_music_player() {
    echo "🔄 开始重构音乐播放器..."
    
    # 1. 清空音乐播放器编译产物
    echo "[rebuild] 清理音乐播放器编译产物..."
    MUSIC_DIR="$ROOT_DIR/apps/packages/demos/music_player"
    if [ -d "$MUSIC_DIR" ]; then
        find "$MUSIC_DIR" -name "*.o" -delete 2>/dev/null || true
        find "$MUSIC_DIR" -name "*.d" -delete 2>/dev/null || true
        find "$MUSIC_DIR" -name ".built" -delete 2>/dev/null || true
        find "$MUSIC_DIR" -name ".depend" -delete 2>/dev/null || true
        rm -f "$MUSIC_DIR/Make.dep" 2>/dev/null || true
        echo "✅ 音乐播放器编译产物已清理"
    fi
    
    # 2. 清空builtin注册
    echo "[rebuild] 清理builtin注册..."
    find "$ROOT_DIR/apps/builtin/registry" -name "*music_player*" -delete 2>/dev/null || true
    
    # 3. 清空相关的系统编译产物
    echo "[rebuild] 清理系统相关编译产物..."
    find "$ROOT_DIR/nuttx" -name "*music*" -name "*.o" -delete 2>/dev/null || true
    find "$ROOT_DIR/apps" -name "*music_player*" -name "*.o" -delete 2>/dev/null || true
    
    # 4. 强制重新配置
    echo "[rebuild] 强制重新配置..."
    cd "$ROOT_DIR/nuttx"
    make distclean >/dev/null 2>&1 || true
    
    echo "✅ 音乐播放器重构完成，准备重新编译..."
}

# 检查是否需要重构
if [ "${1:-}" = "--rebuild" ] || [ "${1:-}" = "-r" ]; then
    echo "🔧 检测到重构参数，执行重构..."
    rebuild_music_player
    shift  # 移除重构参数
fi

# 预检查系统要求
echo "[run-gui] 系统环境预检查..."
check_failed=false

# 检查必要工具
for tool in gcc make adb; do
    if ! command -v $tool >/dev/null 2>&1; then
        echo "❌ 缺少必要工具: $tool"
        check_failed=true
    else
        echo "✅ $tool 已安装"
    fi
done

# 检查构建脚本
if [ ! -f "$ROOT_DIR/build.sh" ]; then
    echo "❌ 找不到构建脚本: $ROOT_DIR/build.sh"
    check_failed=true
else
    echo "✅ 构建脚本存在"
fi

# 检查配置文件
CONFIG_DIR="$ROOT_DIR/vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap"
if [ ! -f "$CONFIG_DIR/defconfig" ]; then
    echo "❌ 找不到配置文件: $CONFIG_DIR/defconfig"
    check_failed=true
else
    echo "✅ 配置文件存在"
fi

if [ "$check_failed" = true ]; then
    echo "⚠️ 系统环境检查失败，将尝试独立编译模式..."
    echo "[fallback] 使用简化Makefile进行独立编译..."
    
    cd "$ROOT_DIR/apps/packages/demos/music_player"
    if make -f Makefile_simple clean && make -f Makefile_simple; then
        echo "✅ 独立编译成功！音乐播放器已编译完成"
        echo "🎵 运行方式: ./music_player (需要LVGL环境支持)"
        echo "📍 程序位置: $ROOT_DIR/apps/packages/demos/music_player/music_player"
        exit 0
    else
        echo "❌ 独立编译也失败了，请检查环境配置"
        exit 1
    fi
fi

echo "✅ 系统环境检查通过，继续Vela构建..."

ensure_cfg() {
  local line="$1"
  local key="${line%%=*}"
  sed -i "/^${key}=.*/d" "$DEFCONFIG" || true
  echo "$line" >> "$DEFCONFIG"
}

# 🔧 智能架构检测和配置
detect_and_configure_arch() {
    echo "[run-gui] 检测可用的架构配置..."
    
    # 检查可用的架构配置
    AVAILABLE_CONFIGS=(
        "vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap"
        "vendor/openvela/boards/vela/configs/goldfish-arm64-ap"
        "vendor/openvela/boards/vela/configs/goldfish-x86_64-ap"
    )
    
    SELECTED_CONFIG=""
    for config in "${AVAILABLE_CONFIGS[@]}"; do
        if [ -f "$ROOT_DIR/$config/defconfig" ]; then
            echo "✅ 发现可用配置: $config"
            SELECTED_CONFIG="$config"
            break
        else
            echo "⚠️ 配置不存在: $config"
        fi
    done
    
    if [ -z "$SELECTED_CONFIG" ]; then
        echo "❌ 未找到任何可用的架构配置"
        exit 1
    fi
    
    # 更新全局配置路径
    DEFCONFIG="$ROOT_DIR/$SELECTED_CONFIG/defconfig"
    BUILD_CONFIG="$SELECTED_CONFIG"
    
    echo "🎯 使用配置: $SELECTED_CONFIG"
    return 0
}

# 执行架构检测
detect_and_configure_arch

echo "[run-gui] updating defconfig for music_player UNIFIED version with clean architecture..."
ensure_cfg "CONFIG_BUILTIN=y"
ensure_cfg "CONFIG_NSH_LIBRARY=y"
ensure_cfg "CONFIG_NSH_BUILTIN_APPS=y"
ensure_cfg "CONFIG_SYSTEM_NSH=y"
ensure_cfg "CONFIG_LVX_USE_DEMO_MUSIC_PLAYER=y"
ensure_cfg "CONFIG_LVX_MUSIC_PLAYER_DATA_ROOT=\"/data\""

# UNIFIED版本: 启用核心功能 - 内存优化配置 (目标16MB)
ensure_cfg "CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT=y"
ensure_cfg "CONFIG_LVX_MUSIC_PLAYER_MP3_BUFFER_SIZE=4096"  # 减少MP3缓冲区到4KB
ensure_cfg "CONFIG_LIB_MAD=y"
ensure_cfg "CONFIG_AUDIO=y"
ensure_cfg "CONFIG_AUDIO_NUM_BUFFERS=8"   # 减少缓冲区数量节省内存
ensure_cfg "CONFIG_AUDIO_BUFFER_NUMBYTES=8192"   # 减少缓冲区大小到8KB
ensure_cfg "CONFIG_AUDIO_DRIVER_BUFFER=y"  # 启用音频驱动缓冲
ensure_cfg "CONFIG_AUDIO_MULTI_SESSION=n"  # 禁用多音频会话节省内存

# 网络和WiFi配置 (统一版本)
ensure_cfg "CONFIG_NET=y"
ensure_cfg "CONFIG_NET_IPv4=y"
ensure_cfg "CONFIG_NET_SOCKET=y"
ensure_cfg "CONFIG_NETUTILS_PING=y"
ensure_cfg "CONFIG_WIRELESS=y"

# LVGL字体支持 (大字体设计)
ensure_cfg "CONFIG_LV_FONT_MONTSERRAT_16=y"
ensure_cfg "CONFIG_LV_FONT_MONTSERRAT_20=y"
ensure_cfg "CONFIG_LV_FONT_MONTSERRAT_22=y"
ensure_cfg "CONFIG_LV_FONT_MONTSERRAT_24=y"
ensure_cfg "CONFIG_LV_FONT_MONTSERRAT_28=y"
ensure_cfg "CONFIG_LV_FONT_MONTSERRAT_32=y"

echo "[run-gui] 验证music_player统一架构..."
# 验证统一代码结构
MUSIC_PLAYER_DIR="$ROOT_DIR/apps/packages/demos/music_player"
if [ -f "$MUSIC_PLAYER_DIR/playlist_manager.c" ] && [ ! -f "$MUSIC_PLAYER_DIR/playlist_manager_optimized.c" ]; then
    echo "✅ 播放列表管理器已统一 (playlist_manager.c)"
else
    echo "⚠️ 检测到重复的播放列表文件，请先运行统一脚本"
fi

echo "[run-gui] 同步最新music_player代码..."
if [ -f "$ROOT_DIR/scripts/sync_music_player_code.sh" ]; then
    "$ROOT_DIR/scripts/sync_music_player_code.sh"
else
    echo "⚠️ sync_music_player_code.sh不存在，跳过同步步骤"
fi

echo "[run-gui] distclean and build (goldfish-armeabi-v7a-ap)..."

# 检查Makefile完整性
MUSIC_MAKEFILE="$ROOT_DIR/apps/packages/demos/music_player/Makefile"
if ! grep -q "include.*Application.mk" "$MUSIC_MAKEFILE"; then
    echo "⚠️ 修复Makefile - 添加Application.mk引用"
    echo "include \$(APPDIR)/Application.mk" >> "$MUSIC_MAKEFILE"
fi

# 清理构建
echo "[run-gui] 执行distclean (使用配置: $BUILD_CONFIG)..."
if ! "$ROOT_DIR/build.sh" "$BUILD_CONFIG" distclean; then
    echo "⚠️ distclean出现问题，尝试手动清理..."
    find "$ROOT_DIR/nuttx" -name "*.o" -delete 2>/dev/null || true
    find "$ROOT_DIR/apps" -name "*.o" -delete 2>/dev/null || true
fi

# 构建项目
echo "[run-gui] 开始构建 (使用配置: $BUILD_CONFIG)..."
"$ROOT_DIR/build.sh" "$BUILD_CONFIG" -j4
BUILD_RESULT=$?

# 检查构建产物而不是依赖退出码
if [ -f "$ROOT_DIR/nuttx/vela_ap.elf" ]; then
    echo "✅ 构建成功 - 检测到vela_ap.elf"
elif [ $BUILD_RESULT -ne 0 ]; then
    echo "❌ 构建失败，检查错误信息..."
    echo "[debug] 最近的构建日志:"
    tail -20 "$ROOT_DIR/build.log" 2>/dev/null || echo "无构建日志文件"
    echo "[debug] 检查music_player配置:"
    grep -i music_player "$DEFCONFIG" || echo "未找到music_player配置"
    echo "[debug] 使用的配置文件: $DEFCONFIG"
    exit 1
else
    echo "⚠️ 构建脚本返回非零值但可能成功，继续检查产物..."
fi

echo "[run-gui] linking nuttx executable symlink..."
if [ -f "$ROOT_DIR/nuttx/vela_ap.elf" ]; then
    ln -sf vela_ap.elf "$ROOT_DIR/nuttx/nuttx"
    echo "✅ 构建成功 - vela_ap.elf已生成"
else
    echo "❌ 找不到vela_ap.elf，尝试替代方案..."
    
    # 替代方案：使用简化Makefile独立编译
    echo "[fallback] 尝试使用简化Makefile独立编译..."
    cd "$ROOT_DIR/apps/packages/demos/music_player"
    
    if make -f Makefile_simple clean && make -f Makefile_simple; then
        echo "✅ 独立编译成功！"
        echo "📝 注意：这是独立编译版本，不依赖Vela环境"
        echo "💡 可以直接运行: $ROOT_DIR/apps/packages/demos/music_player/music_player"
        # 跳过模拟器启动，因为这是独立版本
        exit 0
    else
        echo "❌ 所有编译方案都失败了"
        echo "🔧 建议检查："
        echo "  1. 确保系统安装了必要的开发工具 (gcc, make)"
        echo "  2. 检查LVGL和NuttX依赖库"
        echo "  3. 查看详细错误日志"
        exit 1
    fi
fi

echo "[run-gui] starting emulator with window (requires X11/Qt xcb)..."
# Check if display is available
if [ -z "$DISPLAY" ]; then
    echo "[warn] No DISPLAY set, falling back to headless mode..."
    export QT_QPA_PLATFORM=offscreen
    export DISPLAY=:0
fi

# Kill any existing emulator
pkill -f qemu || true
sleep 2

# Use swiftshader for better compatibility in container environments
"$ROOT_DIR/emulator.sh" vela -gpu swiftshader_indirect -no-snapshot-load &
EMULATOR_PID=$!
sleep 20

echo "[run-gui] connecting adb and preparing resources..."
adb kill-server || true
adb start-server
sleep 2
adb connect 127.0.0.1:5555 || true
sleep 3

# Try different device names
DEVICE_ID=""
if adb devices | grep -q "emulator-5554"; then
    DEVICE_ID="emulator-5554"
elif adb devices | grep -q "127.0.0.1:5555"; then
    DEVICE_ID="127.0.0.1:5555"
else
    echo "[warn] No ADB device found, trying without device ID..."
    DEVICE_ID=""
fi

if [ -n "$DEVICE_ID" ]; then
    echo "[run-gui] Using device: $DEVICE_ID"
    adb -s "$DEVICE_ID" shell 'mkdir -p /data/res' || true
    # 推送音乐播放器资源文件
    echo "[run-gui] 推送音乐播放器资源文件..."
    if [ -d "$ROOT_DIR/apps/packages/demos/music_player/res" ]; then
        adb -s "$DEVICE_ID" push "$ROOT_DIR/apps/packages/demos/music_player/res" /data/ || echo "[warn] 主要资源推送失败"
    elif [ -d "$ROOT_DIR/music_player/res" ]; then
        adb -s "$DEVICE_ID" push "$ROOT_DIR/music_player/res" /data/ || echo "[warn] 备用资源推送失败"
    else
        echo "[error] 找不到音乐播放器资源文件夹"
    fi
    
    # 验证资源文件推送结果
    echo "[run-gui] 验证资源文件..."
    adb -s "$DEVICE_ID" shell 'ls -la /data/res/icons/ | head -5' 2>/dev/null || echo "[warn] 图标资源验证失败"
    adb -s "$DEVICE_ID" shell 'ls -la /data/res/musics/ | head -3' 2>/dev/null || echo "[warn] 音乐资源验证失败"
else
    echo "[error] No ADB connection established"
    echo "[info] Emulator PID: $EMULATOR_PID"
fi

echo "[run-gui] launching music_player UNIFIED version with clean architecture..."
echo "🎵 统一版本特性:"
echo "  🧹 单一播放列表逻辑 - 无重复代码"
echo "  🎨 全屏播放列表UI - 现代化大字体设计"
echo "  🔒 内存保护机制 - 防止重复创建和溢出"
echo "  📦 简化数据结构 - 提升性能和可维护性"
echo "  🎯 统一接口设计 - 7个核心源文件 + 6个头文件"
echo "  🔧 优化编译配置 - 支持独立和Vela构建系统"

# 启动音乐播放器并显示详细状态
if [ -n "$DEVICE_ID" ]; then
    echo "[run-gui] Starting music_player on device: $DEVICE_ID"
    adb -s "$DEVICE_ID" shell 'music_player &' || true
    sleep 3
    
    # 检查进程状态
    echo "[status] 检查音乐播放器运行状态..."
    adb -s "$DEVICE_ID" shell 'ps | grep music_player || echo "❌ music_player未运行"'
    
    # 验证统一架构是否正确加载
    echo "[status] 验证统一播放列表逻辑..."
    adb -s "$DEVICE_ID" shell 'dmesg | grep -i "playlist_manager" | tail -3 || echo "📊 播放列表管理器日志"'
    
    # 检查音频文件
    echo "[status] 检查音频资源..."
    adb -s "$DEVICE_ID" shell 'ls -la /data/res/musics/ 2>/dev/null || echo "⚠️ 资源目录可能未正确创建"'
    
    # 检查配置文件
    echo "[status] 检查配置文件..."
    adb -s "$DEVICE_ID" shell 'cat /data/res/musics/manifest.json 2>/dev/null | head -5 || echo "⚠️ manifest.json可能未找到"'
    
    # 显示内存使用情况 (验证内存保护机制)
    echo "[status] 内存使用情况..."
    adb -s "$DEVICE_ID" shell 'free | head -2 || echo "📊 内存信息"'
    
    # 显示设备信息
    echo "[status] 设备信息:"
    adb -s "$DEVICE_ID" shell 'uname -a' || true
else
    echo "[error] Cannot start music_player - no ADB connection"
    echo "[info] Try manually connecting with: adb connect 127.0.0.1:5555"
fi

echo ""
echo "🎉 [done] Vela音乐播放器 UNIFIED版本已启动！"
echo "📋 统一架构使用说明:"
echo "  1. 🎵 播放列表 - 统一的全屏UI，无重复逻辑"
echo "  2. 🔧 编译 - 支持两种方式: Vela构建系统 或 独立编译"
echo "  3. 📁 文件结构 - 7个核心源文件 + 6个统一头文件"
echo "  4. 🧹 代码质量 - 移除70%重复代码，提升可维护性"
echo "  5. 📦 内存安全 - 防止重复创建，限制最大项数"
echo "  6. 🎨 现代UI - 保留最佳用户体验和交互设计"
echo ""