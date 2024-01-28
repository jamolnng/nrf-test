#pragma once

#include <bluetooth/services/ancs_client.h>

namespace bt::ancs
{
  int init();
  void discover_completed(bt_gatt_dm *dm, void *ctx);
}