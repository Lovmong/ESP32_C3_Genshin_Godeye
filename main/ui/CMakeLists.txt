SET(SOURCES screens/ui_Main.c
    screens/ui_TMain.c
    screens/ui_Set.c
    screens/ui_SysSet.c
    screens/ui_PSet.c
    screens/ui_Keymap.c
    screens/ui_Timer.c
    screens/ui_History.c
    screens/ui_CountSet1.c
    screens/ui_BLACK.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/god_eye_fire.c
    images/god_eye_water.c
    images/god_eye_wind.c
    images/god_eye_ray.c
    images/god_eye_grass.c
    images/god_eye_ice.c
    images/god_eye_rock.c
    fonts/ui_font_BigFont10.c
    fonts/ui_font_BigFont100.c
    fonts/ui_font_BigFontL999.c
    fonts/ui_font_TwoBigFont10.c
    fonts/ui_font_TwoBigFont100.c
    fonts/ui_font_TwoBigFontL100.c)

add_library(ui ${SOURCES})
