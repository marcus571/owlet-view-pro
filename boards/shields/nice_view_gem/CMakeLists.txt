# ============================================================
# nice_view_gem CMakeLists.txt
# 路径：boards/shields/nice_view_gem/CMakeLists.txt
# ============================================================

if(CONFIG_ZMK_DISPLAY)
  target_sources(app PRIVATE
    custom_status_screen.c       # 核心入口
    behavior_screen_swap.c       # 快捷键 Behavior
    widgets/screen.c             # 多屏调度核心
    widgets/screen_charge.c      # 充电参数屏
    widgets/battery.c
    widgets/output.c
    widgets/layer.c
    widgets/util.c
    assets/images.c
  )
endif()
