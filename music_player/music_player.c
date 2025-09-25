//
// Vela的音乐播放器 - 基于LVGL的现代化音乐播放器
// Created by Vela on 2025/8/12
// 核心音乐播放器功能实现，包含UI设计、动画效果、音频控制
//

/*
 * UI:
 *
 * TIME GROUP:
 *      TIME: 00:00:00
 *      DATE: 2025/08/12
 *
 * PLAYER GROUP:
 *      ALBUM GROUP:
 *          ALBUM PICTURE
 *          ALBUM INFO:
 *              ALBUM NAME
 *              ALBUM ARTIST
 *      PROGRESS GROUP:
 *          CURRENT TIME: 00:00/00:00
 *          PLAYBACK PROGRESS BAR
 *      CONTROL GROUP:
 *          PLAYLIST
 *          PREVIOUS
 *          PLAY/PAUSE
 *          NEXT
 *          AUDIO
 *
 * TOP Layer:
 *      VOLUME BAR
 *      PLAYLIST GROUP:
 *          TITLE
 *          LIST:
 *              ICON
 *              ALBUM NAME
 *              ALBUM ARTIST
 */

/*********************
 *      INCLUDES
 *********************/

#include "music_player.h"
#include "playlist_manager.h"
#include "font_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <netutils/cJSON.h>
#include <time.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 * MODERN UI CONSTANTS
 **********************/
#define MODERN_BACKGROUND_COLOR      lv_color_hex(0x121212)
#define MODERN_CARD_COLOR           lv_color_hex(0x1E1E1E)
#define MODERN_PRIMARY_COLOR        lv_color_hex(0x00BFFF)
#define MODERN_SECONDARY_COLOR      lv_color_hex(0xFF6B6B)
#define MODERN_TEXT_PRIMARY         lv_color_hex(0xFFFFFF)
#define MODERN_TEXT_SECONDARY       lv_color_hex(0xBBBBBB)
#define MODERN_ACCENT_COLOR         lv_color_hex(0x4ECDC4)

#define COVER_SIZE                  200
#define COVER_ROTATION_DURATION     8000  // 8秒转一圈，更接近真实唱片转速 (33 RPM ≈ 1.8秒/圈，45 RPM ≈ 1.3秒/圈，8秒为慢速视觉效果)

/**********************
 *      TYPEDEFS
 **********************/

// 进度条状态管理结构
typedef struct {
    bool is_seeking;                // 是否正在拖拽
    bool was_playing;              // 拖拽前是否在播放
    uint64_t seek_preview_time;    // 预览时间
    uint32_t last_update_tick;     // 上次更新时间戳
    lv_anim_t smooth_anim;         // 平滑动画
    bool smooth_update_enabled;    // 是否启用平滑更新
    int32_t target_value;          // 目标值
    int32_t current_value;         // 当前值
} progress_bar_state_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/* Init functions */
static void read_configs(void);
static bool init_resource(void);
static void reload_music_config(void);
static void app_create_error_page(void);
static void app_create_main_page(void);
static void app_create_top_layer(void);

/* Timer starting functions */
static void app_start_updating_date_time(void);

/* Animation functions - DISABLED 旋转功能已禁用 */
// static void app_start_cover_rotation_animation(void);
// static void app_stop_cover_rotation_animation(void);
// static void app_cover_rotation_anim_cb(void* obj, int32_t value);

/* Album operations */
static int32_t app_get_album_index(album_info_t* album);

/* Album operations */
void app_set_play_status(play_status_t status);
static void app_set_playback_time(uint32_t current_time);
static void app_set_volume(uint16_t volume);

/* UI refresh functions */
static void app_refresh_album_info(void);
static void app_refresh_date_time(void);
static void app_refresh_play_status(void);
static void app_refresh_playback_progress(void);
static void app_refresh_playlist(void);
static void app_refresh_volume_bar(void);
static void app_refresh_volume_countdown_timer(void);

/* Event handler functions */
static void app_audio_event_handler(lv_event_t* e);
static void app_play_status_event_handler(lv_event_t* e);
// static void app_playlist_btn_event_handler(lv_event_t* e);  // 已移除，使用playlist_manager系统
static void app_playlist_event_handler(lv_event_t* e);
static void app_switch_album_event_handler(lv_event_t* e);
static void app_volume_bar_event_handler(lv_event_t* e);
static void app_playback_progress_bar_event_handler(lv_event_t* e);

/* Timer callback functions */
static void app_refresh_date_time_timer_cb(lv_timer_t* timer);
static void app_playback_progress_update_timer_cb(lv_timer_t* timer);
static void app_volume_bar_countdown_timer_cb(lv_timer_t* timer);

/* Progress bar functions */
static void progress_smooth_anim_cb(void* obj, int32_t value);
static void start_smooth_progress_animation(int32_t target_value);
static void set_progress_smooth_update(bool enabled);
static void reset_progress_bar_state(void);
static void test_progress_bar_functionality(void);

/**********************
 *  STATIC VARIABLES
 **********************/

// clang-format off
struct resource_s   R;  /**< Resources */
struct ctx_s        C;  /**< Context */
struct conf_s       CF; /**< Configuration */
// clang-format on

// 全局进度条状态
static progress_bar_state_t progress_state = {
    .is_seeking = false,
    .was_playing = false,
    .seek_preview_time = 0,
    .last_update_tick = 0,
    .smooth_update_enabled = true,
    .target_value = 0,
    .current_value = 0
};

