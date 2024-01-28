cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nrf-test VERSION "0.0.0" LANGUAGES C CXX)

execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

target_sources(app PRIVATE
  src/ble/bt.cpp
  src/ble/auth.cpp
  src/ble/ams.cpp
  src/ble/ancs.cpp
  src/ble/bas.cpp
  src/ble/cts.cpp
  src/ble/gap.cpp
  src/ble/gatt_dm.cpp
  src/ble/nus.cpp
  src/ble/utils.cpp
  src/ble/services/gap_client.cpp
  src/ble/services/gadgetbridge.cpp
  src/ble/services/gadgetbridge/gb_parse.cpp
  src/ble/services/gadgetbridge/st_parse.cpp

  src/drivers/display/gc9a01.cpp
  src/drivers/input/cst816s.cpp

  src/ui/ui_simple_watchface.c
  src/ui/ui.c
  src/ui/ui_comp_hook.c
  src/ui/ui_helpers.c
  src/ui/ui_events.cpp

  src/managers/bluetooth.cpp
  src/managers/devkit.cpp
  src/managers/display.cpp
  src/managers/hfclk.cpp

  src/main.cpp
)

target_include_directories(app PRIVATE
  src/
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics -DGIT_HASH=\\\"${GIT_HASH}\\\"")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGIT_HASH=\\\"${GIT_HASH}\\\"")

# NORDIC SDK APP START
target_sources(app PRIVATE ${app_sources})
# NORDIC SDK APP END
