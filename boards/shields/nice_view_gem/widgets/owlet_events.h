// boards/shields/nice_view_gem/widgets/owlet_events.h
#pragma once

#include <zmk/event_manager.h>

struct owlet_charger_state_changed {
    bool     is_charging;       /* true=充电中, false=未充电 */
    uint8_t  vbus_stat;         /* 接口类型枚举 (1=SDP, 2=CDP, 3=DCP, 0=未知) */
    uint8_t  raw_vbus;          /* 新增：BQ25601D REG08 原始寄存器检测值 */
    uint8_t  chrg_stat;         /* 充电阶段 (0=未充, 1=预充, 2=快充, 3=充满) */
    uint16_t ichg_ma;           /* 设定电池充电电流 (mA) */
    uint8_t  battery_soc;       /* 电量百分比 0-100 */
    int16_t  battery_temp_c;    /* 电池温度 (℃) */
    uint32_t charge_duration_s; /* 充电已持续秒数 */
};

ZMK_EVENT_DECLARE(owlet_charger_state_changed);
