//
// 🎵 基于NXPlayer的音频控制器头文件
// 使用Vela系统原生音频播放器，支持MP3播放功能
//

#ifndef AUDIO_CTL_H
#define AUDIO_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/

// 音频格式定义
#define AUDIO_FORMAT_UNKNOWN 0
#define AUDIO_FORMAT_WAV     1
#define AUDIO_FORMAT_MP3     2

// 播放状态定义
#define AUDIO_CTL_STATE_STOP  0
#define AUDIO_CTL_STATE_START 1
#define AUDIO_CTL_STATE_PAUSE 2

/*********************
 *      TYPEDEFS
 *********************/

// WAV文件信息结构
typedef struct {
    uint32_t sample_rate;
    uint16_t num_channels;
    uint16_t bits_per_sample;
    uint32_t data_size;
    uint32_t data_offset;
} wav_s;

// 前向声明 NxPlayer 结构
struct nxplayer_s;

// 音频控制器结构体 - 基于NxPlayer
typedef struct audioctl {
    // 文件信息
    char file_path[512];
    int audio_format;
    off_t file_size;
    
    // NxPlayer 实例 - 使用 Vela 系统音频播放器
    struct nxplayer_s *nxplayer;
    
    // 播放状态控制
    volatile int state;
    volatile int is_playing;
    volatile int is_paused;
    volatile int should_stop;
    pthread_mutex_t control_mutex;
    
    // 播放位置信息
    uint32_t current_position_ms;
    uint32_t total_duration_ms;
    
    // 监控线程
    pthread_t monitor_thread;
    int monitor_running;
    
    // WAV 特有信息（仅WAV格式使用）
    wav_s wav;
    int fd;  // 仅WAV格式使用的文件描述符
    
    // 兼容字段
    int seek;
    uint32_t seek_position;
    uint32_t file_position;
    pthread_t pid;
} audioctl_s;

/*********************
 * GLOBAL PROTOTYPES
 *********************/

/**
 * @brief 检测音频文件格式
 * @param path 音频文件路径
 * @return 音频格式 (AUDIO_FORMAT_*)
 */
int audio_ctl_detect_format(const char *path);

/**
 * @brief 初始化音频控制器（使用NxPlayer）
 * @param path 音频文件路径
 * @return 成功返回控制器指针，失败返回NULL
 */
audioctl_s *audio_ctl_init_nxaudio(const char *path);

/**
 * @brief 开始播放音频
 * @param ctl 音频控制器指针
 * @return 0成功，其他值失败
 */
int audio_ctl_start(audioctl_s *ctl);

/**
 * @brief 暂停播放
 * @param ctl 音频控制器指针
 * @return 0成功，其他值失败
 */
int audio_ctl_pause(audioctl_s *ctl);

/**
 * @brief 恢复播放
 * @param ctl 音频控制器指针
 * @return 0成功，其他值失败
 */
int audio_ctl_resume(audioctl_s *ctl);

/**
 * @brief 停止播放
 * @param ctl 音频控制器指针
 * @return 0成功，其他值失败
 */
int audio_ctl_stop(audioctl_s *ctl);

/**
 * @brief 设置音量
 * @param ctl 音频控制器指针
 * @param vol 音量（0-100）
 * @return 0成功，其他值失败
 */
int audio_ctl_set_volume(audioctl_s *ctl, uint16_t vol);

/**
 * @brief 获取当前播放位置（秒）
 * @param ctl 音频控制器指针
 * @return 播放位置（秒）
 */
int audio_ctl_get_position(audioctl_s *ctl);

/**
 * @brief 跳转到指定位置
 * @param ctl 音频控制器指针
 * @param ms 目标位置（毫秒）
 * @return 0成功，其他值失败
 */
int audio_ctl_seek(audioctl_s *ctl, unsigned ms);

/**
 * @brief 释放音频控制器
 * @param ctl 音频控制器指针
 * @return 0成功，其他值失败
 */
int audio_ctl_uninit_nxaudio(audioctl_s *ctl);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* AUDIO_CTL_H */
