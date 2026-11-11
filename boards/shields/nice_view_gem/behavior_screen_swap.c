// boards/shields/nice_view_gem/behavior_screen_swap.c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include "widgets/screen.h"

#define DT_DRV_COMPAT zmk_behavior_screen_swap

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    zmk_widget_screen_toggle();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_screen_swap_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define SCREEN_SWAP_INST(n)                                                                        \
    static const struct behavior_config behavior_screen_swap_config_##n = {};                      \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_screen_swap_config_##n,                   \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                          &behavior_screen_swap_api);

DT_INST_FOREACH_STATUS_OKAY(SCREEN_SWAP_INST)
