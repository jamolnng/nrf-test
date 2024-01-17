#ifdef CONFIG_BT_ANCS_CLIENT

#include "ble/ancs.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ancs, CONFIG_NRF_TEST_BLE_LOG_LEVEL);

bt_ancs_client ancs_c;

int bt::ancs::init()
{
  int err;
  err = bt_ancs_client_init(&ancs_c);
  if (err)
  {
    LOG_ERR("ANCS client init failed (err %d)", err);
    return err;
  }
  return 0;
}

void bt::ancs::discover_completed(bt_gatt_dm *dm, void *ctx)
{
  int err = bt_ancs_handles_assign(dm, &ancs_c);
  if (err)
  {
    LOG_ERR("Could not assign CTS client handles, error: %d", err);
    return;
  }
}

#endif