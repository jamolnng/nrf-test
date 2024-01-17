cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nrf-test VERSION "0.0.0" LANGUAGES C CXX)

target_sources(app PRIVATE
  src/ble/bt.cpp
  src/ble/auth.cpp
  src/ble/ams.cpp
  src/ble/ancs.cpp
  src/ble/bas.cpp
  src/ble/cts.cpp
  src/ble/gatt_dm.cpp
  src/ble/nus.cpp
  src/ble/utils.cpp

  src/system/gadgetbridge/gb.cpp

  src/main.cpp
)

target_include_directories(app PRIVATE
  src/
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")

# NORDIC SDK APP START
target_sources(app PRIVATE ${app_sources})
# NORDIC SDK APP END