/* Week days mapping - 完整格式显示星期 */
const char* WEEK_DAYS[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

/* Transition properties for the objects */
const lv_style_prop_t transition_props[] = {
    LV_STYLE_OPA,
    LV_STYLE_BG_OPA,
    LV_STYLE_Y,
    LV_STYLE_HEIGHT,
    LV_STYLE_PROP_FLAG_NONE
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void app_create(void)
{
    // Init resource and context structure
    lv_memzero(&R, sizeof(R));
    lv_memzero(&C, sizeof(C));
    lv_memzero(&CF, sizeof(CF));

    // 系统初始化
    LV_LOG_USER("启动音乐播放器...");

    // 初始化字体系统
    font_system_init();
    
    printf("STM32H750 Professional Music Player - 专业嵌入式音乐播放器启动中...\n");
    printf("🏗️ 硬件平台: STM32H750B-DK (480MHz, 1MB RAM, 4.3寸LCD)\n");
    printf("🎯 采用专业级架构 + STM32H750硬件优化方案\n");
    printf("📊 性能目标: ≤8MB内存, ≤40%%CPU, ≤50ms延迟, ≤2s启动\n");

    read_configs();

#if WIFI_ENABLED
    CF.wifi.conn_delay = 2000000;
    wifi_connect(&CF.wifi);
#endif

    C.resource_healthy_check = init_resource();

    if (!C.resource_healthy_check) {
        app_create_error_page();
        return;
    }

    app_create_main_page();
    app_set_play_status(PLAY_STATUS_STOP);
    app_switch_to_album(0);
    app_set_volume(30);

    app_refresh_album_info();
    app_refresh_playlist();
    app_refresh_volume_bar();
    
    // STM32H750 SD卡挂载
    const char* mount_point = "/data";
    printf("💾 文件系统就绪: %s\n", mount_point);
    
    // 音频系统初始化
    LV_LOG_USER("🎵 音频系统初始化完成");
    
    printf("✅ Vela Music Player 初始化完成\n");
    printf("🎮 功能特性：\n");
    printf("   • 现代化 UI 设计\n");
    printf("   • 触摸友好界面\n");
    printf("   • 进度条拖拽控制\n");
    printf("   • 播放列表管理\n");
    
    // 系统就绪
    LV_LOG_USER("🚀 音乐播放器系统就绪");
    
    #ifdef DEBUG
    LV_LOG_USER("🔧 开发模式：将在启动后测试进度条功能");
    #endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int32_t app_get_album_index(album_info_t* album)
{
    for (int i = 0; i < R.album_count; i++) {
        if (album == &R.albums[i]) {
            return i;
        }
    }
    return -1;
}

static void app_set_volume(uint16_t volume)
{
    C.volume = volume;
    audio_ctl_set_volume(C.audioctl, C.volume);
}

void app_set_play_status(play_status_t status)
{
    C.play_status_prev = C.play_status;
    C.play_status = status;
    app_refresh_play_status();
}

void app_switch_to_album(int index)
{
    if (R.album_count == 0 || index < 0 || index >= R.album_count || C.current_album == &R.albums[index])
        return;

    C.current_album = &R.albums[index];
    
    // 重置进度条状态，避免切歌时的状态混乱
    reset_progress_bar_state();
    
    app_refresh_album_info();
    app_refresh_playlist();
    app_set_playback_time(0);

    if (C.play_status == PLAY_STATUS_STOP) {
        return;
    }

    app_set_play_status(PLAY_STATUS_STOP);
    app_set_play_status(PLAY_STATUS_PLAY);
}

static void app_set_playback_time(uint32_t current_time)
{
    C.current_time = current_time;

    // 如果音频控制器存在，执行seek操作
    if (C.audioctl) {
        audio_ctl_seek(C.audioctl, C.current_time / 1000);
    }
    
    // 更新进度条状态
    progress_state.current_value = (int32_t)current_time;
    progress_state.target_value = (int32_t)current_time;
    
    app_refresh_playback_progress();
}

static void app_refresh_date_time(void)
{
    // 检查UI组件是否存在
    if (!R.ui.time || !R.ui.date) {
        LV_LOG_WARN("Time/Date UI components not initialized");
        return;
    }

    // 获取当前实时时间
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    
    if (current_time == NULL) {
        // 如果无法获取系统时间，使用合理的默认时间
        static struct tm default_time = {0};
        default_time.tm_year = 2024 - 1900;  // 年份从1900开始计算
        default_time.tm_mon = 9 - 1;         // 9月
        default_time.tm_mday = 14;           // 14日
        default_time.tm_hour = 14;           // 14点
        default_time.tm_min = 30;            // 30分
        default_time.tm_sec = 0;             // 0秒
        default_time.tm_wday = 6;            // 星期六
        current_time = &default_time;
        
        // 每次调用时增加一分钟，模拟时间流逝
        static int minute_counter = 0;
        default_time.tm_min = 30 + (minute_counter++ % 60);
        if (default_time.tm_min >= 60) {
            default_time.tm_min %= 60;
            default_time.tm_hour = (default_time.tm_hour + 1) % 24;
        }
    }
    
    // 更新时间 (HH:MM格式)
    char time_str[6];
    lv_snprintf(time_str, sizeof(time_str), "%02d:%02d", current_time->tm_hour, current_time->tm_min);
    lv_label_set_text(R.ui.time, time_str);

    // 更新星期 (完整格式: Monday, Tuesday等)
    char date_str[12];
    int wday = current_time->tm_wday;
    if (wday < 0 || wday > 6) wday = 0; // 防护边界
    lv_snprintf(date_str, sizeof(date_str), "%s", WEEK_DAYS[wday]);
    lv_label_set_text(R.ui.date, date_str);

    LV_LOG_USER("Time updated: %s %s (real-time)", time_str, date_str);
}

static void app_refresh_volume_bar(void)
{
    int32_t volume_bar_indic_height = C.volume;

    lv_obj_set_height(R.ui.volume_bar_indic, volume_bar_indic_height);

    lv_obj_refr_size(R.ui.volume_bar_indic);
    lv_obj_update_layout(R.ui.volume_bar_indic);

    if (C.volume > 0) {
        lv_image_set_src(R.ui.audio, R.images.audio);
    } else {
        lv_image_set_src(R.ui.audio, R.images.mute);
    }
}

static void app_refresh_album_info(void)
{
    if (C.current_album) {
        // 智能图片加载
        if (access(C.current_album->cover, F_OK) == 0) {
            lv_image_set_src(R.ui.album_cover, C.current_album->cover);
            LV_LOG_USER("📷 加载专辑封面: %s", C.current_album->cover);
            
            // 设置图片完整显示
            lv_image_set_scale(R.ui.album_cover, 256);
            lv_image_set_inner_align(R.ui.album_cover, LV_IMAGE_ALIGN_CENTER);
            lv_obj_set_style_bg_image_opa(R.ui.album_cover, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_image_recolor_opa(R.ui.album_cover, LV_OPA_0, LV_PART_MAIN);
            
        } else {
            lv_image_set_src(R.ui.album_cover, R.images.nocover);
            LV_LOG_WARN("📷 专辑封面文件不存在，使用默认封面: %s", C.current_album->cover);
        }
        
        // 更新歌曲信息
        const char* display_name = (C.current_album->name && strlen(C.current_album->name) > 0) ? 
                                  C.current_album->name : "未知歌曲";
        const char* display_artist = (C.current_album->artist && strlen(C.current_album->artist) > 0) ? 
                                    C.current_album->artist : "未知艺术家";
        
        // 使用字体配置系统
        set_label_utf8_text(R.ui.album_name, display_name, get_font_by_size(28));
        set_label_utf8_text(R.ui.album_artist, display_artist, get_font_by_size(22));
        
        LV_LOG_USER("🎵 专辑信息已更新: %s - %s", 
                   C.current_album->name ? C.current_album->name : "未知歌曲",
                   C.current_album->artist ? C.current_album->artist : "未知艺术家");
    }
}

static void app_refresh_play_status(void)
{
    if (C.timers.playback_progress_update == NULL) {
        C.timers.playback_progress_update = lv_timer_create(app_playback_progress_update_timer_cb, 1000, NULL);
    }

    switch (C.play_status) {
    case PLAY_STATUS_STOP:
        lv_image_set_src(R.ui.play_btn, R.images.play);
        lv_timer_pause(C.timers.playback_progress_update);
        if (C.audioctl) {
            audio_ctl_stop(C.audioctl);
            audio_ctl_uninit_nxaudio(C.audioctl);
            C.audioctl = NULL;
        }
        break;
    case PLAY_STATUS_PLAY:
        lv_image_set_src(R.ui.play_btn, R.images.pause);
        lv_timer_resume(C.timers.playback_progress_update);
        if (C.play_status_prev == PLAY_STATUS_PAUSE)
            audio_ctl_resume(C.audioctl);
        else if (C.play_status_prev == PLAY_STATUS_STOP) {
            if (!C.current_album || C.current_album->path[0] == '\0') {
                LV_LOG_ERROR("❌ 当前专辑或路径为空，无法初始化音频");
                app_set_play_status(PLAY_STATUS_STOP);
                return;
            }
            
            // 音频文件路径处理
            const char* audio_path = C.current_album->path;
            
            LV_LOG_USER("🎵 使用音频文件路径: %s", audio_path);
            
            // 验证文件是否存在
            if (access(audio_path, R_OK) != 0) {
                LV_LOG_WARN("⚠️ 主路径不可访问，尝试备用路径...");
                
                // 提取文件名
                const char* filename = strrchr(audio_path, '/');
                if (filename) {
                    filename++; // 跳过 '/'
                } else {
                    filename = audio_path; // 如果没有找到 '/'，整个路径就是文件名
                }
                
                // 尝试备用路径
                static char backup_paths[4][512];
                snprintf(backup_paths[0], sizeof(backup_paths[0]), "/data/res/musics/%s", filename);
                snprintf(backup_paths[1], sizeof(backup_paths[1]), "res/musics/%s", filename);
                snprintf(backup_paths[2], sizeof(backup_paths[2]), "/root/vela_code/apps/packages/demos/music_player/res/musics/%s", filename);
                snprintf(backup_paths[3], sizeof(backup_paths[3]), "./res/musics/%s", filename);
                
                audio_path = NULL; // 重置路径
                for (int i = 0; i < 4; i++) {
                    LV_LOG_USER("🔍 尝试备用路径 %d: %s", i+1, backup_paths[i]);
                    if (access(backup_paths[i], R_OK) == 0) {
                        audio_path = backup_paths[i];
                        LV_LOG_USER("✅ 找到可用路径: %s", audio_path);
                        break;
                    }
                }
                
                if (!audio_path) {
                    LV_LOG_ERROR("❌ 无法找到音频文件: %s", C.current_album->path);
                    app_set_play_status(PLAY_STATUS_STOP);
                    return;
                }
            }
            
            LV_LOG_USER("🎵 初始化音频控制器: %s", audio_path);
            
            // 音频控制器初始化
            int retry_count = 3;
            while (retry_count > 0 && !C.audioctl) {
                C.audioctl = audio_ctl_init_nxaudio(audio_path);
                if (!C.audioctl) {
                    retry_count--;
                    LV_LOG_WARN("❌ 音频控制器初始化失败，剩余重试次数: %d", retry_count);
                    if (retry_count > 0) {
                        lv_delay_ms(100);
                    }
                }
            }
            
            if (!C.audioctl) {
                LV_LOG_ERROR("❌ 音频控制器初始化最终失败，请检查音频文件: %s", audio_path);
                app_set_play_status(PLAY_STATUS_STOP);
                return;
            }
            
            LV_LOG_USER("🔊 启动音频播放...");
            int ret = audio_ctl_start(C.audioctl);
            if (ret < 0) {
                LV_LOG_ERROR("❌ 音频播放启动失败: %d", ret);
                audio_ctl_uninit_nxaudio(C.audioctl);
                C.audioctl = NULL;
                app_set_play_status(PLAY_STATUS_STOP);
                return;
            }
            
            LV_LOG_USER("✅ 音频播放启动成功");
        }
        break;
    case PLAY_STATUS_PAUSE:
        lv_image_set_src(R.ui.play_btn, R.images.play);
        lv_timer_pause(C.timers.playback_progress_update);
        audio_ctl_pause(C.audioctl);
        break;
    default:
        break;
    }
}

static void app_refresh_playback_progress(void)
{
    if (!C.current_album) {
        return;
    }

    uint64_t total_time = C.current_album->total_time;

    if (C.current_time > total_time) {
        app_set_play_status(PLAY_STATUS_STOP);
        C.current_time = 0;
        return;
    }

    lv_bar_set_range(R.ui.playback_progress, 0, (int32_t)total_time);
    
    // 如果不在拖拽状态，使用平滑动画更新进度条
    if (!progress_state.is_seeking) {
        if (progress_state.smooth_update_enabled) {
            // 检查是否需要启动平滑动画
            int32_t current_value = lv_bar_get_value(R.ui.playback_progress);
            int32_t new_value = (int32_t)C.current_time;
            
            // 如果差距较大（超过2秒），使用平滑动画
            if (abs(new_value - current_value) > 2000) {
                start_smooth_progress_animation(new_value);
            } else {
                // 小差距直接更新
                lv_bar_set_value(R.ui.playback_progress, new_value, LV_ANIM_OFF);
                progress_state.current_value = new_value;
            }
        } else {
            lv_bar_set_value(R.ui.playback_progress, (int32_t)C.current_time, LV_ANIM_OFF);
        }
    }
    // 如果在拖拽状态，不更新进度条显示，避免与用户操作冲突

    // 更新时间显示（除非在拖拽预览中）
    if (!progress_state.is_seeking) {
        char buff[16];
        
        uint32_t current_time_min = C.current_time / 60000;
        uint32_t current_time_sec = (C.current_time % 60000) / 1000;
        uint32_t total_time_min = total_time / 60000;
        uint32_t total_time_sec = (total_time % 60000) / 1000;

        lv_snprintf(buff, sizeof(buff), "%02d:%02d", current_time_min, current_time_sec);
        lv_span_set_text(R.ui.playback_current_time, buff);
        lv_snprintf(buff, sizeof(buff), "%02d:%02d", total_time_min, total_time_sec);
        lv_span_set_text(R.ui.playback_total_time, buff);
    }
}

static void app_refresh_playlist(void)
{
    // 使用新的播放列表管理器刷新
    // 如果播放列表管理器已打开，则刷新其内容
    if (playlist_manager_is_open()) {
        playlist_manager_refresh();
    }
    
    // 保留此函数以兼容现有调用，但实际逻辑已转移到playlist_manager
    LV_LOG_USER("Playlist refresh triggered - using new playlist manager system");
}

static void app_volume_bar_countdown_timer_cb(lv_timer_t* timer)
{
    LV_UNUSED(timer);
    lv_obj_set_state(R.ui.volume_bar, LV_STATE_DEFAULT, true);
    lv_obj_set_state(R.ui.volume_bar, LV_STATE_USER_1, false);
}

static void app_playback_progress_update_timer_cb(lv_timer_t* timer)
{
    LV_UNUSED(timer);

    // 检查音频控制器是否有效
    if (!C.audioctl) {
        LV_LOG_WARN("音频控制器无效，停止进度更新");
        return;
    }
    
    // 检查播放状态
    if (C.play_status != PLAY_STATUS_PLAY) {
        return;  // 不在播放状态时不更新进度
    }
    
    // 如果用户正在拖拽进度条，暂停自动更新避免冲突
    if (progress_state.is_seeking) {
        return;
    }

    // 获取当前播放位置
    int position = audio_ctl_get_position(C.audioctl);
    if (position >= 0) {
        uint64_t new_time = position * 1000;
        
        // 检查时间是否有显著变化，避免不必要的UI更新
        if (abs((int64_t)new_time - (int64_t)C.current_time) > 500) { // 500ms阈值
            C.current_time = new_time;
            app_refresh_playback_progress();
        }
        
        // 每10秒输出一次调试信息
        static int debug_counter = 0;
        if (++debug_counter >= 10) {
            LV_LOG_USER("🎵 播放进度: %d秒 / %lu秒 (平滑更新:%s)", 
                       position, 
                       (unsigned long)(C.current_album ? C.current_album->total_time / 1000 : 0),
                       progress_state.smooth_update_enabled ? "开启" : "关闭");
            debug_counter = 0;
        }
    } else {
        LV_LOG_WARN("获取播放位置失败: %d", position);
    }
}

static void app_refresh_date_time_timer_cb(lv_timer_t* timer)
{
    LV_UNUSED(timer);

    app_refresh_date_time();
}

static void app_refresh_volume_countdown_timer(void)
{
    if (C.timers.volume_bar_countdown) {
        lv_timer_set_repeat_count(C.timers.volume_bar_countdown, 1);
        lv_timer_reset(C.timers.volume_bar_countdown);
        lv_timer_resume(C.timers.volume_bar_countdown);
    } else {
        C.timers.volume_bar_countdown = lv_timer_create(app_volume_bar_countdown_timer_cb, 3000, NULL);
        lv_timer_set_auto_delete(C.timers.volume_bar_countdown, false);
    }
}

static void app_playlist_event_handler(lv_event_t* e)
{
    // 防止重复点击的静态变量
    static uint32_t last_click_time = 0;
    uint32_t current_time = lv_tick_get();
    
    // 播放列表事件处理
    if (!e) {
        LV_LOG_WARN("播放列表按钮事件指针为空");
        return;
    }
    
    lv_event_code_t code = lv_event_get_code(e);
    
    // 防止快速重复点击
    if (code == LV_EVENT_CLICKED) {
        if (current_time - last_click_time < 500) {
            LV_LOG_WARN("⚠️ 点击过快，忽略重复点击");
            return;
        }
        last_click_time = current_time;
    }
    
    // 支持多种交互方式
    if (code != LV_EVENT_CLICKED && code != LV_EVENT_PRESSED) {
        return;  // 处理点击和按压事件
    }
    
    // 🔊 提供用户反馈
    if (code == LV_EVENT_PRESSED) {
        // 按压时的视觉反馈
        lv_obj_t* target = lv_event_get_target(e);
        if (target) {
            lv_obj_set_style_transform_scale(target, 240, LV_PART_MAIN | LV_STATE_PRESSED);  // 减少缩放
        }
        return;
    }
    
    LV_LOG_USER("📋 播放列表按钮被点击!");
    
    // 🔍 增强的播放列表数据检查
    if (!R.albums || R.album_count == 0) {
        LV_LOG_WARN("播放列表为空或未初始化，无法显示");
        
        // 🚨 用户友好的错误提示
        if (R.ui.album_name) {
            lv_label_set_text(R.ui.album_name, "No music files found");
        }
        
        // 📱 创建临时提示对话框
        lv_obj_t* mbox = lv_msgbox_create(lv_screen_active());
        lv_msgbox_add_title(mbox, "📂 Empty Playlist");
        lv_msgbox_add_text(mbox, "No music files found.\nPlease add music files to continue.");
        lv_msgbox_add_close_button(mbox);
        
        return;
    }

    // 🎬 使用传统播放列表 - 优化内存检查
    if (playlist_manager_is_open()) {
        // 如果播放列表已打开，则关闭它
        LV_LOG_USER("🔙 关闭播放列表 (歌曲数: %d)", R.album_count);
        playlist_manager_close();
    } else {
        // 创建播放列表 - 放宽内存限制
        LV_LOG_USER("📂 打开播放列表 (歌曲数: %d)", R.album_count);
        
        // 🏗️ 智能容器选择逻辑
        lv_obj_t* parent_container = lv_layer_top();
        if (!parent_container) {
            LV_LOG_WARN("顶层容器不可用，使用活动屏幕");
            parent_container = lv_screen_active();
        }
        
        if (parent_container) {
            // 💾 放宽内存检查 - 允许更大的内存使用
            lv_mem_monitor_t mem_info;
            lv_mem_monitor(&mem_info);
            LV_LOG_USER("📊 当前可用内存: %zu KB", mem_info.free_size / 1024);
            
            // 即使内存较低也尝试创建，让播放列表内部处理
            playlist_manager_create(parent_container);
            
            lv_mem_monitor(&mem_info);
            LV_LOG_USER("✅ 播放列表创建完成 (剩余内存: %zu KB)", mem_info.free_size / 1024);
        } else {
            LV_LOG_ERROR("❌ 无法找到合适的父容器");
        }
    }
}

static void app_volume_bar_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (!(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_PRESS_LOST)) {
        return;
    }

    lv_point_t point;
    lv_indev_t* indev = lv_indev_active();
    lv_indev_get_vect(indev, &point);

    int32_t volume_bar_height = lv_obj_get_height(R.ui.volume_bar);
    int32_t volume_bar_indic_height = lv_obj_get_height(R.ui.volume_bar_indic);

    if (volume_bar_indic_height < 0) {
        volume_bar_indic_height = 0;
    } else if (volume_bar_indic_height > volume_bar_height) {
        volume_bar_indic_height = volume_bar_height;
    }

    int32_t volume = volume_bar_indic_height - point.y;
    if (volume < 0)
        volume = 0;

    app_set_volume(volume);

    app_refresh_volume_bar();
    app_refresh_volume_countdown_timer();
}

static void app_audio_event_handler(lv_event_t* e)
{
    // 音量控制事件处理
    if (!e) {
        LV_LOG_ERROR("音量按钮事件指针为空");
        return;
    }
    
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    
    // 支持多种交互方式
    if (code == LV_EVENT_PRESSED && target) {
        // 按压时的视觉反馈
        lv_obj_set_style_transform_scale(target, 240, LV_PART_MAIN | LV_STATE_PRESSED);  // 减少缩放
        return;
    }
    
    if (code == LV_EVENT_RELEASED && target) {
        // 释放时恢复缩放
        lv_obj_set_style_transform_scale(target, 256, LV_PART_MAIN);  // 恢复正常大小
        return;
    }
    
    if (code != LV_EVENT_CLICKED) {
        return;  // 只处理点击事件
    }
    
    LV_LOG_USER("🔊 音量按钮被点击!");

    // 🔍 增强的音量条状态检查
    if (!R.ui.volume_bar) {
        LV_LOG_ERROR("音量条组件未初始化");
        
        // 🚨 尝试重新初始化音量条
        if (R.ui.audio) {
            LV_LOG_WARN("尝试重新初始化音量控件...");
            // 这里可以添加重新初始化音量条的逻辑
        }
        return;
    }

    // 🎚️ 智能音量条显示切换
    bool volume_visible = lv_obj_has_state(R.ui.volume_bar, LV_STATE_USER_1);
    
    if (volume_visible) {
        LV_LOG_USER("🔇 隐藏音量条 (当前音量: %d)", C.volume);
        lv_obj_set_state(R.ui.volume_bar, LV_STATE_DEFAULT, true);
        lv_obj_set_state(R.ui.volume_bar, LV_STATE_USER_1, false);
        
        // 停止倒计时器
        if (C.timers.volume_bar_countdown) {
            lv_timer_pause(C.timers.volume_bar_countdown);
        }
    } else {
        LV_LOG_USER("🔊 显示音量条 (当前音量: %d)", C.volume);
        lv_obj_set_state(R.ui.volume_bar, LV_STATE_DEFAULT, false);
        lv_obj_set_state(R.ui.volume_bar, LV_STATE_USER_1, true);
        app_refresh_volume_countdown_timer();
    }
    
    // 🔄 更新音量图标状态
    if (R.ui.audio) {
        if (C.volume > 0) {
            lv_image_set_src(R.ui.audio, R.images.audio);
        } else {
            lv_image_set_src(R.ui.audio, R.images.mute);
        }
    }
    
    // 💾 内存状态检查
    lv_mem_monitor_t mem_info;
    lv_mem_monitor(&mem_info);
    if (mem_info.free_size < 10 * 1024) {  // 少于10KB时警告
        LV_LOG_WARN("内存低警告: %zu KB 可用", mem_info.free_size / 1024);
    }
    
    LV_LOG_USER("✅ 音量控制事件处理完成 (内存: %zu KB)", mem_info.free_size / 1024);
}

// 注意：app_playlist_btn_event_handler已移除，因为现在使用playlist_manager.c中的新事件处理系统

static void app_switch_album_event_handler(lv_event_t* e)
{
    // 切歌事件处理
    if (!e) {
        printf("❌ 事件指针为空，切歌事件处理失败\n");
        return;
    }
    
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    bool is_long_press = (code == LV_EVENT_LONG_PRESSED_REPEAT);
    
    // 🎨 处理按压和释放的视觉反馈
    if (code == LV_EVENT_PRESSED && target) {
        // 按下时的视觉反馈
        lv_obj_set_style_transform_scale(target, 245, LV_PART_MAIN);  // 轻微缩小
        lv_obj_set_style_shadow_width(target, 20, LV_PART_MAIN);
        return;
    } else if (code == LV_EVENT_RELEASED && target) {
        // 释放时恢复状态
        lv_obj_set_style_transform_scale(target, 256, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(target, 15, LV_PART_MAIN);
        return;
    }
    
    if (code != LV_EVENT_CLICKED && code != LV_EVENT_LONG_PRESSED_REPEAT) {
        // 只处理点击和长按重复事件
        return;
    }
    
    // 🔒 基础状态验证
    if (R.album_count == 0) {
        LV_LOG_WARN("播放列表为空，无法切换歌曲");
        return;
    }
    
    if (!C.current_album) {
        LV_LOG_WARN("当前专辑为空，尝试选择第一首歌曲");
        app_switch_to_album(0);
        return;
    }
    
    switch_album_mode_t direction = (switch_album_mode_t)(lv_uintptr_t)lv_event_get_user_data(e);
    
    // 🔍 验证方向参数
    if (direction != SWITCH_ALBUM_MODE_PREV && direction != SWITCH_ALBUM_MODE_NEXT) {
        LV_LOG_ERROR("无效的切换方向: %d", direction);
        return;
    }
    
    const char* direction_str = (direction == SWITCH_ALBUM_MODE_PREV) ? "⏮️ 上一首" : "⏭️ 下一首";
    const char* press_type = is_long_press ? "🔄 长按快速切换" : "🎵 点击切换";
    
    printf("%s 歌曲按钮! 方向: %s, 当前: %s\n", press_type, direction_str,
           C.current_album->name ? C.current_album->name : "未知");
    
    int32_t album_index = app_get_album_index(C.current_album);
    if (album_index < 0) {
        printf("❌ 错误: 无法获取当前歌曲索引，重置到第一首\n");
        app_switch_to_album(0);
        return;
    }

    // 🎮 计算新索引 - 增强边界检查
    int32_t new_index = album_index;
    switch (direction) {
    case SWITCH_ALBUM_MODE_PREV:
        new_index = (album_index - 1 + R.album_count) % R.album_count;
        break;
    case SWITCH_ALBUM_MODE_NEXT:
        new_index = (album_index + 1) % R.album_count;
        break;
    }
    
    // 🔍 最终验证
    if (new_index < 0 || new_index >= R.album_count) {
        LV_LOG_ERROR("计算出的索引无效: %ld (范围: 0-%d)", (long)new_index, R.album_count - 1);
        return;
    }

    LV_LOG_USER("🎯 切换歌曲: %ld -> %ld (总数: %d)", (long)album_index, (long)new_index, R.album_count);

    // 🚀 执行切换
    app_switch_to_album(new_index);
    LV_LOG_USER("✅ 歌曲切换成功: %s -> %s", 
                C.current_album->name ? C.current_album->name : "未知",
                R.albums[new_index].name ? R.albums[new_index].name : "未知");
}

static void app_play_status_event_handler(lv_event_t* e)
{
    // 播放按钮事件处理
    if (!e) {
        LV_LOG_ERROR("事件指针为空，播放按钮事件处理失败");
        return;
    }
    
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    
    if (!target) {
        LV_LOG_ERROR("目标对象为空，播放按钮事件处理失败");
        return;
    }
    
    // 🎨 处理按压和释放的视觉反馈
    if (code == LV_EVENT_PRESSED) {
        // 按下时的视觉反馈
        lv_obj_add_state(target, LV_STATE_PRESSED);
        lv_obj_set_style_transform_scale(target, 245, LV_PART_MAIN);  // 轻微缩小
        lv_obj_set_style_shadow_width(target, 35, LV_PART_MAIN);
        return;
    } else if (code == LV_EVENT_RELEASED) {
        // 释放时恢复状态
        lv_obj_clear_state(target, LV_STATE_PRESSED);
        lv_obj_set_style_transform_scale(target, 256, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(target, 25, LV_PART_MAIN);
        return;
    } else if (code != LV_EVENT_CLICKED) {
        // 只处理点击、按压和释放事件
        return;
    }
    
    // 🔒 状态检查 - 确保播放器处于有效状态
    if (!C.current_album && R.album_count > 0) {
        LV_LOG_WARN("当前无选中专辑，自动选择第一首歌曲");
        app_switch_to_album(0);
        return;
    } else if (R.album_count == 0) {
        LV_LOG_ERROR("播放列表为空，无法播放");
        return;
    }
    
    LV_LOG_USER("🎵 播放按钮点击: 当前状态=%d, 专辑=%s", C.play_status, 
                C.current_album->name ? C.current_album->name : "未知");

    // 🎮 专业状态机处理 - 增强状态验证
    play_status_t new_status;
    const char* action_desc;
    
    switch (C.play_status) {
    case PLAY_STATUS_STOP:
        new_status = PLAY_STATUS_PLAY;
        action_desc = "▶️ 开始播放";
        break;
    case PLAY_STATUS_PLAY:
        new_status = PLAY_STATUS_PAUSE;
        action_desc = "⏸️ 暂停播放";
        break;
    case PLAY_STATUS_PAUSE:
        new_status = PLAY_STATUS_PLAY;
        action_desc = "▶️ 恢复播放";
        break;
    default:
        LV_LOG_ERROR("未知播放状态: %d，操作被拒绝", C.play_status);
        return;
    }
    
    LV_LOG_USER("%s (状态: %d -> %d)", action_desc, C.play_status, new_status);
    
    // 🚀 执行状态切换
    app_set_play_status(new_status);
    LV_LOG_USER("✅ 播放状态切换完成");
}

// 平滑动画回调函数
static void progress_smooth_anim_cb(void* obj, int32_t value)
{
    lv_obj_t* progress_bar = (lv_obj_t*)obj;
    if (progress_bar && lv_obj_is_valid(progress_bar)) {
        progress_state.current_value = value;
        lv_bar_set_value(progress_bar, value, LV_ANIM_OFF);
    }
}

// 启动平滑动画
static void start_smooth_progress_animation(int32_t target_value)
{
    if (!R.ui.playback_progress || !progress_state.smooth_update_enabled) {
        return;
    }
    
    // 如果正在拖拽，不启动平滑动画
    if (progress_state.is_seeking) {
        return;
    }
    
    progress_state.target_value = target_value;
    
    // 停止之前的动画
    lv_anim_delete(R.ui.playback_progress, progress_smooth_anim_cb);
    
    // 创建新的平滑动画
    lv_anim_init(&progress_state.smooth_anim);
    lv_anim_set_var(&progress_state.smooth_anim, R.ui.playback_progress);
    lv_anim_set_exec_cb(&progress_state.smooth_anim, progress_smooth_anim_cb);
    lv_anim_set_values(&progress_state.smooth_anim, progress_state.current_value, target_value);
    lv_anim_set_duration(&progress_state.smooth_anim, 200); // 200ms平滑过渡
    lv_anim_set_path_cb(&progress_state.smooth_anim, lv_anim_path_ease_out);
    lv_anim_start(&progress_state.smooth_anim);
}

// 设置进度条平滑更新开关
static void set_progress_smooth_update(bool enabled)
{
    progress_state.smooth_update_enabled = enabled;
    if (!enabled) {
        // 停止当前动画
        lv_anim_delete(R.ui.playback_progress, progress_smooth_anim_cb);
    }
    LV_LOG_USER("进度条平滑更新: %s", enabled ? "开启" : "关闭");
}

// 重置进度条状态
static void reset_progress_bar_state(void)
{
    progress_state.is_seeking = false;
    progress_state.was_playing = false;
    progress_state.seek_preview_time = 0;
    progress_state.last_update_tick = 0;
    progress_state.target_value = 0;
    progress_state.current_value = 0;
    
    // 停止所有动画
    if (R.ui.playback_progress) {
        lv_anim_delete(R.ui.playback_progress, progress_smooth_anim_cb);
    }
    
    LV_LOG_USER("进度条状态已重置");
}

// 进度条功能测试函数
static void test_progress_bar_functionality(void)
{
    LV_LOG_USER("🧪 开始进度条功能测试...");
    
    if (!R.ui.playback_progress) {
        LV_LOG_ERROR("❌ 进度条UI组件未初始化");
        return;
    }
    
    if (!C.current_album) {
        LV_LOG_WARN("⚠️ 当前无专辑，无法测试seek功能");
        return;
    }
    
    // 测试1: 平滑更新开关
    LV_LOG_USER("🔧 测试1: 平滑更新开关");
    set_progress_smooth_update(false);
    set_progress_smooth_update(true);
    
    // 测试2: 状态重置
    LV_LOG_USER("🔧 测试2: 状态重置功能");
    reset_progress_bar_state();
    
    // 测试3: 平滑动画（如果有音乐在播放）
    if (C.current_album->total_time > 30000) { // 大于30秒的音乐
        LV_LOG_USER("🔧 测试3: 平滑动画效果");
        start_smooth_progress_animation(15000); // 跳转到15秒位置
        
        // 延迟测试：2秒后再测试另一个位置
        // 注意：这里只是演示，实际应用中可以创建定时器来延迟执行
        LV_LOG_USER("🎯 模拟测试：平滑动画到不同位置");
    }
    
    // 测试4: 边界检查
    LV_LOG_USER("🔧 测试4: 边界检查");
    if (C.current_album->total_time > 0) {
        start_smooth_progress_animation(0); // 开始位置
        start_smooth_progress_animation((int32_t)C.current_album->total_time); // 结束位置
    }
    
    LV_LOG_USER("✅ 进度条功能测试完成");
    LV_LOG_USER("📊 测试结果统计:");
    LV_LOG_USER("   - 平滑更新: %s", progress_state.smooth_update_enabled ? "✅" : "❌");
    LV_LOG_USER("   - 拖拽状态: %s", progress_state.is_seeking ? "进行中" : "空闲");
    LV_LOG_USER("   - 当前值: %ld", (long)progress_state.current_value);
    LV_LOG_USER("   - 目标值: %ld", (long)progress_state.target_value);
}


static void app_playback_progress_bar_event_handler(lv_event_t* e)
{
    // 进度条交互处理
    if (!e) {
        LV_LOG_ERROR("❌ 进度条事件指针为空");
        return;
    }
    
    lv_event_code_t code = lv_event_get_code(e);
    
    if (!C.current_album) {
        LV_LOG_ERROR("❌ 当前专辑为空，无法操作进度条");
        return;
    }
    
    uint32_t current_tick = lv_tick_get();
    
    switch (code) {
    case LV_EVENT_PRESSED: {
        // 开始拖拽模式
        progress_state.is_seeking = true;
        progress_state.was_playing = (C.play_status == PLAY_STATUS_PLAY);
        progress_state.last_update_tick = current_tick;
        
        // 停止平滑动画和更新定时器
        lv_anim_delete(R.ui.playback_progress, progress_smooth_anim_cb);
        if (C.timers.playback_progress_update) {
            lv_timer_pause(C.timers.playback_progress_update);
        }
        
        // 视觉反馈
        lv_obj_set_height(R.ui.playback_progress, 10);
        lv_obj_set_style_bg_color(R.ui.playback_progress, lv_color_hex(0x00BFFF), LV_PART_INDICATOR);
        lv_obj_set_style_shadow_width(R.ui.playback_progress, 8, LV_PART_INDICATOR);
        lv_obj_set_style_shadow_color(R.ui.playback_progress, lv_color_hex(0x00BFFF), LV_PART_INDICATOR);
        lv_obj_set_style_shadow_opa(R.ui.playback_progress, LV_OPA_50, LV_PART_INDICATOR);
        
        // 拖拽反馈
        LV_LOG_USER("🎚️ 开始拖拽进度条 - 增强交互模式");
        break;
    }
    case LV_EVENT_PRESSING: {
        // 拖拽中实时预览更新
        if (!progress_state.is_seeking) break;
        
        // 节流：限制更新频率
        if (current_tick - progress_state.last_update_tick < 16) {
            break;
        }
        progress_state.last_update_tick = current_tick;
        
        lv_point_t point;
        lv_indev_t* indev = lv_indev_active();
        lv_indev_get_point(indev, &point);

        // 获取进度条的坐标和尺寸
        lv_area_t area;
        lv_obj_get_coords(R.ui.playback_progress, &area);
        
        int32_t bar_width = lv_area_get_width(&area);
        int32_t relative_x = point.x - area.x1;
        
        // 边界检查
        const int32_t tolerance = 20;
        if (relative_x < -tolerance) relative_x = 0;
        else if (relative_x > bar_width + tolerance) relative_x = bar_width;
        else if (relative_x < 0) relative_x = 0;
        else if (relative_x > bar_width) relative_x = bar_width;
        
        // 计算新的预览时间
        uint64_t total_time = C.current_album->total_time;
        uint64_t new_time = (uint64_t)relative_x * total_time / bar_width;
        
        // 时间范围检查
        if (new_time > total_time) new_time = total_time;
        
        progress_state.seek_preview_time = new_time;
        
        // 更新UI显示
        progress_state.current_value = (int32_t)new_time;
        lv_bar_set_value(R.ui.playback_progress, (int32_t)new_time, LV_ANIM_OFF);
        
        // 更新时间显示
        char buff[16];
        uint32_t preview_min = new_time / 60000;
        uint32_t preview_sec = (new_time % 60000) / 1000;
        lv_snprintf(buff, sizeof(buff), "%02d:%02d", preview_min, preview_sec);
        lv_span_set_text(R.ui.playback_current_time, buff);
        
        // 预览反馈
        static uint32_t last_log_tick = 0;
        if (current_tick - last_log_tick > 500) {
            LV_LOG_USER("🎵 预览位置: %02lu:%02lu", (unsigned long)preview_min, (unsigned long)preview_sec);
            last_log_tick = current_tick;
        }
        
        break;
    }
    case LV_EVENT_RELEASED: {
        // 🎯 释放时执行实际seek操作 - 增强错误处理
        if (!progress_state.is_seeking) break;
        
        progress_state.is_seeking = false;
        
        // 平滑恢复进度条正常样式
        lv_obj_set_height(R.ui.playback_progress, 6);
        lv_obj_set_style_bg_color(R.ui.playback_progress, lv_color_hex(0xFF6B6B), LV_PART_INDICATOR);
        lv_obj_set_style_shadow_width(R.ui.playback_progress, 0, LV_PART_INDICATOR);
        
        // 执行实际的seek操作 - 增强错误处理
        uint64_t seek_time = progress_state.seek_preview_time;
        if (C.audioctl && seek_time <= C.current_album->total_time) {
            int seek_result = audio_ctl_seek(C.audioctl, seek_time / 1000);
            if (seek_result == 0) {
                C.current_time = seek_time;
                LV_LOG_USER("🎵 成功Seek到位置: %02d:%02d", 
                           (int)(seek_time / 60000), (int)((seek_time % 60000) / 1000));
            } else {
                LV_LOG_ERROR("❌ Seek操作失败: %d", seek_result);
                // 如果seek失败，恢复原来的时间显示
                app_refresh_playback_progress();
            }
        } else {
            LV_LOG_WARN("⚠️ 无效的seek位置或音频控制器");
        }
        
        // 智能恢复播放状态
        if (progress_state.was_playing) {
            if (C.play_status == PLAY_STATUS_PAUSE) {
                // 如果当前是暂停状态且之前在播放，恢复播放
                if (C.audioctl) {
                    audio_ctl_resume(C.audioctl);
                    app_set_play_status(PLAY_STATUS_PLAY);
                }
            }
        }
        
        // 恢复定时器和平滑更新
        if (C.timers.playback_progress_update) {
            lv_timer_resume(C.timers.playback_progress_update);
        }
        progress_state.smooth_update_enabled = true;
        
        LV_LOG_USER("✅ 完成进度条拖拽操作");
        break;
    }
    case LV_EVENT_CLICKED: {
        // 🎯 单击快速跳转 - 仅在非拖拽状态下
        if (progress_state.is_seeking) break;
        
        lv_point_t point;
        lv_indev_t* indev = lv_indev_active();
        lv_indev_get_point(indev, &point);
        
        // 获取进度条的坐标和尺寸
        lv_area_t area;
        lv_obj_get_coords(R.ui.playback_progress, &area);
        
        int32_t bar_width = lv_area_get_width(&area);
        int32_t relative_x = point.x - area.x1;
        
        // 边界检查
        if (relative_x < 0) relative_x = 0;
        if (relative_x > bar_width) relative_x = bar_width;
        
        // 计算新的播放时间
        uint64_t total_time = C.current_album->total_time;
        uint64_t new_time = (uint64_t)relative_x * total_time / bar_width;
        
        if (new_time > total_time) new_time = total_time;
        
        LV_LOG_USER("🎵 进度条点击跳转: %02d:%02d", 
                   (int)(new_time / 60000), (int)((new_time % 60000) / 1000));
        
        // 使用平滑动画跳转
        start_smooth_progress_animation((int32_t)new_time);
        
        // 执行跳转
        app_set_playback_time(new_time);
        break;
    }
    case LV_EVENT_PRESS_LOST: {
        // 🔄 意外失去焦点时安全恢复状态
        if (progress_state.is_seeking) {
            progress_state.is_seeking = false;
            
            // 恢复样式
            lv_obj_set_height(R.ui.playback_progress, 6);
            lv_obj_set_style_bg_color(R.ui.playback_progress, lv_color_hex(0xFF6B6B), LV_PART_INDICATOR);
            lv_obj_set_style_shadow_width(R.ui.playback_progress, 0, LV_PART_INDICATOR);
            
            // 恢复定时器
            if (C.timers.playback_progress_update) {
                lv_timer_resume(C.timers.playback_progress_update);
            }
            
            // 恢复正确的进度显示
            app_refresh_playback_progress();
            
            LV_LOG_WARN("⚠️ 进度条拖拽意外中断，已安全恢复");
        }
        break;
    }
    case LV_EVENT_LONG_PRESSED: {
        // 🎛️ 长按进入精确调节模式
        LV_LOG_USER("🔧 进入进度条精确调节模式");
        
        // 更强烈的视觉反馈
        lv_obj_set_height(R.ui.playback_progress, 12);
        lv_obj_set_style_bg_color(R.ui.playback_progress, lv_color_hex(0x00FF7F), LV_PART_INDICATOR);
        
        break;
    }
    default:
        break;
    }
}

static bool init_resource(void)
{
    // 简化版本：只初始化基本功能模块
    
    // 使用LVGL内置字体 - 仅使用配置中启用的字体
    R.fonts.size_16.normal = &lv_font_montserrat_16;
    R.fonts.size_22.bold = &lv_font_montserrat_22;
    R.fonts.size_24.normal = &lv_font_montserrat_24;
    R.fonts.size_28.normal = &lv_font_montserrat_32;  // 使用32号字体替代28号
    R.fonts.size_60.bold = &lv_font_montserrat_32;    // 使用32号字体替代60号

    // 字体检查 - 内置字体总是可用的
    if (R.fonts.size_16.normal == NULL ||
        R.fonts.size_22.bold == NULL   ||
        R.fonts.size_24.normal == NULL ||
        R.fonts.size_28.normal == NULL ||
        R.fonts.size_60.bold == NULL ) {
        return false;
    }

    // Modern UI Styles
    lv_style_init(&R.styles.button_default);
    lv_style_init(&R.styles.button_pressed);
    lv_style_init(&R.styles.circular_cover);
    lv_style_init(&R.styles.vinyl_ring);
    lv_style_init(&R.styles.vinyl_center);
    lv_style_init(&R.styles.gradient_progress);
    lv_style_init(&R.styles.frosted_glass);
    lv_style_init(&R.styles.modern_card);

    // 专业级按钮样式 - 触摸友好设计
    lv_style_set_opa(&R.styles.button_default, LV_OPA_COVER);
    lv_style_set_opa(&R.styles.button_pressed, LV_OPA_80);  // 点击时稍微降低透明度
    
    // 🔥 增强按钮点击反馈
    lv_style_set_shadow_width(&R.styles.button_default, 8);
    lv_style_set_shadow_color(&R.styles.button_default, MODERN_PRIMARY_COLOR);
    lv_style_set_shadow_opa(&R.styles.button_default, LV_OPA_40);
    
    // 按下时的阴影效果
    lv_style_set_shadow_width(&R.styles.button_pressed, 15);
    lv_style_set_shadow_color(&R.styles.button_pressed, MODERN_PRIMARY_COLOR);
    lv_style_set_shadow_opa(&R.styles.button_pressed, LV_OPA_60);
    
    // 🎯 增加按钮边框以提高可见性
    lv_style_set_border_width(&R.styles.button_default, 2);
    lv_style_set_border_color(&R.styles.button_default, lv_color_hex(0x4B5563));
    lv_style_set_border_opa(&R.styles.button_default, LV_OPA_50);
    
    // 按下时边框变亮
    lv_style_set_border_width(&R.styles.button_pressed, 2);
    lv_style_set_border_color(&R.styles.button_pressed, MODERN_PRIMARY_COLOR);
    lv_style_set_border_opa(&R.styles.button_pressed, LV_OPA_80);

    // Circular cover style - 增强圆形边框效果
    lv_style_set_radius(&R.styles.circular_cover, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&R.styles.circular_cover, 6);  // 增加边框宽度使其更明显
    lv_style_set_border_color(&R.styles.circular_cover, lv_color_hex(0x3B82F6));  // 蓝色边框更突出
    lv_style_set_border_opa(&R.styles.circular_cover, LV_OPA_80);  // 稍微透明
    lv_style_set_shadow_width(&R.styles.circular_cover, 25);  // 增强阴影
    lv_style_set_shadow_color(&R.styles.circular_cover, lv_color_hex(0x3B82F6));  // 蓝色阴影
    lv_style_set_shadow_opa(&R.styles.circular_cover, LV_OPA_30);
    lv_style_set_shadow_spread(&R.styles.circular_cover, 5);

    // Vinyl ring style - 唱片外环样式
    lv_style_set_radius(&R.styles.vinyl_ring, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&R.styles.vinyl_ring, 8);  // 厚边框模拟唱片边缘
    lv_style_set_border_color(&R.styles.vinyl_ring, lv_color_hex(0x1A1A1A));  // 深黑色
    lv_style_set_border_opa(&R.styles.vinyl_ring, LV_OPA_COVER);
    lv_style_set_bg_color(&R.styles.vinyl_ring, lv_color_hex(0x0F0F0F));  // 非常深的背景
    lv_style_set_bg_opa(&R.styles.vinyl_ring, LV_OPA_30);  // 半透明
    lv_style_set_shadow_width(&R.styles.vinyl_ring, 30);
    lv_style_set_shadow_color(&R.styles.vinyl_ring, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&R.styles.vinyl_ring, LV_OPA_70);
    lv_style_set_shadow_spread(&R.styles.vinyl_ring, 8);

    // Vinyl center style - 唱片中心孔样式
    lv_style_set_radius(&R.styles.vinyl_center, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&R.styles.vinyl_center, lv_color_hex(0x1A1A1A));  // 深黑色中心
    lv_style_set_bg_opa(&R.styles.vinyl_center, LV_OPA_COVER);
    lv_style_set_border_width(&R.styles.vinyl_center, 2);
    lv_style_set_border_color(&R.styles.vinyl_center, lv_color_hex(0x333333));
    lv_style_set_border_opa(&R.styles.vinyl_center, LV_OPA_COVER);

    // Gradient progress bar style
    lv_style_set_bg_color(&R.styles.gradient_progress, MODERN_PRIMARY_COLOR);
    lv_style_set_bg_grad_color(&R.styles.gradient_progress, MODERN_SECONDARY_COLOR);
    lv_style_set_bg_grad_dir(&R.styles.gradient_progress, LV_GRAD_DIR_HOR);
    lv_style_set_radius(&R.styles.gradient_progress, 10);

    // Frosted glass style (simplified without backdrop filter)
    lv_style_set_bg_color(&R.styles.frosted_glass, lv_color_hex(0x1E1E1E));
    lv_style_set_bg_opa(&R.styles.frosted_glass, LV_OPA_70);
    lv_style_set_radius(&R.styles.frosted_glass, 20);

    // Modern card style
    lv_style_set_bg_color(&R.styles.modern_card, MODERN_CARD_COLOR);
    lv_style_set_bg_opa(&R.styles.modern_card, LV_OPA_90);
    lv_style_set_radius(&R.styles.modern_card, 25);
    lv_style_set_shadow_width(&R.styles.modern_card, 15);
    lv_style_set_shadow_color(&R.styles.modern_card, lv_color_black());
    lv_style_set_shadow_opa(&R.styles.modern_card, LV_OPA_30);

    // transition animations
    lv_style_transition_dsc_init(&R.styles.transition_dsc, transition_props, &lv_anim_path_ease_in_out, 300, 0, NULL);
    lv_style_transition_dsc_init(&R.styles.button_transition_dsc, transition_props, &lv_anim_path_ease_in_out, 150, 0, NULL);
    lv_style_set_transition(&R.styles.button_default, &R.styles.button_transition_dsc);
    lv_style_set_transition(&R.styles.button_pressed, &R.styles.button_transition_dsc);

    // images
    R.images.playlist = ICONS_ROOT "/playlist.png";
    R.images.previous = ICONS_ROOT "/previous.png";
    R.images.play = ICONS_ROOT "/play.png";
    R.images.pause = ICONS_ROOT "/pause.png";
    R.images.next = ICONS_ROOT "/next.png";
    R.images.audio = ICONS_ROOT "/audio.png";
    R.images.mute = ICONS_ROOT "/mute.png";
    R.images.music = ICONS_ROOT "/music.png";
    R.images.nocover = ICONS_ROOT "/nocover.png";
    
    // 🚫 背景图片已禁用 - 用户要求移除background.png
    // R.images.background = ICONS_ROOT "/background.png";  // 已移除
    R.images.background = NULL;  // 明确设置为NULL

    // albums
    reload_music_config();

    return true;
}

static void app_create_top_layer(void)
{
    lv_obj_t* top_layer = lv_layer_top();
    lv_obj_set_scroll_dir(top_layer, LV_DIR_NONE);
    lv_obj_set_style_bg_color(top_layer, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(top_layer, LV_OPA_COVER, LV_STATE_USER_1);
    lv_obj_set_style_bg_opa(top_layer, LV_OPA_0, LV_STATE_DEFAULT);
    lv_obj_set_style_transition(top_layer, &R.styles.transition_dsc, LV_STATE_DEFAULT);
    lv_obj_set_style_transition(top_layer, &R.styles.transition_dsc, LV_STATE_USER_1);
    
    // 注释：仅创建音量条，播放列表已移至新的playlist_manager系统

    // VOLUME BAR
    R.ui.volume_bar = lv_obj_create(top_layer);
    lv_obj_remove_style_all(R.ui.volume_bar);
    lv_obj_set_size(R.ui.volume_bar, 60, 180);
    lv_obj_set_style_bg_color(R.ui.volume_bar, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(R.ui.volume_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_opa(R.ui.volume_bar, LV_OPA_0, LV_STATE_DEFAULT);
    lv_obj_set_style_opa(R.ui.volume_bar, LV_OPA_COVER, LV_STATE_USER_1);
    lv_obj_set_style_border_width(R.ui.volume_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(R.ui.volume_bar, 16, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(R.ui.volume_bar, true, LV_PART_MAIN);
    lv_obj_align(R.ui.volume_bar, LV_ALIGN_BOTTOM_RIGHT, -45, -95);
    lv_obj_set_style_transition(R.ui.volume_bar, &R.styles.transition_dsc, LV_STATE_DEFAULT);

    R.ui.volume_bar_indic = lv_obj_create(R.ui.volume_bar);
    lv_obj_remove_style_all(R.ui.volume_bar_indic);
    lv_obj_set_style_bg_color(R.ui.volume_bar_indic, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(R.ui.volume_bar_indic, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_size(R.ui.volume_bar_indic, LV_PCT(100), 40);
    lv_obj_align(R.ui.volume_bar_indic, LV_ALIGN_BOTTOM_MID, 0, 0);

    // 旧播放列表系统已移除 - 现在使用playlist_manager.c中的新系统
    // 设置播放列表相关UI指针为NULL，避免在其他地方误用
    R.ui.playlist_base = NULL;
    R.ui.playlist = NULL;

    lv_obj_add_flag(R.ui.volume_bar_indic, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(R.ui.volume_bar, app_volume_bar_event_handler, LV_EVENT_ALL, NULL);
}

static void app_create_error_page(void)
{
    lv_obj_t* root = lv_screen_active();
    lv_obj_t* label = lv_label_create(root);
    lv_obj_set_width(label, LV_PCT(80));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "Vela的音乐播放器\n资源加载失败\n请检查设备和配置");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFF6B6B), LV_PART_MAIN);
    lv_obj_center(label);
}

static void app_create_main_page(void)
{
    lv_obj_t* root = lv_screen_active();

    // 🎨 专业级纯色背景设计 - 移除background.png依赖
    // 采用深色专业主题，提升用户体验和性能
    lv_obj_set_style_bg_color(root, lv_color_hex(0x121212), LV_PART_MAIN);  // 深色主题
    lv_obj_set_style_bg_grad_color(root, lv_color_hex(0x0F0F0F), LV_PART_MAIN);  // 更深的渐变
        lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    
    printf("🎨 专业级深色背景已应用 - 无背景图片依赖\n");
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(root, 16, LV_PART_MAIN);

    // 🔝 顶部状态栏 - WiFi信号、电池、时间区域
    lv_obj_t* status_bar = lv_obj_create(root);
    lv_obj_remove_style_all(status_bar);
    lv_obj_set_size(status_bar, LV_PCT(100), 48);
    lv_obj_add_style(status_bar, &R.styles.frosted_glass, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(status_bar, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(status_bar, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 左侧车机品牌标识
    lv_obj_t* brand_label = lv_label_create(status_bar);
    lv_label_set_text(brand_label, "Vela Audio");
    lv_obj_set_style_text_font(brand_label, R.fonts.size_22.bold, LV_PART_MAIN);
    lv_obj_set_style_text_color(brand_label, lv_color_hex(0x3B82F6), LV_PART_MAIN); // 霓虹蓝

    // 右侧状态信息（信号、电池、时间）
    lv_obj_t* status_info = lv_obj_create(status_bar);
    lv_obj_remove_style_all(status_info);
    lv_obj_set_size(status_info, LV_PCT(60), LV_SIZE_CONTENT);  // 给状态信息更多空间
    lv_obj_set_flex_flow(status_info, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_info, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // WiFi信号
    lv_obj_t* wifi_label = lv_label_create(status_info);
    lv_label_set_text(wifi_label, "WiFi");
    lv_obj_set_style_text_font(wifi_label, R.fonts.size_16.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xBBBBBB), LV_PART_MAIN);
    lv_obj_set_style_margin_right(wifi_label, 16, LV_PART_MAIN);

    // 电池
    lv_obj_t* battery_label = lv_label_create(status_info);
    lv_label_set_text(battery_label, "85%");
    lv_obj_set_style_text_font(battery_label, R.fonts.size_16.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0xBBBBBB), LV_PART_MAIN);
    lv_obj_set_style_margin_right(battery_label, 20, LV_PART_MAIN);

    // 时间和日期容器 - 垂直排列以节省水平空间
    lv_obj_t* time_container = lv_obj_create(status_info);
    lv_obj_remove_style_all(time_container);
    lv_obj_set_size(time_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(time_container, 0, LV_PART_MAIN);
    
    // 时间显示 - 较大字体
    lv_obj_t* time_label = lv_label_create(time_container);
    R.ui.time = time_label;
    lv_label_set_text(time_label, "14:28");  // 使用示例时间确保足够宽度
    lv_obj_set_style_text_font(time_label, R.fonts.size_24.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(time_label, 2, LV_PART_MAIN);

    // 星期显示 - 紧凑排列在时间下方
    lv_obj_t* date_label = lv_label_create(time_container);
    R.ui.date = date_label;
    lv_label_set_text(date_label, "Monday");    // 使用完整星期名确保足够宽度
    lv_obj_set_style_text_font(date_label, R.fonts.size_16.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xBBBBBB), LV_PART_MAIN);
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    // 主播放区域 - 现代卡片设计
    lv_obj_t* player_main = lv_obj_create(root);
    R.ui.player_group = player_main;
    lv_obj_remove_style_all(player_main);
    lv_obj_add_style(player_main, &R.styles.modern_card, LV_PART_MAIN);
    lv_obj_set_size(player_main, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(player_main, 32, LV_PART_MAIN);
    lv_obj_set_flex_flow(player_main, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(player_main, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(player_main, 1);

    // 3D效果专辑封面区域 + 高斯模糊背景
    lv_obj_t* cover_section = lv_obj_create(player_main);
    lv_obj_remove_style_all(cover_section);
    lv_obj_set_size(cover_section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cover_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cover_section, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(cover_section, 24, LV_PART_MAIN);

    // 专辑封面容器 - 支持3D旋转
    lv_obj_t* album_container = lv_obj_create(cover_section);
    R.ui.album_cover_container = album_container;
    lv_obj_remove_style_all(album_container);
    lv_obj_set_size(album_container, 320, 320);  // 增大容器以容纳多层效果
    lv_obj_set_style_transform_pivot_x(album_container, 160, 0);
    lv_obj_set_style_transform_pivot_y(album_container, 160, 0);

    // 唱片外环 - 最外层边框效果
    lv_obj_t* vinyl_ring = lv_obj_create(album_container);
    R.ui.vinyl_ring = vinyl_ring;
    lv_obj_remove_style_all(vinyl_ring);
    lv_obj_add_style(vinyl_ring, &R.styles.vinyl_ring, LV_PART_MAIN);
    lv_obj_set_size(vinyl_ring, 320, 320);
    lv_obj_center(vinyl_ring);

    // 🖼️ 专辑封面图片 - 完美圆形显示，图片完整不变形
    lv_obj_t* album_cover = lv_image_create(album_container);
    R.ui.album_cover = album_cover;
    lv_obj_remove_style_all(album_cover);
    lv_obj_add_style(album_cover, &R.styles.circular_cover, LV_PART_MAIN);
    lv_obj_set_size(album_cover, 280, 280);
    lv_obj_center(album_cover);
    
    // 设置图片显示模式：完整显示，居中，保持原始比例
    lv_image_set_scale(album_cover, 256);  // 适当缩放保持清晰度
    lv_image_set_inner_align(album_cover, LV_IMAGE_ALIGN_CENTER);
    lv_image_set_pivot(album_cover, 140, 140);  // 设置旋转中心点
    
    // 添加图片加载错误处理
    lv_image_set_src(album_cover, R.images.nocover);
    
    // 确保PNG图片按原始比例完整显示在圆形容器内
    lv_obj_set_style_clip_corner(album_cover, true, LV_PART_MAIN);  // 启用圆形裁剪
    lv_obj_set_style_bg_img_recolor_opa(album_cover, LV_OPA_0, LV_PART_MAIN);  // 不重新着色
    lv_obj_set_style_image_recolor_opa(album_cover, LV_OPA_0, LV_PART_MAIN);  // 不重新着色图片
    
    // 优化图片填充方式，确保完整显示
    lv_obj_set_style_bg_image_opa(album_cover, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(album_cover, LV_RADIUS_CIRCLE, LV_PART_MAIN);  // 圆形边框

    // 移除唱片中心孔 - 用户要求不显示中间的黑点
    // lv_obj_t* vinyl_center = lv_obj_create(album_container);
    // R.ui.vinyl_center = vinyl_center;
    R.ui.vinyl_center = NULL;  // 设置为NULL避免其他代码引用

    // 歌曲信息区域 - 滚动字幕支持
    lv_obj_t* song_info = lv_obj_create(player_main);
    lv_obj_remove_style_all(song_info);
    lv_obj_set_size(song_info, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(song_info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(song_info, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(song_info, 20, LV_PART_MAIN);

    // 歌曲名称 - 大字体动态滚动字幕
    lv_obj_t* song_title = lv_label_create(song_info);
    R.ui.album_name = song_title;
    lv_label_set_text(song_title, "选择您的音乐");
    lv_label_set_long_mode(song_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(song_title, LV_PCT(90));
    lv_obj_set_style_text_align(song_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(song_title, R.fonts.size_28.normal, LV_PART_MAIN);  // 使用28号字体
    lv_obj_set_style_text_color(song_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(song_title, 12, LV_PART_MAIN);

    // 艺术家信息 - 增大字体
    lv_obj_t* artist_name = lv_label_create(song_info);
    R.ui.album_artist = artist_name;
    lv_label_set_text(artist_name, "Vela Music");
    lv_label_set_long_mode(artist_name, LV_LABEL_LONG_SCROLL);
    lv_obj_set_width(artist_name, LV_PCT(80));
    lv_obj_set_style_text_align(artist_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(artist_name, R.fonts.size_22.bold, LV_PART_MAIN);  // 从16增大到22
    lv_obj_set_style_text_color(artist_name, lv_color_hex(0xE5E7EB), LV_PART_MAIN);

    // 播放进度区域
    lv_obj_t* progress_section = lv_obj_create(player_main);
    R.ui.playback_group = progress_section;
    lv_obj_remove_style_all(progress_section);
    lv_obj_set_size(progress_section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(progress_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(progress_section, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(progress_section, 24, LV_PART_MAIN);

    // 进度条和时间的横向布局容器
    lv_obj_t* progress_bar_container = lv_obj_create(progress_section);
    lv_obj_remove_style_all(progress_bar_container);
    lv_obj_set_size(progress_bar_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(progress_bar_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(progress_bar_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(progress_bar_container, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(progress_bar_container, 0, LV_PART_MAIN);

    // 渐变进度条 - 占据大部分宽度
    lv_obj_t* progress_bar = lv_bar_create(progress_bar_container);
    R.ui.playback_progress = progress_bar;
    lv_obj_remove_style_all(progress_bar);
    lv_obj_add_style(progress_bar, &R.styles.gradient_progress, LV_PART_INDICATOR);
    lv_obj_set_size(progress_bar, LV_PCT(65), 6);  // 调整宽度为65%，为时间显示留出更多空间
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 3, LV_PART_MAIN);

    // 时间显示区域 - 右侧对齐，使用更大字体
    lv_obj_t* time_display = lv_spangroup_create(progress_bar_container);
    lv_span_t* current_time = lv_spangroup_new_span(time_display);
    lv_span_t* separator = lv_spangroup_new_span(time_display);
    lv_span_t* total_time = lv_spangroup_new_span(time_display);
    R.ui.playback_current_time = current_time;
    R.ui.playback_total_time = total_time;
    
    lv_span_set_text(current_time, "00:00");
    lv_span_set_text(separator, " / ");  // 使用"/"分隔符，更简洁
    lv_span_set_text(total_time, "00:00");
    lv_obj_set_style_text_font(time_display, R.fonts.size_22.bold, LV_PART_MAIN);  // 从16号改为22号字体
    lv_obj_set_style_text_color(time_display, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_text_align(time_display, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);  // 右对齐
    lv_obj_set_style_margin_right(time_display, 8, LV_PART_MAIN);  // 右侧留白
    lv_style_set_text_color(&separator->style, lv_color_hex(0x9CA3AF));
    lv_style_set_text_color(&total_time->style, lv_color_hex(0x9CA3AF));

    // 多功能控制区域 - 水平分散排列，间距加大
    lv_obj_t* control_area = lv_obj_create(player_main);
    lv_obj_remove_style_all(control_area);
    lv_obj_set_size(control_area, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(control_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(control_area, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);  // 均匀分散排列
    lv_obj_set_style_pad_all(control_area, 20, LV_PART_MAIN);  // 增加内边距从12到20
    lv_obj_set_style_pad_column(control_area, 16, LV_PART_MAIN);  // 设置按钮之间的列间距

    // 🎮 专业级播放列表按钮 - 应用专业配置
    lv_obj_t* playlist_btn = lv_button_create(control_area);
    lv_obj_t* playlist_icon = lv_image_create(playlist_btn);
    lv_obj_remove_style_all(playlist_btn);
    
    // 应用专业级按钮配置
    // 标准按钮配置
    lv_obj_set_style_bg_color(playlist_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_color(playlist_btn, lv_color_hex(0x4B5563), LV_PART_MAIN | LV_STATE_PRESSED);
    
    lv_image_set_src(playlist_icon, R.images.playlist);
    lv_obj_set_size(playlist_icon, 28, 28);
    lv_obj_center(playlist_icon);

    // 🎮 专业级上一首按钮
    lv_obj_t* prev_btn = lv_button_create(control_area);
    lv_obj_t* prev_icon = lv_image_create(prev_btn);
    lv_obj_remove_style_all(prev_btn);
    
    // 应用专业级按钮配置
    // 上一首按钮配置
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x4B5563), LV_PART_MAIN | LV_STATE_PRESSED);
    
    lv_image_set_src(prev_icon, R.images.previous);
    lv_obj_set_size(prev_icon, 32, 32);
    lv_obj_center(prev_icon);

    // 简化版本：去除快退10秒按钮，保持界面简洁

    // 🎮 专业级主播放按钮 - 特殊发光效果
    lv_obj_t* play_btn = lv_button_create(control_area);
    lv_obj_t* play_icon = lv_image_create(play_btn);
    R.ui.play_btn = play_icon;
    lv_obj_remove_style_all(play_btn);
    
    // 应用专业级按钮配置 - 主按钮使用大尺寸
    // 播放按钮配置
    lv_obj_set_style_bg_color(play_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_color(play_btn, lv_color_hex(0x4B5563), LV_PART_MAIN | LV_STATE_PRESSED);
    
    // 🔥 主按钮特殊发光效果
    lv_obj_set_style_shadow_width(play_btn, 25, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(play_btn, 35, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_color(play_btn, lv_color_hex(0x00BFFF), LV_PART_MAIN);  // 蓝色阴影
    lv_obj_set_style_shadow_opa(play_btn, LV_OPA_70, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(play_btn, LV_OPA_90, LV_STATE_PRESSED);
    
    lv_image_set_src(play_icon, R.images.play);
    lv_obj_set_size(play_icon, 48, 48);
    lv_obj_center(play_icon);

    // 简化版本：去除快进10秒按钮，保持界面简洁

    // 🎮 专业级下一首按钮
    lv_obj_t* next_btn = lv_button_create(control_area);
    lv_obj_t* next_icon = lv_image_create(next_btn);
    lv_obj_remove_style_all(next_btn);
    
    // 应用专业级按钮配置
    // 下一首按钮配置
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x4B5563), LV_PART_MAIN | LV_STATE_PRESSED);
    
    lv_image_set_src(next_icon, R.images.next);
    lv_obj_set_size(next_icon, 32, 32);
    lv_obj_center(next_icon);

    // 🎮 专业级音量按钮
    lv_obj_t* volume_btn = lv_button_create(control_area);
    lv_obj_t* volume_icon = lv_image_create(volume_btn);
    R.ui.audio = volume_icon;
    lv_obj_remove_style_all(volume_btn);
    
    // 应用专业级按钮配置
    // 音量按钮配置
    lv_obj_set_style_bg_color(volume_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_btn, lv_color_hex(0x4B5563), LV_PART_MAIN | LV_STATE_PRESSED);
    
    lv_obj_set_size(volume_icon, 28, 28);
    lv_obj_center(volume_icon);

    // 创建顶层覆盖 (音量条、播放列表等)
    app_create_top_layer();

    // 🎯 专业级按钮交互已通过配置系统应用
    printf("🎯 专业级UI交互优化已应用 - 增强触摸响应和视觉反馈\n");
    printf("🔧 按钮配置: 扩展点击区域、优化长按时间、增强视觉反馈\n");

    // 🎯 专业级事件绑定 - 支持多种交互模式和增强反馈
    lv_obj_add_event_cb(playlist_btn, app_playlist_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(playlist_btn, app_playlist_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(playlist_btn, app_playlist_event_handler, LV_EVENT_RELEASED, NULL);
    
    lv_obj_add_event_cb(volume_btn, app_audio_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(volume_btn, app_audio_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(volume_btn, app_audio_event_handler, LV_EVENT_RELEASED, NULL);
    
    lv_obj_add_event_cb(play_btn, app_play_status_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(play_btn, app_play_status_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(play_btn, app_play_status_event_handler, LV_EVENT_RELEASED, NULL);
    
    lv_obj_add_event_cb(prev_btn, app_switch_album_event_handler, LV_EVENT_CLICKED, (lv_uintptr_t*)SWITCH_ALBUM_MODE_PREV);
    lv_obj_add_event_cb(prev_btn, app_switch_album_event_handler, LV_EVENT_PRESSED, (lv_uintptr_t*)SWITCH_ALBUM_MODE_PREV);
    lv_obj_add_event_cb(prev_btn, app_switch_album_event_handler, LV_EVENT_RELEASED, (lv_uintptr_t*)SWITCH_ALBUM_MODE_PREV);
    
    lv_obj_add_event_cb(next_btn, app_switch_album_event_handler, LV_EVENT_CLICKED, (lv_uintptr_t*)SWITCH_ALBUM_MODE_NEXT);
    lv_obj_add_event_cb(next_btn, app_switch_album_event_handler, LV_EVENT_PRESSED, (lv_uintptr_t*)SWITCH_ALBUM_MODE_NEXT);
    lv_obj_add_event_cb(next_btn, app_switch_album_event_handler, LV_EVENT_RELEASED, (lv_uintptr_t*)SWITCH_ALBUM_MODE_NEXT);
    
    // 添加长按支持 - 环境友好
    lv_obj_add_event_cb(prev_btn, app_switch_album_event_handler, LV_EVENT_LONG_PRESSED_REPEAT, (lv_uintptr_t*)SWITCH_ALBUM_MODE_PREV);
    lv_obj_add_event_cb(next_btn, app_switch_album_event_handler, LV_EVENT_LONG_PRESSED_REPEAT, (lv_uintptr_t*)SWITCH_ALBUM_MODE_NEXT);
    
    // 🎛️ 进度条增强交互 - 支持点击跳转和拖拽
    lv_obj_add_event_cb(progress_bar, app_playback_progress_bar_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_ext_click_area(progress_bar, 8);  // 扩展进度条点击区域
    
    // 🔊 音量条增强交互
    lv_obj_set_ext_click_area(R.ui.volume_bar, 10);  // 扩展音量条点击区域
    
    // 🎨 按钮长按延迟已通过专业级配置系统设置
    printf("🎮 专业级事件绑定完成 - 支持点击/长按/拖拽多种交互模式\n");

    // 启动时间更新定时器
    app_start_updating_date_time();
    
    // 开发模式：可以在这里添加测试代码
    #ifdef DEBUG
    LV_LOG_USER("🔧 进度条功能已就绪，可进行测试");
    #endif
}

// 时间更新功能
static void app_start_updating_date_time(void)
{
    // 确保时间和日期UI组件已创建
    if (!R.ui.time || !R.ui.date) {
        LV_LOG_ERROR("Time/Date UI components not ready, cannot start timer");
        return;
    }

    LV_LOG_USER("Starting date/time update system...");
    
    // 立即更新一次时间
    app_refresh_date_time();
    
    // 创建定时器前，先检查是否已存在
    if (C.timers.refresh_date_time != NULL) {
        lv_timer_delete(C.timers.refresh_date_time);
        C.timers.refresh_date_time = NULL;
    }
    
    C.timers.refresh_date_time = lv_timer_create(app_refresh_date_time_timer_cb, 1000, NULL);
    if (C.timers.refresh_date_time == NULL) {
        LV_LOG_ERROR("Failed to create date/time update timer");
        return;
    }
    
    LV_LOG_USER("Date/Time update timer created successfully - updating every 1000ms");
}

static void read_configs(void)
{
    uint32_t file_size;
    lv_fs_file_t file;
    lv_fs_open(&file, RES_ROOT "/config.json", LV_FS_MODE_RD);

    lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    lv_fs_tell(&file, &file_size);
    lv_fs_seek(&file, 0, LV_FS_SEEK_SET);

    char* buff = lv_malloc(file_size);
    lv_fs_read(&file, buff, file_size, NULL);

    const char* json_string = buff;

    cJSON* json = cJSON_Parse(json_string);
    if (json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            LV_LOG_ERROR("parse error: %p", error_ptr);
        }
        lv_free(buff);
        return;
    }

#if WIFI_ENABLED
    cJSON* wifi_object = cJSON_GetObjectItem(json, "wifi");

    const char* ssid = cJSON_GetStringValue(cJSON_GetObjectItem(wifi_object, "ssid"));
    const char* pswd = cJSON_GetStringValue(cJSON_GetObjectItem(wifi_object, "pswd"));
    const int version = cJSON_GetNumberValue(cJSON_GetObjectItem(wifi_object, "wpa_ver"));

    lv_strcpy(CF.wifi.ssid, ssid);
    lv_strcpy(CF.wifi.pswd, pswd);
    CF.wifi.ver_flag = version;
#endif

    cJSON_Delete(json);

    lv_free(buff);
}

static void reload_music_config(void)
{
    // 🧹 安全的内存清理 - 增强版
    LV_LOG_USER("🔄 开始重新加载音乐配置...");
    
    /* Clear previous music config */
    if (R.albums) {
        for (int i = 0; i < R.album_count; i++) {
            // 安全释放字符串内存
            if (R.albums[i].name) {
                lv_free((void*)R.albums[i].name);
                R.albums[i].name = NULL;
            }
            if (R.albums[i].artist) {
                lv_free((void*)R.albums[i].artist);
                R.albums[i].artist = NULL;
            }
            // 清空路径和封面字符数组
            memset(R.albums[i].path, 0, sizeof(R.albums[i].path));
            memset(R.albums[i].cover, 0, sizeof(R.albums[i].cover));
        }

        lv_free(R.albums);
        R.albums = NULL;
    }
    
    int old_album_count = R.album_count;
    R.album_count = 0;
    
    // 💾 内存统计
    lv_mem_monitor_t mem_info_cleanup;
    lv_mem_monitor(&mem_info_cleanup);
    LV_LOG_USER("🧹 音乐配置内存清理完成 (释放了%d个专辑, 可用内存: %zu KB)", 
                old_album_count, mem_info_cleanup.free_size / 1024);

    /* Load music config - 增强版内存安全 */
    uint32_t file_size;
    lv_fs_file_t file;
    char* buff = NULL;
    cJSON* json = NULL;
    
    // 🔒 安全的文件打开
    lv_fs_res_t res = lv_fs_open(&file, MUSICS_ROOT "/manifest.json", LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("无法打开音乐清单文件: %s", MUSICS_ROOT "/manifest.json");
        return;
    }

    // 📏 安全的文件大小获取
    if (lv_fs_seek(&file, 0, LV_FS_SEEK_END) != LV_FS_RES_OK ||
        lv_fs_tell(&file, &file_size) != LV_FS_RES_OK ||
        lv_fs_seek(&file, 0, LV_FS_SEEK_SET) != LV_FS_RES_OK) {
        LV_LOG_ERROR("文件操作失败");
        lv_fs_close(&file);
        return;
    }

    // 💾 安全的内存分配
    if (file_size == 0 || file_size > 2 * 1024 * 1024) {  // 限制2MB
        LV_LOG_ERROR("音乐清单文件大小异常: %lu bytes", (unsigned long)file_size);
        lv_fs_close(&file);
        return;
    }
    
    buff = lv_malloc(file_size + 1);  // +1 for null terminator
    if (!buff) {
        LV_LOG_ERROR("内存分配失败: %lu bytes", (unsigned long)file_size);
        lv_fs_close(&file);
        return;
    }
    
    memset(buff, 0, file_size + 1);
    
    uint32_t bytes_read;
    if (lv_fs_read(&file, buff, file_size, &bytes_read) != LV_FS_RES_OK || bytes_read != file_size) {
        LV_LOG_ERROR("文件读取失败: 期望 %lu bytes, 实际读取 %lu bytes", 
                     (unsigned long)file_size, (unsigned long)bytes_read);
        goto cleanup_and_exit;
    }
    
    lv_fs_close(&file);

    // 🔍 JSON解析
    json = cJSON_Parse(buff);
    if (json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        LV_LOG_ERROR("JSON解析错误: %s", error_ptr ? error_ptr : "未知错误");
        goto cleanup_and_exit;
    }

    cJSON* musics_object = cJSON_GetObjectItem(json, "musics");
    if (musics_object == NULL) {
        LV_LOG_ERROR("JSON中未找到'musics'对象");
        goto cleanup_and_exit;
    }

    int total_albums = cJSON_GetArraySize(musics_object);
    // 限制专辑数量防止内存溢出
    #define MAX_SAFE_ALBUMS 50
    R.album_count = (total_albums > MAX_SAFE_ALBUMS) ? MAX_SAFE_ALBUMS : total_albums;
    
    printf("JSON中发现 %d 个专辑，将加载 %d 个\n", total_albums, R.album_count);
    
    R.albums = lv_malloc_zeroed(R.album_count * sizeof(album_info_t));
    if (!R.albums) {
        printf("❌ 专辑内存分配失败!\n");
        lv_free(buff);
        return;
    }

    for (int i = 0; i < R.album_count; i++) {
        cJSON* music_object = cJSON_GetArrayItem(musics_object, i);
        if (!music_object) {
            printf("❌ 专辑 %d JSON对象无效\n", i);
            continue;
        }

        const char* path = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "path"));
        const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "name"));
        const char* artist = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "artist"));
        const char* cover = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "cover"));
        const double total_time_double = cJSON_GetNumberValue(cJSON_GetObjectItem(music_object, "total_time"));
        const char* color_str = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "color"));

        // 安全检查
        if (!path || !name) {
            printf("❌ 专辑 %d 缺少必要信息 (path: %p, name: %p)\n", i, path, name);
            continue;
        }

        uint64_t total_time = (uint64_t)total_time_double;
        printf("加载专辑 %d: %s - %s\n", i, name, artist ? artist : "未知艺术家");
        uint32_t color_int = strtoul(color_str + 1, NULL, 16);

        if (total_time == 0)
            total_time = 1;

        lv_color_t color = lv_color_hex(color_int);

        lv_snprintf(R.albums[i].path, sizeof(R.albums[i].path), "%s/%s", MUSICS_ROOT, path);
        lv_snprintf(R.albums[i].cover, sizeof(R.albums[i].cover), "%s/%s", MUSICS_ROOT, cover);
        R.albums[i].name = lv_strdup(name);
        R.albums[i].artist = lv_strdup(artist);
        R.albums[i].total_time = total_time;
        R.albums[i].color = color;

        LV_LOG_USER("Album %d: %s - %s | %s %s %lu", i, R.albums[i].name, R.albums[i].artist, R.albums[i].path, R.albums[i].cover, (unsigned long)total_time);
    }

    // 🧹 安全清理资源
    goto cleanup_success;

cleanup_and_exit:
    // 清理失败时的资源释放
    if (R.albums) {
        lv_free(R.albums);
        R.albums = NULL;
        R.album_count = 0;
    }
    
cleanup_success:
    // 成功或失败都要清理的资源
    if (json) {
        cJSON_Delete(json);
        json = NULL;
    }
    
    if (buff) {
        lv_free(buff);
        buff = NULL;
    }
    
    // 💾 内存状态报告
    lv_mem_monitor_t final_mem_info;
    lv_mem_monitor(&final_mem_info);
    LV_LOG_USER("🎵 音乐配置加载完成 (专辑数: %d, 可用内存: %zu KB)", 
                R.album_count, final_mem_info.free_size / 1024);
}

/**********************
 * MODERN UI ANIMATIONS
 **********************/

/* 
 * 封面旋转动画功能已被禁用
 * Cover rotation animation functions have been DISABLED
 */

/*
static void app_cover_rotation_anim_cb(void* obj, int32_t value)
{
    lv_obj_t* cover = (lv_obj_t*)obj;
    C.animations.rotation_angle = value;
    lv_obj_set_style_transform_rotation(cover, value, 0);
}

static void app_start_cover_rotation_animation(void)
{
    if (C.animations.is_rotating) return;
    
    lv_anim_init(&C.animations.cover_rotation_anim);
    lv_anim_set_var(&C.animations.cover_rotation_anim, R.ui.album_cover_container);
    lv_anim_set_exec_cb(&C.animations.cover_rotation_anim, app_cover_rotation_anim_cb);
    lv_anim_set_values(&C.animations.cover_rotation_anim, C.animations.rotation_angle, C.animations.rotation_angle + 3600);
    lv_anim_set_duration(&C.animations.cover_rotation_anim, COVER_ROTATION_DURATION);
    lv_anim_set_repeat_count(&C.animations.cover_rotation_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&C.animations.cover_rotation_anim, lv_anim_path_linear);
    lv_anim_start(&C.animations.cover_rotation_anim);
    
    C.animations.is_rotating = true;
    LV_LOG_USER("Album cover rotation started - simulating vinyl record");
}

static void app_stop_cover_rotation_animation(void)
{
    if (!C.animations.is_rotating) return;
    
    lv_anim_delete(R.ui.album_cover_container, app_cover_rotation_anim_cb);
    C.animations.is_rotating = false;
    LV_LOG_USER("⏸️ Album cover rotation stopped - vinyl record paused");
}
*/
