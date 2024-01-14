#include "ble/bt.hpp"
#include "ble/auth.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(main, CONFIG_NRF_TEST_LOG_LEVEL);

void init(k_work *item);
int main();

static bool pairable = false;

K_WORK_DEFINE(init_work, init);

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
  uint32_t buttons = button_state & has_changed;
  int err;

  if (buttons & DK_BTN1_MSK)
  {
    pairable = !pairable;
    bt::auth_set_pairable(pairable);
    LOG_DBG("setting pairable: %d", pairable);
  }
}

static int init_button(void)
{
  int err;

  err = dk_buttons_init(button_changed);
  if (err)
  {
    LOG_DBG("Cannot init buttons (err: %d)", err);
  }

  return err;
}

#define RUN_STATUS_LED DK_LED1

void init(k_work *item)
{
}

int main()
{
  init_button();
  dk_leds_init();

  bt::bt_init();
  bt::auth_set_pairable(false);

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