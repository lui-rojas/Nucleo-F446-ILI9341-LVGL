#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "lvgl.h"

typedef enum { LANG_EN, LANG_DE, LANG_PL, LANG_COUNT } lang_t;
typedef enum { 
    MSG_PLAY, 
    MSG_STOP, 
    MSG_SETTINGS, 
    MSG_RETURN,
    MSG_DARK_MODE, 
    MSG_LANGUAGE,
    MSG_COUNT 
} msg_id_t;

typedef struct {
    lv_color_t bg_color;
    lv_color_t primary_color;
    lv_color_t text_color;
} ui_theme_t;

const char* ui_get_text(msg_id_t id);
const ui_theme_t* ui_get_theme(void);

void ui_set_language(lang_t lang);
lang_t ui_get_language(void);

void ui_set_dark_mode(bool dark_mode);
bool ui_is_dark_mode(void);

#endif