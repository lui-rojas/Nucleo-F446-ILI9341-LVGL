#include "screen_settings.h"
#include "screen_home.h"
#include "widgets/button.h"
#include "ui_manager.h"

static void back_click_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED){
        screen_home_init();
    }
}

static void theme_switch_cb(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED){
        bool is_dark = lv_obj_has_state(sw, LV_STATE_CHECKED);
        ui_set_dark_mode(is_dark);

        screen_settings_init();
    }
}

static void lang_dropdown_cb(lv_event_t * e) {
    lv_obj_t * dropdown = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        uint16_t selected = lv_dropdown_get_selected(dropdown);
        ui_set_language((lang_t)selected);

        screen_settings_init();
    }
}

void screen_settings_init(void) {
    lv_obj_t * settings_src = lv_obj_create(NULL);

    const ui_theme_t* theme = ui_get_theme();
    lv_obj_set_style_bg_color(settings_src, theme->bg_color, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(settings_src);
    lv_label_set_text(title, ui_get_text(MSG_SETTINGS));
    lv_obj_set_style_text_color(title, theme->text_color, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Theme option

    lv_obj_t *lbl_theme = lv_label_create(settings_src);
    lv_label_set_text(lbl_theme, ui_get_text(MSG_DARK_MODE));
    lv_obj_set_style_text_color(lbl_theme, theme->text_color, LV_PART_MAIN);
    lv_obj_align(lbl_theme, LV_ALIGN_TOP_LEFT, 20, 60);

    lv_obj_t *sw = lv_switch_create(settings_src);
    lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -20, 55);
    if(ui_is_dark_mode()){
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw, theme_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //Language options
    lv_obj_t *lbl_lang = lv_label_create(settings_src);
    lv_label_set_text(lbl_lang, ui_get_text(MSG_LANGUAGE));
    lv_obj_set_style_text_color(lbl_lang, theme->text_color, LV_PART_MAIN);
    lv_obj_align(lbl_lang, LV_ALIGN_TOP_LEFT, 20, 120);

    lv_obj_t *dropdown = lv_dropdown_create(settings_src);
    lv_dropdown_set_options(dropdown, "English\nDeutsch\nPolski");
    lv_obj_set_width(dropdown, 110);
    lv_obj_align(dropdown, LV_ALIGN_TOP_RIGHT, -20, 110);

    lv_dropdown_set_selected(dropdown, ui_get_language());
    lv_obj_add_event_cb(dropdown, lang_dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);


    //return button
    lv_obj_t *back_btn = button_create(settings_src, MSG_RETURN, back_click_cb);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t *current_scr = lv_scr_act();
    if(current_scr != NULL && lv_obj_get_child_cnt(current_scr) > 0) {
        lv_obj_t *old_title = lv_obj_get_child(current_scr, 0);
        if(old_title && strcmp(lv_label_get_text(old_title), ui_get_text(MSG_SETTINGS)) == 0){
            lv_scr_load(settings_src);
            return;
        }
    }
 
    lv_scr_load_anim(settings_src, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
}