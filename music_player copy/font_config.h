//
// Vela 音乐播放器 - 字体配置文件
// Created by Vela on 2025/7/27
// 支持UTF-8编码和中文显示的字体配置
//

#ifndef FONT_CONFIG_H
#define FONT_CONFIG_H

#include "lvgl.h"

/*********************
 *      字体配置
 *********************/

// 字体编码支持
#define FONT_UTF8_SUPPORT 1
#define FONT_CHINESE_SUPPORT 1

// 播放列表专用字体大小定义
#define PLAYLIST_TITLE_FONT_SIZE 32      // 播放列表标题字体
#define PLAYLIST_SONG_NAME_FONT_SIZE 24  // 歌曲名称字体
#define PLAYLIST_ARTIST_FONT_SIZE 20     // 艺术家字体
#define PLAYLIST_INFO_FONT_SIZE 16       // 信息字体
#define PLAYLIST_BUTTON_FONT_SIZE 18     // 按钮字体

// MiSans字体路径
#define MISANS_NORMAL_PATH "/data/res/fonts/MiSans-Normal.ttf"
#define MISANS_SEMIBOLD_PATH "/data/res/fonts/MiSans-Semibold.ttf"

// 使用系统默认字体（支持基本拉丁字符）+ 运行时字体渲染
#if LV_FONT_MONTSERRAT_16
    #define FONT_DEFAULT_16 &lv_font_montserrat_16
#else
    #define FONT_DEFAULT_16 &lv_font_default
#endif

#if LV_FONT_MONTSERRAT_20
    #define FONT_DEFAULT_20 &lv_font_montserrat_20
#else
    #define FONT_DEFAULT_20 FONT_DEFAULT_16
#endif

#if LV_FONT_MONTSERRAT_24
    #define FONT_DEFAULT_24 &lv_font_montserrat_24
#else
    #define FONT_DEFAULT_24 FONT_DEFAULT_20
#endif

#if LV_FONT_MONTSERRAT_28
    #define FONT_DEFAULT_28 &lv_font_montserrat_28
#else
    #define FONT_DEFAULT_28 FONT_DEFAULT_24
#endif

#if LV_FONT_MONTSERRAT_32
    #define FONT_DEFAULT_32 &lv_font_montserrat_32
#else
    #define FONT_DEFAULT_32 FONT_DEFAULT_28
#endif

// 中文字体支持 - 如果支持FreeType
#if LV_USE_FREETYPE
    extern lv_font_t* misans_font_16;
    extern lv_font_t* misans_font_20;
    extern lv_font_t* misans_font_24;
    extern lv_font_t* misans_font_28;
    extern lv_font_t* misans_font_32;
    
    #define CHINESE_FONT_16 misans_font_16
    #define CHINESE_FONT_20 misans_font_20
    #define CHINESE_FONT_24 misans_font_24
    #define CHINESE_FONT_28 misans_font_28
    #define CHINESE_FONT_32 misans_font_32
#else
    #define CHINESE_FONT_16 FONT_DEFAULT_16
    #define CHINESE_FONT_20 FONT_DEFAULT_20
    #define CHINESE_FONT_24 FONT_DEFAULT_24
    #define CHINESE_FONT_28 FONT_DEFAULT_28
    #define CHINESE_FONT_32 FONT_DEFAULT_32
#endif

/*********************
 *      字体函数
 *********************/

/**
 * @brief 获取适合的字体（基于大小）
 */
const lv_font_t* get_font_by_size(int size);

/**
 * @brief 检查文本是否包含中文字符
 */
bool text_contains_chinese(const char* text);

/**
 * @brief 设置标签的UTF-8文本
 */
void set_label_utf8_text(lv_obj_t* label, const char* text, const lv_font_t* font);

/**
 * @brief 初始化字体系统
 */
int font_system_init(void);

/**
 * @brief 获取播放列表专用字体
 * @param font_type 字体类型：title, song, artist, info, button
 * @return 对应的字体指针
 */
const lv_font_t* get_playlist_font(const char* font_type);

/**
 * @brief 优化文本显示效果（抗锯齿、字间距等）
 * @param label 标签对象
 * @param text 要显示的文本
 * @param font_type 字体类型
 */
void optimize_text_display(lv_obj_t* label, const char* text, const char* font_type);

#endif // FONT_CONFIG_H
