#
# Copyright (C) 2022 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

-include $(APPDIR)/Make.defs

ifeq ($(CONFIG_LVX_USE_DEMO_MUSIC_PLAYER), y)
PROGNAME = music_player
PRIORITY = 100
STACKSIZE = 16384  # 减少栈大小从40KB到16KB
MODULE = $(CONFIG_LVX_USE_DEMO_MUSIC_PLAYER)

# 音乐播放器源文件 - 使用模拟器兼容版本
CSRCS = music_player.c audio_ctl.c wifi.c splash_screen.c playlist_manager.c font_config.c

# 主入口文件
MAINSRC = music_player_main.c

# 模拟器环境配置
CFLAGS += -DUSING_SIMULATOR_AUDIO=1

# 基础库链接
LDLIBS += -lm -lpthread

endif

include $(TOPDIR)/../apps/Application.mk
