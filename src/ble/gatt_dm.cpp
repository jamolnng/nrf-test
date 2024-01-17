#include "ble/gatt_dm.hpp"

#ifdef CONFIG_BT_CTS_CLIENT
#include "ble/cts.hpp"
#endif
#ifdef CONFIG_BT_AMS_CLIENT
#include "ble/ams.hpp"
#endif
#ifdef CONFIG_BT_ANCS_CLIENT
#include "ble/ancs.hpp"
#endif

#include <bluetooth/gatt_dm.h>

#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/uuid.h>

// can't call it bt_gatt_dm since that's already in use :/
LOG_MODULE_REGISTER(bt_app_gatt_dm, CONFIG_NRF_TEST_LOG_LEVEL);

void discover_all_completed_cb(bt_gatt_dm *dm, void *ctx);
void discover_all_service_not_found_cb(bt_conn *conn, void *ctx);
void discover_all_error_found_cb(bt_conn *conn, int err, void *ctx);
const bt_gatt_dm_cb discover_all_cb = {
    .completed = discover_all_completed_cb,
    .service_not_found = discover_all_service_not_found_cb,
    .error_found = discover_all_error_found_cb,
};

bool found[] = {
#ifdef CONFIG_BT_CTS_CLIENT
    false,
#endif
#ifdef CONFIG_BT_AMS_CLIENT
    false,
#endif
#ifdef CONFIG_BT_ANCS_CLIENT
    false,
#endif
};

const char *service_names[] = {
#ifdef CONFIG_BT_CTS_CLIENT
    "CTS Client",
#endif
#ifdef CONFIG_BT_AMS_CLIENT
    "AMS Client",
#endif
#ifdef CONFIG_BT_ANCS_CLIENT
    "ANCS Client",
#endif
};

void discover_all_completed_cb(bt_gatt_dm *dm, void *ctx)
{
  bt_gatt_dm_data_print(dm);

  const struct bt_gatt_dm_attr *gatt_service_attr =
      bt_gatt_dm_service_get(dm);
  const struct bt_gatt_service_val *gatt_service =
      bt_gatt_dm_attr_service_val(gatt_service_attr);

  bool handled = false;
#ifdef CONFIG_BT_CTS_CLIENT
  {
    bt_uuid_16 param = BT_UUID_INIT_16(BT_UUID_CTS_VAL);
    if (bt_uuid_cmp(gatt_service->uuid, reinterpret_cast<bt_uuid *>(&param)) == 0)
    {
      found[bt::gatt_dm::CTS_Client] = true;
      LOG_DBG("%s found", service_names[bt::gatt_dm::CTS_Client]);
      int err = bt::cts::init();
      if (err)
      {
        LOG_ERR("Failed to enable CTS Client, err: %d", err);
      }
      bt::cts::discover_completed(dm, ctx);
      handled = true;
    }
  }
#endif
#ifdef CONFIG_BT_AMS_CLIENT
  {
    bt_uuid_128 param = BT_UUID_INIT_128(BT_UUID_AMS_VAL);
    if (bt_uuid_cmp(gatt_service->uuid, reinterpret_cast<bt_uuid *>(&param)) == 0)
    {
      found[bt::gatt_dm::AMS_Client] = true;
      LOG_DBG("%s found", service_names[bt::gatt_dm::AMS_Client]);
      int err = bt::ams::init();
      if (err)
      {
        LOG_ERR("Failed to enable AMS Client, err: %d", err);
      }
      bt::ams::discover_completed(dm, ctx);
      handled = true;
    }
  }
#endif
#ifdef CONFIG_BT_ANCS_CLIENT
  {
    bt_uuid_128 param = BT_UUID_INIT_128(BT_UUID_ANCS_VAL);
    if (bt_uuid_cmp(gatt_service->uuid, reinterpret_cast<bt_uuid *>(&param)) == 0)
    {
      found[bt::gatt_dm::ANCS_Client] = true;
      LOG_DBG("%s found", service_names[bt::gatt_dm::ANCS_Client]);
      int err = bt::ancs::init();
      if (err)
      {
        LOG_ERR("Failed to enable ANCS Client, err: %d", err);
      }
      bt::ancs::discover_completed(dm, ctx);
      handled = true;
    }
  }
#endif
  if (!handled)
  {
    char uuid_str[BT_UUID_STR_LEN];
    // size_t attr_count = bt_gatt_dm_attr_cnt(dm);
    bt_uuid_to_str(gatt_service->uuid, uuid_str, sizeof(uuid_str));
    LOG_ERR("Unhandled service found %s", uuid_str);
    // LOG_ERR("Attribute count: %d\n", attr_count);
  }

  bt_gatt_dm_data_release(dm);

  bt_gatt_dm_continue(dm, NULL);
}

void discover_all_service_not_found_cb(bt_conn *conn, void *ctx)
{
  LOG_DBG("No more services");
  unsigned int index = 0;
  for (auto f : found)
  {
    if (!f)
    {
      LOG_ERR("Service %s not found", service_names[index]);
    }
    index++;
  }
}

void discover_all_error_found_cb(bt_conn *conn, int err, void *ctx)
{
  LOG_ERR("The discovery procedure failed, err %d", err);
}

void bt::gatt_dm::start(bt_conn *conn)
{
  LOG_DBG("Starting GATT discovery manager");
  int err = bt_gatt_dm_start(conn, NULL, &discover_all_cb, NULL);
  // if err == -EALREADY then we are already running the gatt dm
  if (err && err != -EALREADY)
  {
    LOG_ERR("Failed to start GATT discovery (err %d)", err);
  }
}

bool bt::gatt_dm::has(Service service)
{
  return found[service];
}