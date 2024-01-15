#pragma once

#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/cts_client.h>

namespace bt
{
  namespace cts
  {
    int init();
    void connected(bt_conn *conn);
    void security_changed(struct bt_conn *conn, bt_security_t level);
    void read_current_time(bt_cts_read_cb cb);
  }
}