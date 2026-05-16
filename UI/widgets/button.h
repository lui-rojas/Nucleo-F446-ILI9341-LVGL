#ifndef BUTTON_H
#define BUTTON_H

#include "ui_manager.h"

lv_obj_t* button_create(lv_obj_t* parent, msg_id_t text_id, lv_event_cb_t event_cb);

#endif