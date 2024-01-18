#include "ble/gap.hpp"

#include "ble/gatt_dm.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_gap_client, CONFIG_NRF_TEST_BLE_LOG_LEVEL);

static bt::gap_client::gap_client client;

int bt::gap::init()
{
  int err = bt::gap_client::gap_client_init(&client);
  if (err)
  {
    LOG_ERR("GAP client init failed (err %d)", err);
    return err;
  }

  return 0;
}

void bt::gap::discover_completed(bt_gatt_dm *dm, void *ctx)
{
  int err = bt::gap_client::gap_client_handles_assign(dm, &client);
  if (err)
  {
    LOG_ERR("Could not assign GAP client handles, error: %d", err);
    return;
  }
}

void bt::gap::read_device_name(bt::gap_client::gap_client_read_cb cb)
{
  if (bt::gatt_dm::has(bt::gatt_dm::GAP_Client))
  // implicit since we only enable services if securty >= L2
  //  && (bt_conn_get_security(current_conn) >= BT_SECURITY_L2))
  {
    int err = bt::gap_client::gap_client_read_device_name(&client, cb);
    if (err)
    {
      LOG_ERR("Failed reading device name (err: %d)", err);
    }
  }
}
