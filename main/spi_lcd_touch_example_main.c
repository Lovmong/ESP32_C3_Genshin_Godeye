/*
 * @Author: VitoMao
 * @Date: 2025-04-29 11:14:39
 * @Last Modified by: VotoMao
 * @Last Modified time: 2025-04-29 11:45:25
 * @brief
 * 将电池电压替换成3.3V
 * @version v1.0
 *
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "ESP_LOG.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "ui/ui.h"
#include "config/config.h"
#include "esp_sleep.h"
#include "sofI2c.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_touch_cst816s.h"

#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "battery_monitor.h"

#include "screen_test.h"
#include "screen_god_eye.h"

static const char *TAG = "main";

#include "driver/adc.h"
uint8_t Version_app = 5;
uint8_t version_tp = 1;

#define LCD_HOST SPI2_HOST
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000) // TODO 修改此处

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_SCLK 5
#define EXAMPLE_PIN_NUM_MOSI 3
#define EXAMPLE_PIN_NUM_MISO -1
#define EXAMPLE_PIN_NUM_LCD_DC 6
#define EXAMPLE_PIN_NUM_LCD_RST -1
#define EXAMPLE_PIN_NUM_LCD_CS -1
#define EXAMPLE_PIN_NUM_BK_LIGHT 2
// 定义 LEDC 通道和定时器
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_FREQ_HZ 5000                // PWM 信号频率 5kHz
#define LEDC_RESOLUTION LEDC_TIMER_8_BIT // 8位分辨率，最大值255

#define EXAMPLE_PIN_NUM_I2C_TOUCH_SCL 0
#define EXAMPLE_PIN_NUM_I2C_TOUCH_SDA 1
#define EXAMPLE_PIN_NUM_I2C_TOUCH_RST -1
#define EXAMPLE_PIN_NUM_I2C_TOUCH_INT 9
#define I2C_TOUCH_FREQ 100000

#define EXAMPLE_LCD_H_RES 240
#define EXAMPLE_LCD_V_RES 240

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

#define EXAMPLE_LVGL_TICK_PERIOD_MS 1
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 300
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE (12 * 1024) // TODO 修改此处
#define EXAMPLE_LVGL_TASK_PRIORITY 2
uint8_t touchpad_cnt = 0;
#define GPIO_POWER_KEY_DET_10 10 // 电源开关检测
#define GPIO_PWR_EN_7 7          // 电源开关控制
#define DEBOUNCE_DELAY 50        // 按键去抖动延迟（单位：毫秒）

static SemaphoreHandle_t lvgl_mux = NULL;
lv_timer_t *Sleep_down = NULL;
int Sleep_down_Num = 90;     // 休眠计数 总数  30分钟 休眠 1800秒 180次
int Sleep_down_count = 0;    // 休眠计数  当前总数
int Sleep_down_HarfNum = 48; // 超过了8分钟 进入休眠 48
uint8_t Flag_is_sleep = 0;
uint8_t Flag_is_sleep1 = 0;
esp_lcd_touch_handle_t tp = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;
void setBacklight(uint8_t pwm);

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void example_lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

    switch (drv->rotated)
    {
    case LV_DISP_ROT_NONE:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
        // Rotate LCD touch
        esp_lcd_touch_set_mirror_y(tp, false);
        esp_lcd_touch_set_mirror_x(tp, false);
#endif
        break;
    case LV_DISP_ROT_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
        // Rotate LCD touch
        esp_lcd_touch_set_mirror_y(tp, false);
        esp_lcd_touch_set_mirror_x(tp, false);
#endif
        break;
    case LV_DISP_ROT_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
        // Rotate LCD touch
        esp_lcd_touch_set_mirror_y(tp, false);
        esp_lcd_touch_set_mirror_x(tp, false);
#endif
        break;
    case LV_DISP_ROT_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
        // Rotate LCD touch
        esp_lcd_touch_set_mirror_y(tp, false);
        esp_lcd_touch_set_mirror_x(tp, false);
#endif
        break;
    }
}
extern void showBLACK(void);

extern void clearBLACK(void);

#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
static void example_lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    /* Read touch controller data */
    esp_lcd_touch_read_data(drv->user_data);

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0)
    {
        Sleep_down_count = 0;
        if (1 == Flag_is_sleep)
        {
            Flag_is_sleep1 = 1;
            // lv_disp_trig_activity(NULL);
            // lv_indev_reset(NULL, data);
            // clearBLACK();
            ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
            setBacklight(80);
            // data->state = LV_INDEV_STATE_RELEASED;
            // current_touch_point.is_pressed = 0;
            ESP_LOGI("触摸坐标Flag_is_sleep", ":X:%d Y:%d", data->point.x, data->point.y);
        }
        else
        {
            data->point.x = touchpad_x[0];
            // data->point.y = touchpad_y[0] + 10; //如果是倒过来 那么需要+10
            data->point.y = touchpad_y[0];
            // data->point.y = touchpad_y[0];
            data->state = LV_INDEV_STATE_PRESSED;

            // setBacklight(game_state.back_light);
            // ESP_LOGI("触摸坐标", ":X:%d Y:%d", data->point.x, data->point.y); // 修正这里
            current_touch_point.x = touchpad_x[0];
            current_touch_point.y = touchpad_y[0] + 10;
            current_touch_point.is_pressed = 1;
        }
    }
    else
    {
        if (Flag_is_sleep1)
        {
            Flag_is_sleep = 0;
            Flag_is_sleep1 = 0;
            ESP_LOGI("切换", ":X:%d Y:%d", data->point.x, data->point.y);
        }
        // ESP_LOGI("无触摸", ":X:%d Y:%d", data->point.x, data->point.y);
        data->state = LV_INDEV_STATE_RELEASED;
        current_touch_point.is_pressed = 0;
    }
}
#endif

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

