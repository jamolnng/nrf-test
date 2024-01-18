#pragma once

#include <bluetooth/services/nus_client.h>
#include <bluetooth/services/nus.h>

namespace bt
{
  namespace nus
  {
    struct nus_cb
    {
      void (*receive)(const uint8_t *data, uint16_t len);
    };

    int init();
    void discover_completed(bt_gatt_dm *dm, void *ctx);
    int send(const uint8_t *data, uint16_t len);
    bool can_send();
    void set_callback(nus_cb *recv_cb);
  }
}