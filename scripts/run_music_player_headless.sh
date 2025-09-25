#!/usr/bin/env bash
set -euo pipefail

# Resolve repo root
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
DEFCONFIG="$ROOT_DIR/vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig"

echo "[run] repo root: $ROOT_DIR"

ensure_cfg() {
  # usage: ensure_cfg KEY=VALUE (quotes kept)
  local line="$1"
  local key="${line%%=*}"
  # remove any existing line for this key
  sed -i "/^${key}=.*/d" "$DEFCONFIG" || true
  echo "$line" >> "$DEFCONFIG"
}

echo "[run] updating defconfig for music_player and NSH builtins..."
ensure_cfg "CONFIG_BUILTIN=y"
ensure_cfg "CONFIG_NSH_LIBRARY=y"
ensure_cfg "CONFIG_NSH_BUILTIN_APPS=y"
ensure_cfg "CONFIG_SYSTEM_NSH=y"
ensure_cfg "CONFIG_LVX_USE_DEMO_MUSIC_PLAYER=y"
ensure_cfg "CONFIG_LVX_MUSIC_PLAYER_DATA_ROOT=\"/data\""
# Enable MP3 support and audio features
ensure_cfg "CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT=y"
ensure_cfg "CONFIG_LIB_MAD=y"
ensure_cfg "CONFIG_AUDIO=y"
ensure_cfg "CONFIG_AUDIO_DEVICES=y"

echo "[run] 同步最新music_player代码..."
"$ROOT_DIR/scripts/sync_music_player_code.sh"

echo "[run] distclean and build (goldfish-armeabi-v7a-ap)..."
"$ROOT_DIR/build.sh" vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap distclean
"$ROOT_DIR/build.sh" vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j4

echo "[run] linking nuttx executable symlink..."
ln -sf vela_ap.elf "$ROOT_DIR/nuttx/nuttx"

echo "[run] starting emulator headless..."
export QT_QPA_PLATFORM=offscreen
export DISPLAY=:0
pkill -f qemu || true
sleep 2
"$ROOT_DIR/emulator.sh" vela -no-window -gpu off -no-audio -no-snapshot-load &
EMULATOR_PID=$!
sleep 20

echo "[run] connecting adb and preparing resources..."
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
    echo "[run] Using device: $DEVICE_ID"
    adb -s "$DEVICE_ID" shell 'mkdir -p /data/res' || true
    adb -s "$DEVICE_ID" push "$ROOT_DIR/music_player/res" /data/ >/dev/null 2>&1 || echo "[warn] Resource push failed"
    
    echo "[run] launching music_player..."
    adb -s "$DEVICE_ID" shell 'music_player &' || true
    sleep 2
    adb -s "$DEVICE_ID" shell 'ps | grep music_player || echo "music_player not running"'
else
    echo "[error] No ADB connection established"
    echo "[info] Emulator PID: $EMULATOR_PID"
    echo "[info] You can try manually: adb connect 127.0.0.1:5555"
fi

echo "[done] music_player started (headless)."


