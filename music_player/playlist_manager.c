// Playlist Manager

#include "music_player.h"
#include "playlist_manager.h"
#include "font_config.h"
#include <stdio.h>

// External variables
extern struct resource_s R;
extern struct ctx_s C;

// Defines
#define MAX_PLAYLIST_ITEMS 6
#define PLAYLIST_ITEM_HEIGHT 70
#define PLAYLIST_NO_ANIMATION
// Variables
static lv_obj_t* playlist_container = NULL;
static bool playlist_is_open = false;

// Functions
static void create_playlist_item(lv_obj_t* parent, int index);
static void playlist_item_click_cb(lv_event_t* e);
static void playlist_close_cb(lv_event_t* e);
static void create_playlist_item(lv_obj_t* parent, int index) {
    if (index >= R.album_count || index >= MAX_PLAYLIST_ITEMS) {
        return;
    }
    
    // 最简单的按钮 - 完全对应emergency逻辑
    lv_obj_t* btn = lv_btn_create(parent);
    if (!btn) return;
    
    lv_obj_set_size(btn, LV_PCT(100), PLAYLIST_ITEM_HEIGHT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x4A4A4A), LV_PART_MAIN | LV_STATE_PRESSED);  // 按下时的颜色
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);  // 圆角
    lv_obj_set_style_margin_bottom(btn, 4, LV_PART_MAIN);  // 项目间距
    
    // 增强的标签 - 使用更大字体和更好布局
    lv_obj_t* label = lv_label_create(btn);
    if (!label) return;
    
    char text[80];
    const char* name = R.albums[index].name ? R.albums[index].name : "Unknown";
    snprintf(text, sizeof(text), "%d. %s", index + 1, name);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    // 应用播放列表歌曲字体
    const lv_font_t* playlist_font = get_playlist_font("song");
    if (playlist_font) {
        lv_obj_set_style_text_font(label, playlist_font, LV_PART_MAIN);
    }
    
    // 设置文本颜色为白色，更清晰
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    
    // 点击事件 - 完全对应emergency逻辑
    lv_obj_add_event_cb(btn, playlist_item_click_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)index);
}

/**
 * @brief 播放列表项点击事件 - 完全对应emergency_item_click
 */
static void playlist_item_click_cb(lv_event_t* e) {
    int index = (int)(uintptr_t)lv_event_get_user_data(e);
    
    if (index >= 0 && index < R.album_count) {
        // 直接切换歌曲 - 完全对应emergency逻辑
        extern void app_switch_to_album(int index);
        app_switch_to_album(index);
        
        // 关闭播放列表 - 完全对应emergency逻辑
        playlist_manager_close();
    }
}

/**
 * @brief 关闭播放列表回调 - 完全对应emergency_close_cb
 */
static void playlist_close_cb(lv_event_t* e) {
    playlist_manager_close();
}

/*********************
 * GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 创建播放列表 - 完全对应emergency_playlist_create
 */
void playlist_manager_create(lv_obj_t* parent) {
    if (playlist_container || !parent) {
        return;
    }
    
    // 移除printf调用以避免多线程死锁
    
    // 最简单的全屏容器 - 完全对应emergency逻辑
    playlist_container = lv_obj_create(parent);
    if (!playlist_container) {
        // 移除printf调用以避免多线程死锁
        return;
    }
    
    lv_obj_remove_style_all(playlist_container);
    lv_obj_set_size(playlist_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(playlist_container, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(playlist_container, LV_OPA_80, LV_PART_MAIN);
    
    // 点击背景关闭 - 完全对应emergency逻辑
    lv_obj_add_flag(playlist_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(playlist_container, playlist_close_cb, LV_EVENT_CLICKED, NULL);
    
    // 简单的内容区域 - 完全对应emergency逻辑
    lv_obj_t* content = lv_obj_create(playlist_container);
    if (!content) {
        lv_obj_del(playlist_container);
        playlist_container = NULL;
        return;
    }
    
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, LV_PCT(90), LV_PCT(80));  // 增大播放列表尺寸 (从80%x60%增加到90%x80%)
    lv_obj_center(content);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(content, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(content, 8, LV_PART_MAIN);
    
    // 阻止事件冒泡 - 完全对应emergency逻辑
    lv_obj_remove_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);
    
    // 增强标题 - 使用更大字体和更好样式
    lv_obj_t* title = lv_label_create(content);
    if (title) {
        lv_label_set_text(title, "Playlist");  // 使用英文标题避免字体显示问题
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_margin_bottom(title, 12, LV_PART_MAIN);  // 增加底部边距
        
        // 应用播放列表标题字体
        const lv_font_t* title_font = get_playlist_font("title");
        if (title_font) {
            lv_obj_set_style_text_font(title, title_font, LV_PART_MAIN);
        }
        
        // 标题居中对齐
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }
    
    // 创建有限的列表项 - 完全对应emergency逻辑
    int max_items = (R.album_count < MAX_PLAYLIST_ITEMS) ? R.album_count : MAX_PLAYLIST_ITEMS;
    for (int i = 0; i < max_items; i++) {
        create_playlist_item(content, i);
    }
    
    playlist_is_open = true;
    // 移除printf调用以避免多线程死锁
}

/**
 * @brief 关闭播放列表 - 完全对应emergency_playlist_close
 */
void playlist_manager_close(void) {
    if (playlist_container) {
        lv_obj_del(playlist_container);
        playlist_container = NULL;
    }
    playlist_is_open = false;
    // 移除printf调用以避免多线程死锁
}

/**
 * @brief 检查播放列表是否打开 - 完全对应emergency_playlist_is_active
 */
bool playlist_manager_is_open(void) {
    return playlist_is_open;
}

/**
 * @brief 刷新播放列表 - 简单的重新创建逻辑
 */
void playlist_manager_refresh(void) {
    if (playlist_is_open) {
        // 如果已经打开，先关闭再重新创建
        lv_obj_t* parent = lv_obj_get_parent(playlist_container);
        playlist_manager_close();
        if (parent) {
            playlist_manager_create(parent);
        }
    }
}