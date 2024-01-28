#include "ble/bt.hpp"
#include "ble/auth.hpp"
#include "ble/gatt_dm.hpp"

#ifdef CONFIG_BT_AMS_CLIENT
#include "ble/ams.hpp"
#endif
#ifdef CONFIG_BT_ANCS_CLIENT
#include "ble/ancs.hpp"
#endif
#ifdef CONFIG_BT_CTS_CLIENT
#include "ble/cts.hpp"
#endif
#ifdef CONFIG_BT_NUS
#include "ble/nus.hpp"
#endif
#ifdef CONFIG_BT_BAS
#include "ble/bas.hpp"
#endif

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
// #include <bluetooth/services/ancs_client.h>

#include <bluetooth/gatt_dm.h>

LOG_MODULE_REGISTER(bt, CONFIG_NRF_TEST_BLE_LOG_LEVEL);

static uint32_t mtu_max_send_len = 0;
bool _connected = false;
static bt_conn *_bt_current_conn;

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
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
#ifdef CONFIG_BT_ANCS_CLIENT
    BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_ANCS_VAL),
#endif
};

static const bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_SOME, BLE_UUID_TRANSPORT_VAL),
};

// static const bt_data sd[] = {
//     BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
//     // BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
//     BT_DATA_BYTES(BT_DATA_UUID128_ALL, BLE_UUID_TRANSPORT_VAL),
//     BT_DATA_BYTES(BT_DATA_UUID16_ALL,
//                   BT_UUID_16_ENCODE(BT_UUID_CTS_VAL),
//                   BT_UUID_16_ENCODE(BT_UUID_BAS_VAL), ),
//     BT_DATA_BYTES(BT_DATA_SOLICIT128,
// #ifdef CONFIG_BT_AMS_CLIENT
//                   BT_UUID_AMS_VAL,
// #endif
// #ifdef CONFIG_BT_ANCS_CLIENT
//                   BT_UUID_ANCS_VAL,
// #endif
// #ifdef CONFIG_BT_NUS
//                   BT_UUID_NUS_VAL,
// #endif
//                   )};

static void connected(bt_conn *conn, uint8_t err);
static void disconnected(bt_conn *conn, uint8_t reason);
void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
                       const bt_addr_le_t *identity);
static void security_changed(bt_conn *conn, bt_security_t level, bt_security_err err);
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .identity_resolved = identity_resolved,
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
}

void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
                       const bt_addr_le_t *identity)
{
  char addr_identity[BT_ADDR_LE_STR_LEN];
  char addr_rpa[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
  bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));
  LOG_DBG("Identity resolved %s -> %s", addr_rpa, addr_identity);
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

bool existing_bond(bt_conn *conn)
{
  // check for existing bond, if not, disconnect
  auto conn_addr = bt_conn_get_dst(conn);
  auto bc = bond_check{
      .found = false,
      .addr = *conn_addr};
  bt_foreach_bond(BT_ID_DEFAULT, check_bond, &bc);
  // if (!bc.found)
  // {
  //   bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
  //   return;
  // }
  return bc.found;
}

static void connected(bt_conn *conn, uint8_t err)
{
  _connected = false;
  if (err)
  {
    LOG_ERR("Connection failed (err 0x%02x)", err);
    return;
  }
  _bt_current_conn = bt_conn_ref(conn);
  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_DBG("Connected %s", addr);
  mtu_max_send_len = bt_gatt_get_mtu(conn) - 3;
  LOG_DBG("Initial MTU: %d", mtu_max_send_len);
  request_mtu_exchange(conn);

  if (bt::auth::pairable() || existing_bond(conn))
  {
    int rc = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (rc != 0)
    {
      LOG_ERR("Failed to set security: %d", rc);
      bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
      return;
    }
    _connected = true;
  }
  else
  {
    bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
  }
}

static void disconnected(bt_conn *conn, uint8_t reason)
{
  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_DBG("Disconnected: %s (reason %u)", addr, reason);

  if (_bt_current_conn)
  {
    bt_conn_unref(_bt_current_conn);
    _bt_current_conn = NULL;
  }

  _connected = false;
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
      // only start services if we have a secure connection
      bt::gatt_dm::start(conn);
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

#ifdef CONFIG_BT_NUS
  err = bt::nus::init();
  if (err)
  {
    LOG_ERR("Failed to enable NUS, err: %d", err);
  }
#endif

  auto param = BT_LE_ADV_CONN_NAME[0];

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

bool bt::connected()
{
  return _connected;
}

bool bt::secure_connection()
{
  return bt_conn_get_security(_bt_current_conn) >= BT_SECURITY_L2;
}