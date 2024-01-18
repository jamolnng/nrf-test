#include <cstdint>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

namespace bt
{
  namespace gap_client
  {
    struct gap_client;

    typedef void (*gap_client_read_cb)(gap_client *client,
                                       const void *data, uint16_t length,
                                       int err);

    typedef void (*gap_client_notify_cb)(struct gap_client *client,
                                         const void *data, uint16_t length);

    struct gap_client
    {
      bt_conn *conn;
      uint16_t handle_dev_name;
      atomic_t state;
      struct bt_gatt_read_params read_params;
      struct bt_gatt_subscribe_params notify_params;
      gap_client_read_cb read_cb;
      gap_client_notify_cb notify_cb;
    };
    int gap_client_init(gap_client *client);
    int gap_client_handles_assign(bt_gatt_dm *dm, gap_client *client);
    int gap_client_read_device_name(gap_client *client, gap_client_read_cb func);
  }
}