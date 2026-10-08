#include "lvgl.h"
#include "screen_test.h"

// 颜色列表
static uint32_t color_list[] = {
    0xFFFFFF, // 白
    0xFF0000, // 红
    0x00FF00, // 绿
    0x0000FF, // 蓝
    0xFFFF00, // 黄
    0x808080, // 灰
    0x000000, // 黑
};

#define COLOR_COUNT (sizeof(color_list) / sizeof(color_list[0]))
static uint8_t current_color_idx = 0;
static lv_obj_t *bg_obj;
static lv_timer_t *color_switch_timer = NULL; // 定时器句柄

// 定时器回调函数 - 切换颜色
static void color_switch_timer_cb(lv_timer_t *timer)
{
    (void)timer; // 避免未使用变量警告

    current_color_idx++;
    if (current_color_idx >= COLOR_COUNT)
        current_color_idx = 0;

    // 切换背景色（保持原有格式）
    lv_obj_set_style_bg_color(
        bg_obj,
        lv_color_hex(color_list[current_color_idx]),
        LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void color_switch_click_cb()
{
    current_color_idx++;
    if (current_color_idx >= COLOR_COUNT)
        current_color_idx = 0;

    // 切换背景色（保持原有格式）
    lv_obj_set_style_bg_color(
        bg_obj,
        lv_color_hex(color_list[current_color_idx]),
        LV_PART_MAIN | LV_STATE_DEFAULT);
}

// 创建界面
void lv_color_switch_test(void)
{
    bg_obj = lv_obj_create(NULL);
    lv_obj_set_size(bg_obj, LV_HOR_RES, LV_VER_RES);
    //
    lv_obj_add_flag(bg_obj, LV_OBJ_FLAG_CLICKABLE);

    // 去掉边框、圆角
    lv_obj_set_style_border_width(bg_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bg_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 初始背景色（保持原有格式）
    lv_obj_set_style_bg_color(
        bg_obj,
        lv_color_hex(color_list[current_color_idx]),
        LV_PART_MAIN | LV_STATE_DEFAULT);

    // 创建定时器：每3000ms（3秒）执行一次回调，无用户数据
    // color_switch_timer = lv_timer_create(color_switch_timer_cb, 3000, NULL);

    // 创建点击事件回调
    lv_obj_add_event_cb(bg_obj, color_switch_click_cb, LV_EVENT_CLICKED, NULL);

    lv_scr_load(bg_obj);
}

// 可选：添加定时器销毁函数（如需手动停止切换）
void lv_color_switch_test_stop(void)
{
    if (color_switch_timer)
    {
        lv_timer_del(color_switch_timer);
        color_switch_timer = NULL;
    }
}
