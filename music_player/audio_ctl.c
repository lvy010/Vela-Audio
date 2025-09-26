// Includes

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>

#define _GNU_SOURCE

#include "audio_ctl.h"

#define USING_SIMULATOR_AUDIO 1
#ifndef AUDIO_DEBUG
#define AUDIO_DEBUG 0
#endif

#if AUDIO_DEBUG
#define AUDIO_LOG(fmt, ...) do { \
    printf("[SIMULATOR AUDIO] " fmt "\n", ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)
#else
#define AUDIO_LOG(fmt, ...)
#endif

// Functions
static uint32_t estimate_mp3_duration(off_t file_size);
static void* monitor_thread_func(void* arg);
static void* playback_simulation_thread(void* arg) __attribute__((unused));

// 估算MP3播放时长
static uint32_t estimate_mp3_duration(off_t file_size)
{
    // 假设平均比特率为128kbps
    const uint32_t avg_bitrate = 128000; // bps
    const uint32_t bytes_per_sec = avg_bitrate / 8;
    
    if (bytes_per_sec > 0) {
        return (uint32_t)(file_size / bytes_per_sec);
    }
    
    return 240; // 默认4分钟
}

// 播放状态监控线程
static void* monitor_thread_func(void* arg)
{
    audioctl_s* ctl = (audioctl_s*)arg;
    if (!ctl) {
        AUDIO_LOG("监控线程参数为空，退出");
        return NULL;
    }
    
    AUDIO_LOG("播放监控线程启动");
    
    while (ctl->monitor_running) {
        // 每100ms检查一次播放状态
        usleep(100000);
        
        pthread_mutex_lock(&ctl->control_mutex);
        
        if (ctl->is_playing && !ctl->is_paused) {
            // 更新播放位置
            ctl->current_position_ms += 100;
            
            // 检查是否播放完成
            if (ctl->current_position_ms >= ctl->total_duration_ms * 1000) {
                AUDIO_LOG("模拟播放完成，停止播放");
                ctl->is_playing = false;
                ctl->state = AUDIO_CTL_STATE_STOP;
            }
        }
        
        pthread_mutex_unlock(&ctl->control_mutex);
    }
    
    AUDIO_LOG("播放监控线程退出");
    return NULL;
}

// 模拟播放线程
static void* playback_simulation_thread(void* arg)
{
    audioctl_s* ctl = (audioctl_s*)arg;
    if (!ctl) {
        return NULL;
    }
    
    AUDIO_LOG("模拟播放线程启动: %s", ctl->file_path);
    
    // 模拟播放过程
    while (ctl->is_playing && !ctl->should_stop) {
        usleep(1000000); // 每秒更新一次
        
        pthread_mutex_lock(&ctl->control_mutex);
        if (ctl->is_playing && !ctl->is_paused) {
            // 模拟音频输出（在真实环境中这里会有实际的音频处理）
            // 这里我们只是简单地等待
        }
        pthread_mutex_unlock(&ctl->control_mutex);
    }
    
    AUDIO_LOG("模拟播放线程结束");
    return NULL;
}

// Global Functions

// 检测音频文件格式
int audio_ctl_detect_format(const char *path)
{
    if (!path) {
        AUDIO_LOG("文件路径为空");
        return AUDIO_FORMAT_UNKNOWN;
    }
    
    size_t len = strlen(path);
    if (len < 4) {
        AUDIO_LOG("文件名太短: %s", path);
        return AUDIO_FORMAT_UNKNOWN;
    }
    
    const char *ext = path + len - 4;
    AUDIO_LOG("检测文件扩展名: %s", ext);
    
    if (strcasecmp(ext, ".mp3") == 0) {
        AUDIO_LOG("检测到MP3格式: %s", path);
        return AUDIO_FORMAT_MP3;
    } else if (strcasecmp(ext, ".wav") == 0) {
        AUDIO_LOG("检测到WAV格式: %s", path);
        return AUDIO_FORMAT_WAV;
    }
    
    AUDIO_LOG("未知音频格式: %s", path);
    return AUDIO_FORMAT_UNKNOWN;
}

