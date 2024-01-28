#pragma once

#include "ble/bt.hpp"
#include "ble/auth.hpp"

namespace managers::bt
{
  class Bluetooth
  {
  public:
    Bluetooth(Bluetooth const &) = delete;
    void operator=(Bluetooth const &) = delete;
    static Bluetooth &instance();

    void init();

    bool connected();
    bool secure();

  private:
    Bluetooth();
    ~Bluetooth() = default;

    static void passkey_display_cb(unsigned int passkey);
    static ::bt::auth::auth_cb _auth_callbacks;

    static void connected_cb();
    static void disconnected_cb();
    static ::bt::bt_cb _bt_callbacks;
  };
}