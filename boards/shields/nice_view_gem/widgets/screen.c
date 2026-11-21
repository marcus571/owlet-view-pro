// boards/shields/nice_view_gem/widgets/screen.c
//
// 2026-11-21 修改记录（第六轮）：
//   输出状态增加 usb_hid_ready 字段（取自 zmk_usb_is_hid_ready()），
//   供中栏 USB 图标使用，修复开机早期端点零初始化(USB=0)导致的图标误显示。
//   其余逻辑与第五轮一致：拔线立即清零回主屏、充满未拔线停留充电屏、
//   充电事件走 ZMK_DISPLAY_WIDGET_LISTENER、所有 LVGL 绘制在 display
//   线程执行、swap_work 同样提交到 display 队列、无调试计数器。
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>

#include "battery.h"
#include "layer.h"
#include "output.h"
#include "screen.h"
#include "screen_charge.h"

/* 直接引用主仓库暴露出来的头文件 */
#include "owlet_charger_event.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/* 保存最新的充电状态（只在 display 线程读写） */
static struct owlet_charger_state_changed current_charger_state = {0};

/* 分隔线 */
#define SEPARATOR_Y        4
#define SEPARATOR_H        61
#define SEPARATOR_LEFT_X   53
#define SEPARATOR_RIGHT_X  106

static void draw_separator(lv_obj_t *canvas, int x) {
    lv_draw_rect_dsc_t dsc;
    init_rect_dsc(&dsc, LVGL_FOREGROUND);
    lv_canvas_draw_rect(canvas, x, SEPARATOR_Y, 1, SEPARATOR_H, &dsc);
}

static void draw_all(lv_obj_t *widget_obj, struct zmk_widget_screen *screen_widget) {
    lv_obj_t *canvas = lv_obj_get_child(widget_obj, 0);
    fill_background(canvas);

    if (screen_widget->current_screen == SCREEN_0) {
        draw_battery_status(canvas, &screen_widget->state);
        draw_output_status(canvas, &screen_widget->state);
        draw_layer_status(canvas, &screen_widget->state);
        draw_separator(canvas, SEPARATOR_LEFT_X);
        draw_separator(canvas, SEPARATOR_RIGHT_X);
    } else {
        draw_charge_screen(canvas, &current_charger_state);
    }

    /* 强制 LVGL 刷新该区域，解决修改了 cbuf 但不刷屏的问题 */
    lv_obj_invalidate(widget_obj);
}

/* 在 display 线程执行屏幕切换 */
static void swap_work_handler(struct k_work *work) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->current_screen = (widget->current_screen == SCREEN_0) ? SCREEN_1 : SCREEN_0;

        if (widget->current_screen == SCREEN_0 && current_charger_state.is_charging) {
            widget->user_override = true;
        } else {
            widget->user_override = false;
        }

        draw_all(widget->obj, widget);
    }
}
static K_WORK_DEFINE(swap_work, swap_work_handler);

void zmk_widget_screen_toggle(void) {
    k_work_submit_to_queue(zmk_display_work_q(), &swap_work);
}

/* ---------- 充电状态：在 display 线程更新 UI ---------- */
static void charger_status_update_cb(struct owlet_charger_state_changed state) {
    current_charger_state = state;

    struct zmk_widget_screen *widget;

    if (state.is_charging) {
        SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
            /* 只有用户没有手动切走时，才强制锁定在 SCREEN_1；
             * 充满未拔线时 is_charging 仍为 true，充电屏保持停留 */
            if (!widget->user_override) {
                widget->current_screen = SCREEN_1;
            }
            draw_all(widget->obj, widget);
        }
    } else {
        /* 拔线：数据已清零，立即切回主屏，不再延时回落 */
        SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
            widget->user_override = false;
            widget->current_screen = SCREEN_0;
            draw_all(widget->obj, widget);
        }
    }
}

static struct owlet_charger_state_changed
charger_status_get_state(const zmk_event_t *eh) {
    struct owlet_charger_state_changed empty = {0};

    if (eh == NULL) {
        return empty;
    }

    const struct owlet_charger_state_changed *ev = as_owlet_charger_state_changed(eh);
    if (ev == NULL) {
        return empty;
    }

    return *ev;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_charger_status, struct owlet_charger_state_changed,
                            charger_status_update_cb, charger_status_get_state);
ZMK_SUBSCRIPTION(widget_charger_status, owlet_charger_state_changed);

/* ---------- 电量 ---------- */
static void set_battery_status(struct zmk_widget_screen *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif
    widget->state.battery = state.level;
    draw_all(widget->obj, widget);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    uint8_t level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge();

    if (level >= 95) {
        level = 100;
    }

    return (struct battery_status_state){
        .level = level,
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state);
ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif

/* ---------- 图层 ---------- */
static void set_layer_status(struct zmk_widget_screen *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_mask = state.layer_mask;
    draw_all(widget->obj, widget);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    uint32_t mask = zmk_keymap_layer_state();

    mask |= (1U << zmk_keymap_layer_default());
    mask |= (1U << index);

    return (struct layer_status_state){
        .index = index,
        .layer_mask = mask
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

/* ---------- 输出/连接 ---------- */
static void set_output_status(struct zmk_widget_screen *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;
    widget->state.usb_hid_ready = state->usb_hid_ready;
    draw_all(widget->obj, widget);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
        .usb_hid_ready = zmk_usb_is_hid_ready(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, SCREEN_CANVAS_W, SCREEN_CANVAS_H);

    widget->current_screen = SCREEN_0; /* 默认启动 Screen 0 */
    widget->user_override = false;

    lv_obj_t *canvas = lv_canvas_create(widget->obj);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, widget->cbuf, SCREEN_CANVAS_W, SCREEN_CANVAS_H,
                         LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_layer_status_init();
    widget_output_status_init();
    widget_charger_status_init();

    struct status_state initial_state = {0};

    uint8_t init_level = zmk_battery_state_of_charge();
    initial_state.battery = (init_level >= 95) ? 100 : init_level;

    initial_state.layer_index = zmk_keymap_highest_layer_active();
    initial_state.layer_mask = (1U << zmk_keymap_layer_default()) | (1U << initial_state.layer_index);
    widget->state = initial_state;

    draw_all(widget->obj, widget);

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) { return widget->obj; }
