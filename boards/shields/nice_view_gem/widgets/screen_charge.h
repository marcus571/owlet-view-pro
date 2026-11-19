// boards/shields/nice_view_gem/widgets/screen_charge.h
#pragma once

#include <lvgl.h>
/* 直接引用主仓库暴露出来的头文件 */
#include "owlet_charger_event.h"

void draw_charge_screen(lv_obj_t *canvas, const struct owlet_charger_state_changed *state);