bool example_lvgl_lock(int timeout_ms)
{
    // Convert timeout in milliseconds to FreeRTOS ticks
    // If `timeout_ms` is set to -1, the program will block until the condition is met
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void example_lvgl_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

static void example_lvgl_port_task(void *arg)
{
    // //////ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    while (1)
    {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (example_lvgl_lock(-1))
        {
            task_delay_ms = lv_timer_handler();
            // Release the mutex
            example_lvgl_unlock();
        }
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS)
        {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        }
        else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS)
        {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

i2c_master_bus_handle_t i2c_bus_handle;

/**
 * @brief i2c master initialization
 */
static void i2c_example_master_init()
{
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = EXAMPLE_PIN_NUM_I2C_TOUCH_SCL,
        .sda_io_num = EXAMPLE_PIN_NUM_I2C_TOUCH_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));
}
// 定义中断服务（ISR）
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    // //////ESP_LOGI(TAG, "Turn off");
    int current_level = gpio_get_level(GPIO_PWR_EN_7);
    gpio_set_level(GPIO_PWR_EN_7, !current_level);
}
// 按键检测 充电检测
void button_task(void *arg)
{
    while (1)
    {
        // 获取GPIO10引脚的当前电平
        bool button_state = gpio_get_level(GPIO_POWER_KEY_DET_10);

        // 如果GPIO10为高，则拉低GPIO7
        if (button_state == 1)
        {
            gpio_set_level(GPIO_PWR_EN_7, 0); // 拉低GPIO7
            // //////ESP_LOGI("Button", "Button pressed, GPIO7 set to LOW.");
            //  为了避免反复检测，在GPIO10被拉高时等待一段时间（去抖动）
            vTaskDelay(DEBOUNCE_DELAY / portTICK_PERIOD_MS);
        }

        // game_state.Battate = gpio_get_level(GPIO_NUM_20);
        // ////////ESP_LOGI("Game state", "充电检测%d", game_state.Battate);

        // 每10ms检查一次GPIO10的状态
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}
static void PowerIO_init()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_POWER_KEY_DET_10),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLDOWN_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE // 设置中断为上升沿触发
    };
    gpio_config(&io_conf);
    // 设置 GPIO7 为输出模式
    gpio_reset_pin(GPIO_NUM_7);                       // 重置 GPIO 配置（确保干净状态）
    gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT); // 设置为输出模式
    gpio_set_level(GPIO_NUM_7, 1);

    // gpio_reset_pin(GPIO_NUM_20);
    // gpio_set_direction(GPIO_NUM_20, GPIO_MODE_INPUT); // 设置为输入模式
}
static void off_power()
{
    // //////ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_set_level(GPIO_NUM_7, 0);
}
// 初始化背光
void backlight_init(void)
{
    // 配置 LEDC 定时器
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_RESOLUTION, // 分辨率
        .freq_hz = LEDC_FREQ_HZ,            // PWM频率
        .speed_mode = LEDC_LOW_SPEED_MODE,  // 高速模式
        .timer_num = LEDC_TIMER             // 定时器编号
    };
    ledc_timer_config(&ledc_timer);

    // 配置 LEDC 通道
    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL,
        .duty = 0,                            // 初始占空比为0（暗）
        .gpio_num = EXAMPLE_PIN_NUM_BK_LIGHT, // 控制背光的GPIO
        .speed_mode = LEDC_LOW_SPEED_MODE,    // 高速模式
        .hpoint = 0,                          // 默认值
        .timer_sel = LEDC_TIMER               // 使用定时器
    };
    ledc_channel_config(&ledc_channel);
}

// 设置背光亮度，0-255对应0%-100%的亮度
void setBacklight(uint8_t pwm)
{
    // 限制pwm值在0-100之间
    if (pwm > 100)
        pwm = 100;
    // 将pwm值从百分比转换为0-255范围的值
    uint8_t duty = (pwm * 255) / 100;
    // 设置PWM占空比
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
}

