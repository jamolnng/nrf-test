#include "ble/cts_client.hpp"

#include <zephyr/logging/log.h>

#include <bluetooth/services/cts_client.h>

LOG_MODULE_REGISTER(bt_cts, LOG_LEVEL_WRN);

static bt_cts_client cts_c;
/* Local copy of the current connection. */
static bt_conn *current_conn;

int bt::cts_init()
{
  int err = bt_cts_client_init(&cts_c);
  if (err)
  {
    printk("CTS client init failed (err %d)\n", err);
    return err;
  }

  return 0;
}