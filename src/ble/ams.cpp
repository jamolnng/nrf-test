#ifdef CONFIG_BT_AMS_CLIENT

#include "ble/ams.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ams, CONFIG_NRF_TEST_LOG_LEVEL);

bt_ams_client ams_c;

int bt::ams::init()
{
  int err;
  err = bt_ams_client_init(&ams_c);
  if (err)
  {
    LOG_ERR("AMS client init failed (err %d)", err);
    return err;
  }
  return 0;
}

void bt::ams::discover_completed(bt_gatt_dm *dm, void *ctx)
{
}

#endif