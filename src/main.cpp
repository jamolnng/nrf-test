#include "ble/bt.hpp"
#include "ble/auth.hpp"
#include "ble/cts.hpp"
#include "ble/nus.hpp"
#include "ble/utils.hpp"

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
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
#define GB_HTTP_REQUEST "{\"t\":\"http\", \"url\":\"https://opentdb.com/api.php?amount=1&difficulty=easy&type=boolean\"} \n"

void run_pair(k_work *item);
K_WORK_DELAYABLE_DEFINE(pair_work, run_pair);
void run_pair(k_work *item)
{
  bt::auth::set_pairable(!bt::auth::pairable());
}

void run_time(k_work *item);
K_WORK_DELAYABLE_DEFINE(time_work, run_time);
void run_time(k_work *item)
{
  bt::cts::read_current_time(read_current_time_cb);
  bt::nus::send(reinterpret_cast<const uint8_t *>(GB_HTTP_REQUEST), sizeof(GB_HTTP_REQUEST) - 1);
}

void run_unregister(k_work *item);
K_WORK_DELAYABLE_DEFINE(unregister_work, run_unregister);
void run_unregister(k_work *item)
{
  bt_le_adv_stop();
  bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
  bt_disable();
  int err = bt_le_filter_accept_list_clear();
  LOG_DBG("btn3 (%d)", err);
  k_sleep(K_MSEC(4000));
  sys_reboot(SYS_REBOOT_WARM);
}

void run_reset(k_work *item);
K_WORK_DELAYABLE_DEFINE(reset_work, run_reset);
void run_reset(k_work *item)
{
  bt_disable();
  LOG_DBG("btn4");
  k_sleep(K_MSEC(4000));
  sys_reboot(SYS_REBOOT_WARM);
}

void on_input_subsys_callback(struct input_event *evt)
{
  LOG_DBG("Event: %d, %d, %d, %d", evt->sync, evt->code, evt->type, evt->value);
  if (evt->value == 1)
  {
    switch (evt->code)
    {
    case 11: // button 1
      k_work_schedule(&pair_work, K_NO_WAIT);
      break;
    case 2: // button 2
      k_work_schedule(&time_work, K_NO_WAIT);
      break;
    case 3: // button 3
      k_work_schedule(&unregister_work, K_NO_WAIT);
      break;
    case 4: // button 4
      k_work_schedule(&reset_work, K_NO_WAIT);
      break;
    default:
      break;
    }
  }
}

void run_blink(k_work *item);
K_WORK_DELAYABLE_DEFINE(blink_work, run_blink);
void run_blink(k_work *item)
{
  static int blink_status = 0;
  dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
  k_work_schedule(&blink_work, K_MSEC(500));
}

void run_init(k_work *item)
{
  dk_leds_init();
  bt::init();
  bt::auth::set_pairable(false);
  INPUT_CALLBACK_DEFINE(NULL, on_input_subsys_callback);

  k_work_schedule(&blink_work, K_NO_WAIT);
}
K_WORK_DEFINE(init_work, run_init);

int main()
{
  k_work_submit(&init_work);
  return 0;
}