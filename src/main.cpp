#include "ble/bt.hpp"
#include "ble/auth.hpp"
#include "ble/cts.hpp"
#include "ble/utils.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(main, CONFIG_NRF_TEST_LOG_LEVEL);

#define RUN_STATUS_LED DK_LED1

void read_current_time_cb(struct bt_cts_client *cts_c,
                          struct bt_cts_current_time *current_time,
                          int err)
{
  if (err)
  {
    LOG_ERR("Cannot read Current Time: error: %d", err);
    return;
  }
  bt::current_time_print(current_time);
}

void button_changed(uint32_t button_state, uint32_t has_changed)
{
  uint32_t buttons = button_state & has_changed;

  if (buttons & DK_BTN1_MSK)
  {
    bt::auth::set_pairable(!bt::auth::pairable());
  }

  if (buttons & DK_BTN2_MSK)
  {
    bt::cts::read_current_time(read_current_time_cb);
  }

  if (buttons & DK_BTN3_MSK)
  {
    bt_le_adv_stop();
    bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
    bt_disable();
    int err = bt_le_filter_accept_list_clear();
    LOG_DBG("btn3 (%d)", err);
    k_sleep(K_MSEC(4000));
    sys_reboot(SYS_REBOOT_WARM);
  }

  if (buttons & DK_BTN4_MSK)
  {
    bt_disable();
    LOG_DBG("btn4");
    k_sleep(K_MSEC(4000));
    sys_reboot(SYS_REBOOT_WARM);
  }
}

int init_button(void)
{
  int err;

  err = dk_buttons_init(button_changed);
  if (err)
  {
    LOG_DBG("Cannot init buttons (err: %d)", err);
  }

  return err;
}

int main()
{
  init_button();
  dk_leds_init();

  bt::init();
  bt::auth::set_pairable(false);

  int blink_status = 0;
  while (true)
  {
    dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
    k_sleep(K_MSEC(500));
  }

  return 0;
}

// const struct bt_uuid_16 test = {
//     .uuid = {BT_UUID_TYPE_16},
//     .val = (0x1805),
// };

// err = bt_gatt_dm_start(conn, reinterpret_cast<const bt_uuid *>(&test), &discover_cb, NULL);
// if (err)
// {
//   printk("Failed to start discovery (err %d)\n", err);
// }

// struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONNECTABLE,
//                                                     BT_GAP_ADV_FAST_INT_MIN_2,
//                                                     BT_GAP_ADV_FAST_INT_MAX_2, NULL);

// err = bt_le_adv_start(&param, ad, ARRAY_SIZE(ad), NULL, 0);
// if (err)
// {
//   printk("Advertising failed to start (err %d)\n", err);
//   return 0;
// }