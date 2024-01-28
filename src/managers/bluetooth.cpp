#include "managers/bluetooth.hpp"

#include "ble/bt.hpp"
#include "ble/auth.hpp"
#include "ble/services/gadgetbridge.hpp"

#include <zephyr/logging/log.h>

using namespace managers::bt;

LOG_MODULE_REGISTER(nrf_test_bt, CONFIG_NRF_TEST_LOG_LEVEL);

::bt::auth::auth_cb Bluetooth::_auth_callbacks = {
    .passkey_display = Bluetooth::passkey_display_cb,
};

::bt::bt_cb Bluetooth::_bt_callbacks = {
    .connected = Bluetooth::connected_cb,
    .disconnected = Bluetooth::disconnected_cb,
};

Bluetooth &Bluetooth::instance()
{
  static Bluetooth bluetooth;
  return bluetooth;
}

Bluetooth::Bluetooth()
{
}

void Bluetooth::init()
{
  ::bt::init();
  ::bt::set_callback(&_bt_callbacks);
  ::bt::auth::set_callback(&_auth_callbacks);
  ::bt::auth::set_pairable(false);
  ::bt::services::gadgetbridge::init();
}

bool Bluetooth::connected()
{
  return ::bt::connected();
}

bool Bluetooth::secure()
{
  return ::bt::secure_connection();
}

void Bluetooth::passkey_display_cb(unsigned int passkey)
{
  LOG_INF("Passkey %06u", passkey);
}

void Bluetooth::connected_cb()
{
  LOG_DBG("BT Connected");
}

void Bluetooth::disconnected_cb()
{
  LOG_DBG("BT Disconnected");
}
