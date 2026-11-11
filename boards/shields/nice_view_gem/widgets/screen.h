// boards/shields/nice_view_gem/widgets/screen.h
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "util.h"

enum screen_id {
    SCREEN_0 = 0,  /* 默认状态屏 */
    SCREEN_1       /* 充电参数屏 */
};

struct zmk_widget_screen {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[SCREEN_CANVAS_W * SCREEN_CANVAS_H];
    struct status_state state;
    enum screen_id current_screen;
    bool user_override; /* 记录用户是否手动切离了充电屏 */
};

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget);

/* 供 Behavior 调用的屏幕切换接口 */
void zmk_widget_screen_toggle(void);
