#include "ble/auth.hpp"

#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/conn.h>

LOG_MODULE_REGISTER(bt_auth, CONFIG_NRF_TEST_BLE_LOG_LEVEL);

// static bool pairing_enabled;

bt::auth::auth_cb *auth_callbacks;

static void passkey_display(bt_conn *conn, unsigned int passkey);
static void pairing_accept(bt_conn *conn);
static void pairing_deny(bt_conn *conn);
static void auth_cancel(bt_conn *conn);
static bt_conn_auth_cb auth_cb_display = {
    .passkey_display = nullptr,
    .passkey_entry = nullptr,
    .cancel = auth_cancel,
    .pairing_confirm = pairing_deny,
};

static void pairing_complete(bt_conn *conn, bool bonded);
static void pairing_failed(bt_conn *conn, enum bt_security_err reason);
static bt_conn_auth_info_cb auth_cb_info = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed,
};

static void passkey_display(bt_conn *conn, unsigned int passkey)
{
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_DBG("Passkey for %s: %06u", addr, passkey);

  if (auth_callbacks && auth_callbacks->passkey_display)
  {
    auth_callbacks->passkey_display(passkey);
  }
}

void auth_cancel(bt_conn *conn)
{
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_DBG("Pairing cancelled: %s", addr);
}

void pairing_deny(bt_conn *conn)
{
  LOG_DBG("Pairing deny");
  bt_conn_auth_cancel(conn);
}

void pairing_accept(bt_conn *conn)
{
  LOG_DBG("Pairing accept");
  bt_conn_auth_pairing_confirm(conn);
}

void pairing_complete(bt_conn *conn, bool bonded)
{
  LOG_DBG("Pairing complete");
  bt::auth::set_pairable(false);

  if (auth_callbacks && auth_callbacks->pairing_complete)
  {
    auth_callbacks->pairing_complete();
  }
}

void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
  struct bt_conn_info info;
  char addr[BT_ADDR_LE_STR_LEN];

  if (bt_conn_get_info(conn, &info) < 0)
  {
    addr[0] = '\0';
  }

  bt_addr_le_to_str(info.le.remote, addr, sizeof(addr));
  LOG_DBG("Pairing Failed (%d). Disconnecting.", reason);
  bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

int bt::auth::init()
{
  int err;

  err = bt_conn_auth_cb_register(&auth_cb_display);
  if (err)
  {
    LOG_DBG("Failed to register authorization callbacks.");
    return err;
  }

  err = bt_conn_auth_info_cb_register(&auth_cb_info);
  if (err)
  {
    LOG_DBG("Failed to register authorization info callbacks.");
    return err;
  }
  return 0;
}

void bt::auth::set_pairable(bool pairable)
{
  if (pairable)
  {
    LOG_DBG("Enable Pairable");
    auth_cb_display.pairing_confirm = pairing_accept;
    auth_cb_display.passkey_display = passkey_display;
    bt_conn_auth_cb_register(&auth_cb_display);
  }
  else
  {
    LOG_DBG("Disable Pairable");
    auth_cb_display.pairing_confirm = pairing_deny;
    auth_cb_display.passkey_display = nullptr;
    bt_conn_auth_cb_register(&auth_cb_display);
  }
}

bool bt::auth::pairable()
{
  return auth_cb_display.pairing_confirm == pairing_accept;
}

void bt::auth::set_callback(auth_cb *cb)
{
  if (cb != nullptr)
  {
    auth_callbacks = cb;
  }
}