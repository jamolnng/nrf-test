#include "ble/bt.hpp"
#include "ble/auth.hpp"
#include "ble/cts.hpp"
#include "ble/gatt_dm.hpp"
#include "ble/nus.hpp"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
// #include <bluetooth/services/ancs_client.h>

#include <bluetooth/gatt_dm.h>

LOG_MODULE_REGISTER(bt, CONFIG_NRF_TEST_LOG_LEVEL);

static uint32_t mtu_max_send_len = 0;

// static bt_conn *current_conn;

struct bond_check
{
  bool found;
  bt_addr_le_t addr;
};

#define BLE_UUID_TRANSPORT_VAL \
  BT_UUID_128_ENCODE(0x40495bc0, 0xb302, 0x11ee, 0x9ec1, 0x0800200c9a66)

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    // BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_BYTES_LIST_LE16(BT_APPEARANCE_SMARTWATCH)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_BYTES_LIST_LE16(BT_APPEARANCE_GENERIC_WATCH)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME,
                  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL),
#ifdef CONFIG_BT_CTS_CLIENT
                  BT_UUID_16_ENCODE(BT_UUID_CTS_VAL),
#endif
                  ),
    BT_DATA_BYTES(BT_DATA_SOLICIT128,
#ifdef CONFIG_BT_AMS_CLIENT
                  BT_UUID_AMS_VAL,
#endif
#ifdef CONFIG_BT_NUS
                  BT_UUID_NUS_VAL,
#endif
                  ),
};

static const bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BLE_UUID_TRANSPORT_VAL),
};

static void connected(bt_conn *conn, uint8_t err);
static void disconnected(bt_conn *conn, uint8_t reason);
static void security_changed(bt_conn *conn, bt_security_t level, bt_security_err err);
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

void mtu_exchange_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params);
static struct bt_gatt_exchange_params exchange_params = {
    .func = mtu_exchange_cb,
};

int ble_comm_get_mtu(bt_conn *conn)
{
  return bt_gatt_get_mtu(conn);
}

void mtu_exchange_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
  if (!err)
  {
    mtu_max_send_len = bt_gatt_get_mtu(conn) - 3;
    LOG_DBG("MTU exchange done. %d", mtu_max_send_len);
  }
  else
  {
    LOG_ERR("MTU exchange failed (err %" PRIu8 ")", err);
  }
}

void request_mtu_exchange(bt_conn *conn)
{
  int err;
  err = bt_gatt_exchange_mtu(conn, &exchange_params);
  if (err)
  {
    LOG_ERR("MTU exchange failed (err %d)", err);
  }
  // else
  // {
  //   LOG_DBG("MTU exchange pending");
  // }
}

static void check_bond(const bt_bond_info *info, void *data)
{
  auto bc = reinterpret_cast<bond_check *>(data);
  auto addr = &bc->addr;
  auto dest = &info->addr;

  if (!bt_addr_le_cmp(dest, addr))
  {
    LOG_DBG("Bond found");
    bc->found = true;
  }
}

static void connected(bt_conn *conn, uint8_t err)
{
  if (err)
  {
    LOG_ERR("Connection failed (err 0x%02x)", err);
    return;
  }
  // current_conn = bt_conn_ref(conn);
  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_DBG("Connected %s", addr);
  mtu_max_send_len = bt_gatt_get_mtu(conn) - 3;
  LOG_DBG("Initial MTU: %d", mtu_max_send_len);
  request_mtu_exchange(conn);

  if (bt::auth::pairable())
  {
    int rc = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (rc != 0)
    {
      LOG_ERR("Failed to set security: %d", rc);
      bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
      return;
    }
  }
  else
  {
    // check for existing bond, if not, disconnect
    auto conn_addr = bt_conn_get_dst(conn);
    auto bc = bond_check{
        .found = false,
        .addr = *conn_addr};
    bt_foreach_bond(BT_ID_DEFAULT, check_bond, &bc);
    if (!bc.found)
    {
      bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
      return;
    }
  }
}

static void disconnected(bt_conn *conn, uint8_t reason)
{
  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_DBG("Disconnected: %s (reason %u)", addr, reason);

  // if (current_conn)
  // {
  //   bt_conn_unref(current_conn);
  //   current_conn = NULL;
  // }
}

static void security_changed(bt_conn *conn, bt_security_t level, bt_security_err err)
{
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  if (err)
  {
    LOG_ERR("Security failed: %s level %u err %d", addr, level, err);
    bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
  }
  else
  {
    LOG_DBG("Security changed: %s level %u", addr, level);
    if (level >= BT_SECURITY_L2)
    {
      int err;
#ifdef CONFIG_BT_CTS_CLIENT
      err = bt::cts::init();
      if (err)
      {
        LOG_ERR("Failed to enable CTS Client, err: %d", err);
      }
#endif

#ifdef CONFIG_BT_AMS_CLIENT
      err = bt::ams::init();
      if (err)
      {
        LOG_ERR("Failed to enable AMS Client, err: %d", err);
      }
#endif

#ifdef CONFIG_BT_ANCS_CLIENT
      err = bt::ancs::init();
      if (err)
      {
        LOG_ERR("Failed to enable ANCS Client, err: %d", err);
      }
#endif
      // only start services if we have a secure connection
      bt::gatt_dm::start(conn);

#ifdef CONFIG_BT_NUS
      err = bt::nus::init();
      if (err)
      {
        LOG_ERR("Failed to enable NUS, err: %d", err);
      }
#endif
    }
  }
}

static void add_bonded_addr_to_filter_list(const struct bt_bond_info *info, void *data)
{
  int *bond_count = reinterpret_cast<int *>(data);
  char addr_str[BT_ADDR_LE_STR_LEN];

  bt_le_filter_accept_list_add(&info->addr);
  bt_addr_le_to_str(&info->addr, addr_str, sizeof(addr_str));
  LOG_DBG("Added %s to advertising accept filter", addr_str);
  (*bond_count)++;
}

int bt::init()
{
  int err;

  err = bt_enable(nullptr);
  if (err)
  {
    LOG_ERR("Failed to enable Bluetooth, err: %d", err);
    return err;
  }

#ifdef CONFIG_SETTINGS
  err = settings_load();
  if (err)
  {
    LOG_ERR("Failed to load user settings, err: %d", err);
    return err;
  }
#endif

  err = bt::auth::init();
  if (err)
  {
    LOG_ERR("Failed to auth, err: %d", err);
    return err;
  }

  auto param = BT_LE_ADV_CONN[0];

  int bond_count = 0;
  bt_foreach_bond(BT_ID_DEFAULT, add_bonded_addr_to_filter_list, &bond_count);
  LOG_DBG("Bond Count %d", bond_count);

  /* If we have got at least one bond, activate the filter */
  if (bond_count > 0)
  {
    /* BT_LE_ADV_OPT_FILTER_CONN is required to activate accept filter list,
     * BT_LE_ADV_OPT_FILTER_SCAN_REQ will prevent sending scan response data to
     * devices, that are not on the accept filter list
     */
    param.options |= BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_FILTER_SCAN_REQ;
  }

  err = bt_le_adv_start(&param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
  if (err)
  {
    LOG_ERR("Advertising failed to start (err %d)", err);
    return err;
  }
  return 0;
}

uint32_t bt::max_send_len()
{
  return mtu_max_send_len;
}