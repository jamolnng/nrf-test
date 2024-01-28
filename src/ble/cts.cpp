#include "ble/cts.hpp"
#include "ble/utils.hpp"
#include "ble/gatt_dm.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <bluetooth/services/cts_client.h>

LOG_MODULE_REGISTER(bt_cts, CONFIG_NRF_TEST_BLE_LOG_LEVEL);

static bt_cts_client cts_c;

void notify_current_time_cb(bt_cts_client *cts_c,
                            bt_cts_current_time *current_time)
{
  bt::utils::current_time_print(current_time);
}

static void enable_notifications(bt_conn *conn)
{
  LOG_DBG("Enable CTS notifications");

  // both of these are implicit
  // if (has_cts && (bt_conn_get_security(conn) >= BT_SECURITY_L2))
  {
    int err = bt_cts_subscribe_current_time(&cts_c, notify_current_time_cb);
    if (err)
    {
      LOG_ERR("Cannot subscribe to current time value notification (err %d)", err);
    }
  }
}

void bt::cts::discover_completed(bt_gatt_dm *dm, void *ctx)
{
  int err = bt_cts_handles_assign(dm, &cts_c);
  if (err)
  {
    LOG_ERR("Could not assign CTS client handles, error: %d", err);
    return;
  }

  // secturity >= L2 implicit since we don't enable services otherwise
  // if (bt_conn_get_security(cts_c.conn) >= BT_SECURITY_L2)
  // {
  // this happens when previously bonded
  enable_notifications(cts_c.conn);
  //   LOG_DBG("Security >= L2");
  // }
  // else
  // {
  //   LOG_DBG("Security < L2");
  // }
}

int bt::cts::init()
{
  int err = bt_cts_client_init(&cts_c);
  if (err)
  {
    LOG_ERR("CTS client init failed (err %d)", err);
    return err;
  }

  return 0;
}

void bt::cts::read_current_time(bt_cts_read_cb cb)
{
  if (bt::gatt_dm::has(bt::gatt_dm::CTS_Client))
  // implicit since we only enable services if securty >= L2
  //  && (bt_conn_get_security(current_conn) >= BT_SECURITY_L2))
  {
    int err = bt_cts_read_current_time(&cts_c, cb);
    if (err)
    {
      LOG_ERR("Failed reading current time (err: %d)", err);
    }
  }
}