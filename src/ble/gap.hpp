#include "ble/services/gap_client.hpp"

namespace bt
{
  namespace gap
  {
    int init();
    void discover_completed(bt_gatt_dm *dm, void *ctx);
    void read_device_name(gap_client::gap_client_read_cb cb);
  }
}