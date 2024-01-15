#pragma once

#include <bluetooth/services/nus_client.h>
#include <bluetooth/services/nus.h>

namespace bt
{
  namespace nus
  {
    int init();
    void discover_completed(bt_gatt_dm *dm, void *ctx);
  }
}