// 初始化 ADC
void adc_init()
{
    battery_monitor_init();
    // float voltage = battery_monitor_get_voltage();
    // float percentage = battery_monitor_get_percentage();

    // //////ESP_LOGI("BAT", "Battery Voltage: %.2f V", voltage);
    // //////ESP_LOGI("BAT", "Battery Percentage: %.2f%%", percentage);
}

void Sleep_down_fun(lv_timer_t *timer)
{
    bool IsopenTime = 0;
    // 休眠10秒倒计时 30s 无人操作 即亮一半的亮度
    if (IsopenTime == 1)
    {
        Sleep_down_count = 0;
    }
    else
    {
        Sleep_down_count++;
        // ESP_LOGI("拉闸", "Sleep_down_count：%d", Sleep_down_count);
    }
    if (Sleep_down_count >= Sleep_down_HarfNum && 0 == Flag_is_sleep)
    {
        setBacklight(0);
        Flag_is_sleep = 1;
        ESP_LOGI("拉闸", "进入休眠");
        // showBLACK();
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
    }
    if (Sleep_down_count >= Sleep_down_Num)
    {
        // 配置 GPIO9 为输入引脚，并启用上拉电阻
        // gpio_config_t io_conf = {
        //     .pin_bit_mask = (1ULL << GPIO_WAKEUP_PIN), // 设置 GPIO9
        //     .mode = GPIO_MODE_INPUT,                   // 配置为输入模式
        //     .pull_up_en = GPIO_PULLUP_ENABLE,          // 启用上拉电阻
        //     .pull_down_en = GPIO_PULLDOWN_DISABLE,     // 禁用下拉电阻
        //     .intr_type = GPIO_INTR_DISABLE             // 禁用中断
        // };
        // gpio_config(&io_conf);

        // 配置 GPIO9 为高电平唤醒源
        // esp_sleep_enable_ext0_wakeup(EXAMPLE_PIN_NUM_I2C_TOUCH_INT, 0); // 5脚低电平唤醒
        //  esp_sleep_enable_ext0_wakeup();

        // //////ESP_LOGI("拉闸", "睡觉");
        // esp_light_sleep_start(); // 进入轻休眠模式
        // ESP_LOGI("拉闸", "Sleep_down_count：%d", Sleep_down_count);
        off_power();
        // //////ESP_LOGI("拉闸", "睡眠结束");
    }
}
uint8_t read_Version_tp()
{
    gpio_reset_pin(1);                       // 重置 GPIO 配置（确保干净状态）
    gpio_set_direction(1, GPIO_MODE_OUTPUT); // 设置为输出模式
    gpio_reset_pin(0);                       // 重置 GPIO 配置（确保干净状态）
    gpio_set_direction(0, GPIO_MODE_OUTPUT); // 设置为输出模式
    SW_I2C1_Init();
    write_addr(0x2A);
    uint8_t buf[1] = {0};
    Read_I2C_ByteN(0xA9, buf, 1);
    return buf[0];
}

void app_main(void)
{
    PowerIO_init();
    version_tp = read_Version_tp();
    ESP_LOGI("TEST", "当前版本号: %d", version_tp);
    adc_init();

    backlight_init();
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,

        .trans_queue_depth = 10,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    ESP_LOGI("LCD SPI 频率: ", "%d", EXAMPLE_LCD_PIXEL_CLOCK_HZ);

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    tp_io_config.scl_speed_hz = I2C_TOUCH_FREQ;

    i2c_example_master_init();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = EXAMPLE_PIN_NUM_I2C_TOUCH_RST,
        .int_gpio_num = EXAMPLE_PIN_NUM_I2C_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        // .interrupt_callback = example_lvgl_touch_cb,
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &tp));
    lv_init();
    lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 20);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.drv_update_cb = example_lvgl_port_update_callback;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    // TODO 旋转屏幕
    disp_drv.rotated = 0;
    disp_drv.sw_rotate = 0;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    static lv_indev_drv_t indev_drv; // Input device driver (Touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_lvgl_touch_cb;
    indev_drv.user_data = tp;
    lv_indev_drv_register(&indev_drv);

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);

    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    if (example_lvgl_lock(-1))
    {
        // example_lvgl_demo_ui(disp);
        // ui_init();
        // lv_color_switch_test();
        lv_god_eye_screen();
        // Release the mutex
        example_lvgl_unlock();
    }

    // 定时器 休眠
    // setBacklight(game_state.back_light);
    setBacklight(80);

    Sleep_down = lv_timer_create(Sleep_down_fun, 10000, NULL);
    while (1 == gpio_get_level(GPIO_POWER_KEY_DET_10))
    {
        // 每10ms检查一次GPIO10的状态
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
