#include "ble/bas.hpp"

#include <zephyr/bluetooth/services/bas.h>

uint8_t bt::bas::get_level()
{
  return bt_bas_get_battery_level();
}

void bt::bas::set_level(uint8_t level)
{
  bt_bas_set_battery_level(level);
}