// 初始化音频控制器
audioctl_s *audio_ctl_init_nxaudio(const char *path)
{
    if (!path) {
        AUDIO_LOG("初始化失败：文件路径为空");
        return NULL;
    }
    
    AUDIO_LOG("初始化模拟器音频控制器: %s", path);
    
    // 分配内存
    audioctl_s *ctl = (audioctl_s*)calloc(1, sizeof(audioctl_s));
    if (!ctl) {
        // 内存分配失败 - 静默处理
        return NULL;
    }
    
    // 初始化控制结构
    strncpy(ctl->file_path, path, sizeof(ctl->file_path) - 1);
    ctl->file_path[sizeof(ctl->file_path) - 1] = '\0';
    
    // 检测文件格式
    ctl->audio_format = audio_ctl_detect_format(path);
    if (ctl->audio_format == AUDIO_FORMAT_UNKNOWN) {
        AUDIO_LOG("不支持的音频格式，释放资源");
        free(ctl);
        return NULL;
    }
    
    // 获取文件大小
    struct stat st;
    if (stat(path, &st) == 0) {
        ctl->file_size = st.st_size;
        AUDIO_LOG("文件大小: %lld bytes", (long long)ctl->file_size);
        
        // 估算播放时长
        if (ctl->audio_format == AUDIO_FORMAT_MP3) {
            uint32_t duration_sec = estimate_mp3_duration(ctl->file_size);
            ctl->total_duration_ms = duration_sec * 1000; // 转换为毫秒
            AUDIO_LOG("估算MP3播放时长: %lu 秒 (%lu ms)", (unsigned long)duration_sec, (unsigned long)ctl->total_duration_ms);
        } else {
            ctl->total_duration_ms = 240 * 1000; // 默认4分钟，转换为毫秒
        }
    } else {
        AUDIO_LOG("无法获取文件信息: %s", strerror(errno));
        free(ctl);
        return NULL;
    }
    
    // 模拟器环境下，不创建真实的NxPlayer，而是使用模拟指针
    ctl->nxplayer = (struct nxplayer_s*)0x12345678; // 模拟指针
    AUDIO_LOG("模拟器环境：创建虚拟NxPlayer实例: %p", ctl->nxplayer);
    
    // 初始化互斥锁
    if (pthread_mutex_init(&ctl->control_mutex, NULL) != 0) {
        AUDIO_LOG("初始化互斥锁失败");
        free(ctl);
        return NULL;
    }

    // 初始化状态
    ctl->state = AUDIO_CTL_STATE_STOP;
    ctl->is_playing = false;
    ctl->is_paused = false;
    ctl->should_stop = false;
    ctl->current_position_ms = 0;
    ctl->monitor_running = 0;
    
    AUDIO_LOG("模拟器音频控制器初始化完成");
    return ctl;
}

// 开始播放音频
int audio_ctl_start(audioctl_s *ctl)
{
    if (!ctl) {
        AUDIO_LOG("启动播放失败：控制器为空");
        return -1;
    }
    
    if (!ctl->nxplayer) {
        AUDIO_LOG("启动播放失败：NxPlayer为空");
        return -1;
    }

    AUDIO_LOG("开始模拟播放: %s", ctl->file_path);
    AUDIO_LOG("音频格式: %d", ctl->audio_format);
    
    pthread_mutex_lock(&ctl->control_mutex);
    
    // 如果已经在播放，先停止
    if (ctl->is_playing) {
        AUDIO_LOG("停止当前模拟播放");
        ctl->is_playing = false;
        ctl->should_stop = true;
    }
    
    // 检查文件是否存在
    if (access(ctl->file_path, R_OK) != 0) {
        AUDIO_LOG("文件访问失败: %s, errno: %d (%s)", ctl->file_path, errno, strerror(errno));
        pthread_mutex_unlock(&ctl->control_mutex);
        return -1;
    }
    
    AUDIO_LOG("文件访问正常，开始模拟播放...");
    
    // 模拟播放成功
    ctl->state = AUDIO_CTL_STATE_START;
    ctl->is_playing = true;
    ctl->is_paused = false;
    ctl->should_stop = false;
    ctl->current_position_ms = 0;
    
    // 启动监控线程
    if (!ctl->monitor_running) {
        ctl->monitor_running = 1;
        if (pthread_create(&ctl->monitor_thread, NULL, monitor_thread_func, ctl) != 0) {
            AUDIO_LOG("创建监控线程失败");
            // 继续，不是致命错误
        } else {
            AUDIO_LOG("监控线程已启动");
        }
    }
    
    pthread_mutex_unlock(&ctl->control_mutex);
    
    AUDIO_LOG("模拟播放启动成功");
    return 0; // 成功
}

// 暂停播放
int audio_ctl_pause(audioctl_s *ctl)
{
    if (!ctl || !ctl->nxplayer) {
        AUDIO_LOG("暂停失败：控制器或NxPlayer为空");
        return -1;
    }
    
    AUDIO_LOG("暂停模拟播放");
    
    pthread_mutex_lock(&ctl->control_mutex);
    
    if (!ctl->is_playing || ctl->is_paused) {
        AUDIO_LOG("当前未在播放或已暂停");
        pthread_mutex_unlock(&ctl->control_mutex);
        return 0;
    }
    
    ctl->state = AUDIO_CTL_STATE_PAUSE;
    ctl->is_paused = true;
    AUDIO_LOG("模拟暂停成功");
    
    pthread_mutex_unlock(&ctl->control_mutex);
    return 0;
}

