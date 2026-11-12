// boards/shields/nice_view_gem/widgets/screen_charge.c
#include "screen_charge.h"
#include "util.h"
#include "../assets/custom_fonts.h"
#include <stdio.h>

/* 提取并格式化电流值 */
static void format_current(char *buf, size_t len, uint16_t current_ma) {
    if (current_ma < 1000) {
        snprintf(buf, len, "%umA", current_ma);
    } else {
        snprintf(buf, len, "%u.%02uA", current_ma / 1000, (current_ma % 1000) / 10);
    }
}

/* 提取并格式化时间值 */
static void format_duration(char *buf, size_t len, uint32_t seconds) {
    uint32_t mins = seconds / 60;
    if (mins < 60) {
        snprintf(buf, len, "%um", mins);
    } else {
        snprintf(buf, len, "%uh%02um", mins / 60, mins % 60);
    }
}

void draw_charge_screen(lv_obj_t *canvas, const struct owlet_charger_state_changed *state) {
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &pixel_operator_mono, LV_TEXT_ALIGN_LEFT);

    char text[32];

    /* screen1 专属中分线 (X=80) */
    lv_draw_rect_dsc_t line_dsc;
    init_rect_dsc(&line_dsc, LVGL_FOREGROUND);
    lv_canvas_draw_rect(canvas, 80, 4, 1, 61, &line_dsc);

    /* --- 左栏 (X = 4, 宽 76) --- */
    
    // 1. 充电接口类型
    const char *type_str = "UNK";
    if (state->vbus_stat == 1 || state->vbus_stat == 2) {
        type_str = "USB";
    } else if (state->vbus_stat == 3) {
        type_str = "CHG";
    }
    snprintf(text, sizeof(text), "Type:%s", type_str); 
    lv_canvas_draw_text(canvas, 4, 8, 76, &label_dsc, text);

    // 2. 设定的充电电流 (使用 ichg_ma)
    char cur_str[16];
    format_current(cur_str, sizeof(cur_str), state->ichg_ma);
    snprintf(text, sizeof(text), "Set:%s", cur_str); 
    lv_canvas_draw_text(canvas, 4, 28, 76, &label_dsc, text);

    // 3. 充电时长
    char time_str[16];
    format_duration(time_str, sizeof(time_str), state->charge_duration_s);
    snprintf(text, sizeof(text), "Time:%s", time_str); 
    lv_canvas_draw_text(canvas, 4, 48, 76, &label_dsc, text);

    /* --- 右栏 (X = 84, 宽 76) --- */

    // 1. 电池电量
    snprintf(text, sizeof(text), "SOC:%u%%", state->battery_soc); 
    lv_canvas_draw_text(canvas, 84, 8, 76, &label_dsc, text);

    // 2. 充电状态 (精简文案)
    const char *chrg_str = "IDLE";
    if (state->chrg_stat == 1) {
        chrg_str = "PRE";
    } else if (state->chrg_stat == 2) {
        chrg_str = "FST";
    } else if (state->chrg_stat == 3) {
        chrg_str = "FULL";
    }
    snprintf(text, sizeof(text), "Chg:%s", chrg_str); 
    lv_canvas_draw_text(canvas, 84, 28, 76, &label_dsc, text);

    // 3. 电池电压 (替换原温度，使用整数运算实现两位小数显示)
    uint16_t mv = state->battery_voltage_mv;
    snprintf(text, sizeof(text), "Volt:%u.%02uV", mv / 1000, (mv % 1000) / 10);
    lv_canvas_draw_text(canvas, 84, 48, 76, &label_dsc, text);
}
