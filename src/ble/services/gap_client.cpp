#include "ble/services/gap_client.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <bluetooth/gatt_dm.h>

#include <cerrno>
#include <cstring>

LOG_MODULE_REGISTER(bt_gap_client_service, CONFIG_NRF_TEST_BLE_LOG_LEVEL);

enum
{
  GAP_ASYNC_READ_PENDING,
  GAP_NOTIF_ENABLED
};

void gap_client_reinit(bt::gap_client::gap_client *client)
{
  client->conn = NULL;
  client->handle_dev_name = 0;
  client->state = ATOMIC_INIT(0);
}

int bt::gap_client::gap_client_init(bt::gap_client::gap_client *client)
{
  if (!client)
  {
    return -EINVAL;
  }
  memset(client, 0, sizeof(gap_client));
  return 0;
}

int bt::gap_client::gap_client_handles_assign(bt_gatt_dm *dm, gap_client *client)
{
  const struct bt_gatt_dm_attr *gatt_service_attr =
      bt_gatt_dm_service_get(dm);
  const struct bt_gatt_service_val *gatt_service =
      bt_gatt_dm_attr_service_val(gatt_service_attr);
  const struct bt_gatt_dm_attr *gatt_chrc;
  const struct bt_gatt_dm_attr *gatt_desc;

  bt_uuid_16 param = BT_UUID_INIT_16(BT_UUID_GAP_VAL);
  if (bt_uuid_cmp(gatt_service->uuid, reinterpret_cast<bt_uuid *>(&param)))
  {
    return -ENOTSUP;
  }

  LOG_DBG("GAP client found");

  gap_client_reinit(client);

  param = {
      .uuid = {BT_UUID_TYPE_16},
      .val = {BT_UUID_GAP_DEVICE_NAME_VAL},
  };

  gatt_chrc = bt_gatt_dm_char_by_uuid(dm, reinterpret_cast<bt_uuid *>(&param));
  if (!gatt_chrc)
  {
    return -EINVAL;
  }

  gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, reinterpret_cast<const bt_uuid *>(&param));
  if (!gatt_desc)
  {
    return -EINVAL;
  }
  client->handle_dev_name = gatt_desc->handle;

  LOG_DBG("GAP device name characteristic found");

  client->conn = bt_gatt_dm_conn_get(dm);
  return 0;
}

uint8_t gap_client_read_callback(bt_conn *conn, uint8_t err, bt_gatt_read_params *params, const void *data, uint16_t length)
{
  bt::gap_client::gap_client *client;
  client = CONTAINER_OF(params, struct bt::gap_client::gap_client, read_params);

  atomic_clear_bit(&client->state, GAP_ASYNC_READ_PENDING);
  if (client->read_cb)
  {
    client->read_cb(client, data, length, err);
  }

  return BT_GATT_ITER_STOP;
}

uint8_t gap_client_notify_callback(bt_conn *conn, bt_gatt_subscribe_params *params, const void *data, uint16_t length)
{
  bt::gap_client::gap_client *client;

  client = CONTAINER_OF(params, struct bt::gap_client::gap_client, notify_params);

  if (client->notify_cb)
  {
    client->notify_cb(client, data, length);
  }

  return BT_GATT_ITER_CONTINUE;
}

int bt::gap_client::gap_client_read_device_name(gap_client *client, gap_client_read_cb func)
{
  if (!client || !client->conn || !func)
  {
    return -EINVAL;
  }
  if (atomic_test_and_set_bit(&client->state, GAP_ASYNC_READ_PENDING))
  {
    return -EBUSY;
  }

  client->read_cb = func;

  client->read_params.func = gap_client_read_callback;
  client->read_params.handle_count = 1;
  client->read_params.single.handle = client->handle_dev_name;
  client->read_params.single.offset = 0;

  int err = bt_gatt_read(client->conn, &client->read_params);
  if (err)
  {
    atomic_clear_bit(&client->state, GAP_ASYNC_READ_PENDING);
  }

  return err;
}