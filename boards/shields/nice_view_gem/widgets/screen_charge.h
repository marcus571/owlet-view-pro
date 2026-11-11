// boards/shields/nice_view_gem/widgets/screen_charge.h
#pragma once

#include <lvgl.h>
#include "owlet_events.h"

void draw_charge_screen(lv_obj_t *canvas, const struct owlet_charger_state_changed *state);
