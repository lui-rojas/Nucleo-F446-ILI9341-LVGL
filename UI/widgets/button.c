#include "button.h"

lv_obj_t* button_create(lv_obj_t* parent, msg_id_t text_id, lv_event_cb_t event_cb)
{
    const ui_theme_t* theme = ui_get_theme();

    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_style_bg_color(btn, theme->primary_color, LV_PART_MAIN);


    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, ui_get_text(text_id));
    lv_obj_set_style_text_color(label, theme->text_color, LV_PART_MAIN );
    lv_obj_center(label);

    if(event_cb != NULL){
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}