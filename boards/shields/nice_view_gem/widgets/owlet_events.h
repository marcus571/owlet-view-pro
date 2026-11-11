// boards/shields/nice_view_gem/widgets/owlet_events.h
#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

/* 主仓库下发的充电状态事件结构体 */
struct owlet_charger_state_changed {
    bool     is_charging;       /* 是否在充电 */
    uint8_t  vbus_stat;         /* 1=USB, 3=Charger */
    uint8_t  chrg_stat;         /* 0=未充电, 1=预充, 2=快充, 3=充满 */
    uint16_t iindpm_ma;         /* 设定输入限流 */
    uint16_t ichg_ma;           /* 设定充电电流 */
    uint8_t  battery_soc;       /* 电量百分比 0-100 */
    int16_t  battery_temp_c;    /* 电池温度 */
    uint32_t charge_duration_s; /* 充电持续秒数 */
};

ZMK_EVENT_DECLARE(owlet_charger_state_changed);
