#include "ble/cts.hpp"
#include "ble/utils.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <bluetooth/services/cts_client.h>

LOG_MODULE_REGISTER(bt_cts, CONFIG_NRF_TEST_LOG_LEVEL);

static bt_cts_client cts_c;
static bt_conn *current_conn;

static bool has_cts;

static void discover_completed_cb(bt_gatt_dm *dm, void *ctx);
static void discover_service_not_found_cb(bt_conn *conn, void *ctx);
static void discover_error_found_cb(bt_conn *conn, int err, void *ctx);
static const bt_gatt_dm_cb discover_cb = {
    .completed = discover_completed_cb,
    .service_not_found = discover_service_not_found_cb,
    .error_found = discover_error_found_cb,
};

static void cts_discover_retry_handle(k_work *item);
K_WORK_DELAYABLE_DEFINE(cts_discover_retry, cts_discover_retry_handle);

static void notify_current_time_cb(bt_cts_client *cts_c,
                                   bt_cts_current_time *current_time)
{
  bt::current_time_print(current_time);
}

static void enable_notifications(bt_conn *conn)
{
  LOG_DBG("Enable CTS notifications");

  if (has_cts && (bt_conn_get_security(conn) >= BT_SECURITY_L2))
  {
    int err = bt_cts_subscribe_current_time(&cts_c, notify_current_time_cb);
    if (err)
    {
      LOG_ERR("Cannot subscribe to current time value notification (err %d)", err);
    }
  }
}

static void discover_completed_cb(bt_gatt_dm *dm, void *ctx)
{
  LOG_DBG("The discovery procedure succeeded");

  bt_gatt_dm_data_print(dm);

  int err = bt_cts_handles_assign(dm, &cts_c);
  if (err)
  {
    LOG_ERR("Could not assign CTS client handles, error: %d", err);
  }
  else
  {
    has_cts = true;

    if (bt_conn_get_security(cts_c.conn) >= BT_SECURITY_L2)
    {
      // this happens when previously bonded
      enable_notifications(cts_c.conn);
      LOG_DBG("Security >= L2");
    }
    else
    {
      LOG_DBG("Security < L2");
    }
  }

  err = bt_gatt_dm_data_release(dm);
  if (err)
  {
    LOG_ERR("Could not release the discovery data, error code: %d", err);
  }
}

static void discover_service_not_found_cb(bt_conn *conn, void *ctx)
{
  LOG_DBG("The service could not be found during the discovery");
}

static void discover_error_found_cb(bt_conn *conn, int err, void *ctx)
{
  LOG_ERR("The discovery procedure failed, err %d", err);
}

void discover_gattp(bt_conn *conn)
{
  const struct bt_uuid_16 param = {
      .uuid = {BT_UUID_TYPE_16},
      .val = (0x1805),
  };

  int err = bt_gatt_dm_start(conn, reinterpret_cast<const bt_uuid *>(&param), &discover_cb, NULL);
  if (err)
  {
    if (err == -EALREADY)
    {
      // Only one DM discovery can happen at a time, another may be running, so queue it
      k_work_schedule(&cts_discover_retry, K_MSEC(500));
    }
    else
    {
      LOG_ERR("Failed to start discovery for CTS GATT service (err %d)", err);
    }
  }
}

static void cts_discover_retry_handle(struct k_work *item)
{
  discover_gattp(current_conn);
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

void bt::cts::connected(bt_conn *conn)
{
  has_cts = false;
}

void bt::cts::security_changed(struct bt_conn *conn, bt_security_t level)
{
  if (level >= BT_SECURITY_L2)
  {
    discover_gattp(conn);
  }
  // enable_notifications(conn);
}

void bt::cts::read_current_time(bt_cts_read_cb cb)
{
  if (has_cts && (bt_conn_get_security(current_conn) >= BT_SECURITY_L2))
  {
    int err = bt_cts_read_current_time(&cts_c, cb);
    if (err)
    {
      LOG_ERR("Failed reading current time (err: %d)", err);
    }
  }
}