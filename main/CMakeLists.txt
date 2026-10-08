file(GLOB_RECURSE SRC_SOURCES
    components/*.c
    ui/fonts/*.c
    ui/images/*.c
    ui/screens/*.c
)

idf_component_register(
    SRCS "spi_lcd_touch_example_main.c"
         "config/config.c"
         "ui/ui_helpers.c"
         "ui/ui.c"
         ${SRC_SOURCES}
         "ui/ui_events.c"
         "battery_monitor.c"
         "screen_test.c"
         "screen_god_eye.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES bt
    REQUIRES nvs_flash sofI2c
)
