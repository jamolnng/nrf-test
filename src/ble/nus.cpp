#include "ble/nus.hpp"

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

// can't call it bt_nus since that's already in use :/
LOG_MODULE_REGISTER(bt_app_nus, CONFIG_NRF_TEST_LOG_LEVEL);

void data_received(bt_conn *conn, const uint8_t *const data, uint16_t len)
{
  LOG_HEXDUMP_DBG(data, len, "RX");
}

bt_nus_cb nus_cb = {
    .received = data_received,
};

int bt::nus::init()
{
  int err = bt_nus_init(&nus_cb);
  if (err)
  {
    LOG_ERR("NUS init failed (err %d)", err);
    return err;
  }

  LOG_DBG("NUS enabled");

  return 0;
}
