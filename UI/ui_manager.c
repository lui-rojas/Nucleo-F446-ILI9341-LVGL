#include "ui_manager.h"
#include <stddef.h>

static lang_t current_language = LANG_EN;
static bool is_dark_mode = true;

static const ui_theme_t dark_theme = {
    .bg_color = LV_COLOR_MAKE(30, 30, 30),
    .primary_color = LV_COLOR_MAKE(0, 122, 255),
    .text_color = LV_COLOR_MAKE(255, 255 , 255)
};

static const ui_theme_t light_theme = {
    .bg_color = LV_COLOR_MAKE(240, 240, 240),
    .primary_color = LV_COLOR_MAKE(90, 122, 255),
    .text_color = LV_COLOR_MAKE(0,0,0)
};

static const char* const translations[LANG_COUNT][MSG_COUNT] = {
    [LANG_EN] = {
        [MSG_SETTINGS] = "Settings",
        [MSG_RETURN] = "Return",
        [MSG_DARK_MODE] = "Dark Mode",
        [MSG_LANGUAGE] = "Language"
    },
    [LANG_DE] = {
        [MSG_SETTINGS] = "Einstellungen",
        [MSG_RETURN] = "Return",
        [MSG_DARK_MODE] = "Dunkler Modus",
        [MSG_LANGUAGE] = "Sprache"
    },
    [LANG_PL] = {
        [MSG_SETTINGS] = "Ustawienia",
        [MSG_RETURN] = "Wstecz",
        [MSG_DARK_MODE] = "Tryb ciemny",
        [MSG_LANGUAGE] = "Jezyk"
    }
};

const char* ui_get_text(msg_id_t id){
    if(id >= MSG_COUNT) return "";
    return translations[current_language][id];
}
 
const ui_theme_t* ui_get_theme(void) {
    if(is_dark_mode) {
        return &dark_theme;
    }else{
        return &light_theme;
    }
}

void ui_set_language(lang_t lang){
    if(lang < LANG_COUNT) current_language = lang;
}

lang_t ui_get_language(void) {
    return current_language;
}

void ui_set_dark_mode(bool is_dark) {
    is_dark_mode = is_dark;
}

bool ui_is_dark_mode(void){
    return is_dark_mode;
}