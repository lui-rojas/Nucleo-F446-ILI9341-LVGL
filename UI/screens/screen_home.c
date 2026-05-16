#include "screen_home.h"
#include "screen_settings.h"
#include "widgets/button.h"
#include "ui_manager.h"

#include <string.h>
#include <stdio.h>


static void settings_click_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED){

        char dbg[64];
        snprintf(dbg, sizeof(dbg), "Open Settings \r\n");
        HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 10);
        screen_settings_init();
    }
}

void screen_home_init(void) {
    lv_obj_t *home_scr = lv_obj_create(NULL);
    const ui_theme_t* theme = ui_get_theme();
    lv_obj_set_style_bg_color(home_scr, theme->bg_color, LV_PART_MAIN);

    lv_obj_t* btn = button_create(home_scr, MSG_SETTINGS, settings_click_cb);
    lv_obj_align(btn, LV_ALIGN_CENTER,0,0);
 
    lv_scr_load_anim(home_scr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, true);
}