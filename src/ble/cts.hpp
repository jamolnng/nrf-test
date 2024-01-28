#pragma once

#include <bluetooth/services/cts_client.h>

namespace bt::cts
{
  int init();
  void discover_completed(bt_gatt_dm *dm, void *ctx);
  void read_current_time(bt_cts_read_cb cb);
}