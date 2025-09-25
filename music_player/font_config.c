//
// Vela 音乐播放器 - 字体配置实现
// Created by Vela on 2025/8/05
// 支持UTF-8编码和中文显示的字体配置实现
//

#include "font_config.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// FreeType字体实例
#if LV_USE_FREETYPE
lv_font_t* misans_font_16 = NULL;
lv_font_t* misans_font_20 = NULL;
lv_font_t* misans_font_24 = NULL;
lv_font_t* misans_font_28 = NULL;
lv_font_t* misans_font_32 = NULL;

// FreeType字体支持暂时禁用，使用LVGL内置字体
// static lv_ft_info_t misans_info_16;
// static lv_ft_info_t misans_info_20;
// static lv_ft_info_t misans_info_24;
// static lv_ft_info_t misans_info_28;
// static lv_ft_info_t misans_info_32;
#endif

// 简化版字体支持标志
static bool font_system_initialized = false;

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 获取适合的字体（基于大小）- 优先使用中文字体
 */
const lv_font_t* get_font_by_size(int size)
{
#if LV_USE_FREETYPE
    // 如果支持FreeType，优先使用MiSans中文字体
    if (size >= 32 && CHINESE_FONT_32) {
        return CHINESE_FONT_32;
    } else if (size >= 28 && CHINESE_FONT_28) {
        return CHINESE_FONT_28;
    } else if (size >= 24 && CHINESE_FONT_24) {
        return CHINESE_FONT_24;
    } else if (size >= 20 && CHINESE_FONT_20) {
        return CHINESE_FONT_20;
    } else if (CHINESE_FONT_16) {
        return CHINESE_FONT_16;
    }
#endif
    
    // 回退到系统默认字体
    if (size >= 32) {
        return FONT_DEFAULT_32;
    } else if (size >= 28) {
        return FONT_DEFAULT_28;
    } else if (size >= 24) {
        return FONT_DEFAULT_24;
    } else if (size >= 20) {
        return FONT_DEFAULT_20;
    } else {
        return FONT_DEFAULT_16;
    }
}

/**
 * @brief 检查文本是否包含中文字符
 */
bool text_contains_chinese(const char* text)
{
    if (!text) return false;
    
    const unsigned char* p = (const unsigned char*)text;
    while (*p) {
        // 检查UTF-8编码的中文字符范围
        if (*p >= 0xE4 && *p <= 0xE9) {
            // 可能是中文字符
            if (*(p+1) >= 0x80 && *(p+1) <= 0xBF && 
                *(p+2) >= 0x80 && *(p+2) <= 0xBF) {
                return true;
            }
        }
        p++;
    }
    return false;
}

/**
 * @brief 设置标签的UTF-8文本
 */
void set_label_utf8_text(lv_obj_t* label, const char* text, const lv_font_t* font)
{
    if (!label || !text) return;
    
    // 设置字体
    if (font) {
        lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    }
    
    // 设置文本 - LVGL会自动处理UTF-8编码
    lv_label_set_text(label, text);
    
    // 如果包含中文，可能需要特殊处理
    if (text_contains_chinese(text)) {
        // 对于中文文本，可能需要调整一些显示属性
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    }
}

/**
 * @brief 获取播放列表专用字体
 */
const lv_font_t* get_playlist_font(const char* font_type)
{
    if (!font_type) return get_font_by_size(16);
    
    if (strcmp(font_type, "title") == 0) {
        return get_font_by_size(PLAYLIST_TITLE_FONT_SIZE);
    } else if (strcmp(font_type, "song") == 0) {
        return get_font_by_size(PLAYLIST_SONG_NAME_FONT_SIZE);
    } else if (strcmp(font_type, "artist") == 0) {
        return get_font_by_size(PLAYLIST_ARTIST_FONT_SIZE);
    } else if (strcmp(font_type, "info") == 0) {
        return get_font_by_size(PLAYLIST_INFO_FONT_SIZE);
    } else if (strcmp(font_type, "button") == 0) {
        return get_font_by_size(PLAYLIST_BUTTON_FONT_SIZE);
    }
    
    return get_font_by_size(16); // 默认字体
}

/**
 * @brief 优化文本显示效果
 */
void optimize_text_display(lv_obj_t* label, const char* text, const char* font_type)
{
    if (!label || !text) return;
    
    // 获取合适的字体
    const lv_font_t* font = get_playlist_font(font_type);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    
    // 设置文本内容
    lv_label_set_text(label, text);
    
    // 优化显示效果
    if (text_contains_chinese(text)) {
        // 中文文本优化
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_letter_space(label, 1, LV_PART_MAIN); // 增加字间距
    } else {
        // 英文文本优化
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_letter_space(label, 0, LV_PART_MAIN);
    }
    
    // 根据字体类型设置特殊样式
    if (strcmp(font_type, "title") == 0) {
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_opa(label, LV_OPA_100, LV_PART_MAIN);
    } else if (strcmp(font_type, "song") == 0) {
        lv_obj_set_style_text_color(label, lv_color_hex(0xF3F4F6), LV_PART_MAIN);
        lv_obj_set_style_text_opa(label, LV_OPA_90, LV_PART_MAIN);
    } else if (strcmp(font_type, "artist") == 0) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x9CA3AF), LV_PART_MAIN);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN);
    } else if (strcmp(font_type, "info") == 0) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x6B7280), LV_PART_MAIN);
        lv_obj_set_style_text_opa(label, LV_OPA_70, LV_PART_MAIN);
    }
    
    // 启用子像素渲染（如果支持）
    #if defined(LV_FONT_SUBPX) && LV_FONT_SUBPX
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    #endif
}

/**
 * @brief 初始化字体系统
 */
int font_system_init(void)
{
    printf("字体系统初始化...\n");
    
    // 检查可用字体
    printf("可用字体检查:\n");
    
#if LV_FONT_MONTSERRAT_16
    printf("  Montserrat 16px - 可用\n");
#endif
#if LV_FONT_MONTSERRAT_20
    printf("  Montserrat 20px - 可用\n");
#endif
#if LV_FONT_MONTSERRAT_24
    printf("  Montserrat 24px - 可用\n");
#endif
#if LV_FONT_MONTSERRAT_28
    printf("  Montserrat 28px - 可用\n");
#endif
#if LV_FONT_MONTSERRAT_32
    printf("  Montserrat 32px - 可用\n");
#endif

#if LV_USE_FREETYPE
    printf("🇨🇳 检查MiSans中文字体支持...\n");
    
    // 检查字体文件是否存在
    if (access(MISANS_NORMAL_PATH, F_OK) == 0) {
        printf("  找到MiSans-Normal.ttf - 可用\n");
        // TODO: 在后续版本中启用FreeType字体加载
        printf("  📋 字体文件已就绪，暂时使用默认字体\n");
    } else {
        printf("  ⚠️ 未找到MiSans字体文件: %s\n", MISANS_NORMAL_PATH);
    }
#else
    printf("  📋 FreeType未启用，使用LVGL内置字体\n");
#endif

    // 测试UTF-8支持
    const char* test_text = "测试中文显示";
    if (text_contains_chinese(test_text)) {
        printf("  UTF-8中文字符检测正常\n");
    }
    
    font_system_initialized = true;
    printf("字体系统初始化完成\n");
    return 0;
}
