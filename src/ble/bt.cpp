#include "ble/bt.hpp"
#include "ble/auth.hpp"
#include "ble/cts_client.hpp"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#define BLE_UUID_TRANSPORT_VAL \
  BT_UUID_128_ENCODE(0x40495bc0, 0xb302, 0x11ee, 0x9ec1, 0x0800200c9a66)

LOG_MODULE_REGISTER(bt, CONFIG_APP_LOG_LEVEL);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                  (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
                  (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
    // BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_ANCS_VAL),
};

static const struct bt_data ad_nus[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BLE_UUID_TRANSPORT_VAL),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
  if (err)
  {
    printk("Connection failed (err 0x%02x)\n", err);
  }
  else
  {
    printk("Connected\n");
  }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
  printk("Disconnected (reason 0x%02x)\n", reason);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  if (!err)
  {
    printk("Security changed: %s level %u\n", addr, level);

    // enable_notifications();
  }
  else
  {
    printk("Security failed: %s level %u err %d\n", addr, level, err);
  }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

int bt::bt_init()
{
  int err;

  err = bt_enable(nullptr);
#ifdef CONFIG_SETTINGS
  settings_load();
#endif
  if (err)
  {
    printk("Failed to enable Bluetooth, err: %d\n", err);
    return err;
  }

  err = bt::auth_init();
  if (err)
  {
    printk("Failed to auth, err: %d\n", err);
    return err;
  }

  const auto param = BT_LE_ADV_CONN_NAME[0];
  err = bt_le_adv_start(&param, ad, ARRAY_SIZE(ad), NULL, 0);
  if (err)
  {
    printk("Advertising failed to start (err %d)\n", err);
    return err;
  }

  // err = bt::cts_init();
  // if (err)
  // {
  //   LOG_ERR("Failed to enable CTS Client, err: %d", err);
  //   return err;
  // }

  return 0;
}