
#include "lvgl.h"
#include "screen_god_eye.h"
#include <stdio.h>

extern void setBacklight(uint8_t pwm);

LV_IMG_DECLARE(god_eye_fire);
LV_IMG_DECLARE(god_eye_water);
LV_IMG_DECLARE(god_eye_wind);
LV_IMG_DECLARE(god_eye_ray);
LV_IMG_DECLARE(god_eye_grass);
LV_IMG_DECLARE(god_eye_ice);
LV_IMG_DECLARE(god_eye_rock);

static lv_timer_t *_timer = NULL; // 定时器句柄
static lv_obj_t *bg_obj = NULL;
static lv_obj_t *img = NULL;

static const lv_img_dsc_t *eyes_list[7] = {
    &god_eye_fire,
    &god_eye_water,
    &god_eye_wind,
    &god_eye_ray,
    &god_eye_grass,
    &god_eye_ice,
    &god_eye_rock,
};

static uint8_t brightness = 128;
static int8_t step = 1;

// 定时器回调函数 - 切换颜色
static void _timer_cb(lv_timer_t *timer)
{
    (void)timer; // 避免未使用变量警告

    if (brightness == 128)
        step = -1;
    else if (brightness == 0)
        step = 1;

    brightness += step;

    // ESP_LOGI(NULL, "brightness = %d\n", brightness);
    // printf("brightness = %d\n", brightness);
    setBacklight(brightness);
}

static void _click_cb(lv_event_t *e)
{
    (void)e;

    static uint8_t idx = 0;

    idx = (idx + 1) % 7;
    lv_img_set_src(img, eyes_list[idx]);

    brightness = 128;
    step = -1;
    setBacklight(brightness);
}

// 创建界面
void lv_god_eye_screen(void)
{
    bg_obj = lv_obj_create(NULL);
    lv_obj_set_size(bg_obj, LV_HOR_RES, LV_VER_RES);
    //
    lv_obj_add_flag(bg_obj, LV_OBJ_FLAG_CLICKABLE);

    // 去掉边框、圆角
    lv_obj_set_style_border_width(bg_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bg_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 1. 创建图片对象
    img = lv_img_create(bg_obj);

    // 2. 设置图片源为 C 数组的指针
    lv_img_set_src(img, &god_eye_fire);

    // 3. 居中显示
    lv_obj_center(img);

    // 创建定时器：每100ms 调整一次背光亮度
    _timer = lv_timer_create(_timer_cb, 30, NULL);

    // 创建点击事件回调
    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(img, _click_cb, LV_EVENT_CLICKED, NULL);

    lv_scr_load(bg_obj);
}

// 可选：添加定时器销毁函数（如需手动停止切换）
void lv_god_eye_screen_stop(void)
{
    if (_timer)
    {
        lv_timer_del(_timer);
        _timer = NULL;
    }
}
