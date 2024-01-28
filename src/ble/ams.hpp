#pragma once

#include <bluetooth/services/ams_client.h>

namespace bt::ams
{
  int init();
  void discover_completed(bt_gatt_dm *dm, void *ctx);
}