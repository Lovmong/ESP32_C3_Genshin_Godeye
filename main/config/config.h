#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h" // 包含 LVGL 头文件
#include <stdio.h>
#include <stdint.h>

    typedef struct
    {
        int x;           // 触摸点的 X 坐标
        int y;           // 触摸点的 Y 坐标
        bool is_pressed; // 当前是否处于按下状态
    } TouchPoint_t;
    extern TouchPoint_t current_touch_point;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // CONFIG_H