// 恢复播放
int audio_ctl_resume(audioctl_s *ctl)
{
    if (!ctl || !ctl->nxplayer) {
        AUDIO_LOG("恢复播放失败：控制器或NxPlayer为空");
        return -1;
    }
    
    AUDIO_LOG("恢复模拟播放");
    
    pthread_mutex_lock(&ctl->control_mutex);
    
    if (!ctl->is_playing || !ctl->is_paused) {
        AUDIO_LOG("当前未暂停");
        pthread_mutex_unlock(&ctl->control_mutex);
        return 0;
    }
    
    ctl->state = AUDIO_CTL_STATE_START;
    ctl->is_paused = false;
    AUDIO_LOG("恢复模拟播放成功");
    
    pthread_mutex_unlock(&ctl->control_mutex);
    return 0;
}

// 停止播放
int audio_ctl_stop(audioctl_s *ctl)
{
    if (!ctl || !ctl->nxplayer) {
        AUDIO_LOG("停止播放失败：控制器或NxPlayer为空");
        return -1;
    }
    
    AUDIO_LOG("停止模拟播放");
    
    pthread_mutex_lock(&ctl->control_mutex);
    
    // 停止监控线程
    if (ctl->monitor_running) {
        ctl->monitor_running = 0;
        ctl->should_stop = true;
        pthread_mutex_unlock(&ctl->control_mutex);
        
        // 等待监控线程结束
        if (ctl->monitor_thread != 0) {
            pthread_join(ctl->monitor_thread, NULL);
            ctl->monitor_thread = 0;
            AUDIO_LOG("监控线程已停止");
        }
        
        pthread_mutex_lock(&ctl->control_mutex);
    }
    
    ctl->state = AUDIO_CTL_STATE_STOP;
    ctl->is_playing = false;
    ctl->is_paused = false;
    ctl->current_position_ms = 0;
    AUDIO_LOG("停止模拟播放成功");
    
    pthread_mutex_unlock(&ctl->control_mutex);
    return 0;
}

// 设置音量
int audio_ctl_set_volume(audioctl_s *ctl, uint16_t vol)
{
    if (!ctl || !ctl->nxplayer) {
        AUDIO_LOG("设置音量失败：控制器或NxPlayer为空");
        return -1;
    }
    
    // 确保音量在有效范围内
    if (vol > 100) {
        vol = 100;
    }
    
    AUDIO_LOG("设置模拟音量: %d", vol);
    return 0;
}

// 获取当前播放位置
int audio_ctl_get_position(audioctl_s *ctl)
{
    if (!ctl) {
        return 0;
    }
    
    pthread_mutex_lock(&ctl->control_mutex);
    uint32_t position_sec = ctl->current_position_ms / 1000;
    pthread_mutex_unlock(&ctl->control_mutex);
    
    return (int)position_sec;
}

// 跳转到指定位置
int audio_ctl_seek(audioctl_s *ctl, unsigned ms)
{
    if (!ctl) {
        AUDIO_LOG("跳转失败：控制器为空");
        return -1;
    }
    
    AUDIO_LOG("模拟跳转到位置: %lu ms", (unsigned long)ms);
    
    pthread_mutex_lock(&ctl->control_mutex);
    
    if (ms <= ctl->total_duration_ms) {
        ctl->current_position_ms = ms;
        AUDIO_LOG("模拟位置更新成功: %lu ms", (unsigned long)ms);
    } else {
        AUDIO_LOG("跳转位置超出文件长度: %lu ms > %lu ms", (unsigned long)ms, (unsigned long)ctl->total_duration_ms);
        pthread_mutex_unlock(&ctl->control_mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&ctl->control_mutex);
    return 0;
}

// 释放音频控制器
int audio_ctl_uninit_nxaudio(audioctl_s *ctl)
{
    if (!ctl) {
        return 0;
    }
    
    AUDIO_LOG("释放模拟器音频控制器");
    
    // 停止播放
    audio_ctl_stop(ctl);
    
    // 模拟器环境下不需要释放真实的NxPlayer
    ctl->nxplayer = NULL;
    
    // 销毁互斥锁
    pthread_mutex_destroy(&ctl->control_mutex);
    
    // 释放内存
    free(ctl);
    
    AUDIO_LOG("模拟器音频控制器释放完成");
    return 0;
}
