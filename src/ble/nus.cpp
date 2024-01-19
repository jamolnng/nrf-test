#include "ble/nus.hpp"

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

// can't call it bt_nus since that's already in use :/
LOG_MODULE_REGISTER(bt_app_nus, CONFIG_NRF_TEST_BLE_LOG_LEVEL);

bt::nus::nus_cb *nus_callbacks;
bool _can_send = false;

void data_received(bt_conn *conn, const uint8_t *const data, uint16_t len)
{
  if (nus_callbacks != nullptr && nus_callbacks->receive)
  {
    nus_callbacks->receive(data, len);
  }
  // memset(tstr, 0, sizeof(tstr));
  // snprintf(tstr, sizeof(tstr), "%.*s", len, data);
  // LOG_DBG("%.*s", len, data);
  // char str[sizeof("RX 000000")];
  // snprintf(str, sizeof(str), "RX %d", len);
  // LOG_HEXDUMP_DBG(data, len, str);
}

void send_enabled(bt_nus_send_status status)
{
  _can_send = status == bt_nus_send_status::BT_NUS_SEND_STATUS_ENABLED;
}

bt_nus_cb _nus_cb = {
    .received = data_received,
    .send_enabled = send_enabled,
};

int bt::nus::init()
{
  int err = bt_nus_init(&_nus_cb);
  if (err)
  {
    LOG_ERR("NUS init failed (err %d)", err);
    return err;
  }

  LOG_DBG("NUS enabled");

  return 0;
}

int bt::nus::send(const uint8_t *data, uint16_t len)
{
  int err = bt_nus_send(NULL, data, len);
  if (err)
  {
    LOG_ERR("Error sending NUS data (err %d)", err);
    return err;
  }
  return 0;
}

void bt::nus::set_callback(nus_cb *cb)
{
  if (cb != nullptr)
  {
    nus_callbacks = cb;
  }
}

bool bt::nus::can_send()
{
  return _can_send;
}