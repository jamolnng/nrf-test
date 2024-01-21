#include "ble/bt.hpp"
#include "ble/bas.hpp"
#include "ble/auth.hpp"
#include "ble/cts.hpp"
#include "ble/gap.hpp"
#include "ble/nus.hpp"
#include "ble/utils.hpp"

#include "ble/services/gadgetbridge.hpp"

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/reboot.h>

#include <array>
#include <ctime>

LOG_MODULE_REGISTER(main, CONFIG_NRF_TEST_LOG_LEVEL);

#define RUN_STATUS_LED DK_LED1
#define BT_STATUS_LED DK_LED2

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

void read_device_name_cb(bt::gap_client::gap_client *client, const void *data, uint16_t len, int err)
{
  if (err)
  {
    LOG_ERR("Cannot read device name: error: %d", err);
    return;
  }
  LOG_INF("%.*s", len, reinterpret_cast<const char *>(data));
}

#define GB_HTTP_REQUEST "{\"t\":\"http\",\"id\":\"quiz\",\"url\":\"https://opentdb.com/api.php?amount=1&difficulty=easy&type=boolean\"} \n"

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
  bt::gap::read_device_name(read_device_name_cb);
  bt::nus::send(reinterpret_cast<const uint8_t *>(GB_HTTP_REQUEST), sizeof(GB_HTTP_REQUEST) - 1);

  std::array<char, 25> time_str;
  auto now = std::time(nullptr);
  if (std::strftime(time_str.data(), time_str.size(), "%c", std::localtime(&now)))
    LOG_INF("Current time: %s", time_str.data());
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
    case INPUT_KEY_0: // button 1
      k_work_schedule(&pair_work, K_NO_WAIT);
      break;
    case INPUT_KEY_1: // button 2
      k_work_schedule(&time_work, K_NO_WAIT);
      break;
    case INPUT_KEY_2: // button 3
      k_work_schedule(&unregister_work, K_NO_WAIT);
      break;
    case INPUT_KEY_3: // button 4
      k_work_schedule(&reset_work, K_NO_WAIT);
      break;
    default:
      break;
    }
  }
}

void run_send(k_work *item);
K_WORK_DELAYABLE_DEFINE(send_work, run_send);
void run_send(k_work *item)
{
  if (!bt::connected() || !bt::nus::can_send() || system::gadgetbridge::send_ver())
  {
    k_work_schedule(&send_work, K_MSEC(500));
  }
}

void run_blink(k_work *item);
K_WORK_DELAYABLE_DEFINE(blink_work, run_blink);
void run_blink(k_work *item)
{
  static int blink_status = 0;
  dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
  dk_set_led(BT_STATUS_LED, bt::connected());
  k_work_schedule(&blink_work, K_MSEC(500));
}

void run_batt(k_work *item);
K_WORK_DELAYABLE_DEFINE(batt_work, run_batt);
void run_batt(k_work *item)
{
  if (bt::connected())
  {
    static uint8_t batt = 100;
    static uint8_t dir = -1;
    batt += dir;
    bt::bas::set_level(batt);

    int msg_len;
    std::array<char, 100> buf{0};

    int v = 3 * batt + 30;
    msg_len = snprintf(buf.data(),
                       sizeof(buf),
                       "{\"t\":\"status\", \"bat\": %d, \"volt\": %d.%02d, \"chg\": %d} \n",
                       batt,
                       v / 100,
                       v % 100,
                       dir == 1);
    bt::nus::send(reinterpret_cast<uint8_t *>(buf.data()), msg_len);

    if (batt == 50)
    {
      dir = 1;
    }
    if (batt == 100)
    {
      dir = -1;
    }
  }
  k_work_schedule(&batt_work, K_MSEC(1000));
}

void passkey_display(unsigned int passkey)
{
  LOG_INF("Passkey %06u", passkey);
}
bt::auth::auth_cb _auth_callbacks = {
    .passkey_display = passkey_display,
};

void run_init(k_work *item)
{
  dk_leds_init();
  bt::init();
  bt::auth::set_callback(&_auth_callbacks);
  bt::auth::set_pairable(false);
  system::gadgetbridge::init();

  INPUT_CALLBACK_DEFINE(NULL, on_input_subsys_callback);

  k_work_schedule(&blink_work, K_NO_WAIT);
  k_work_schedule(&batt_work, K_NO_WAIT);
  k_work_schedule(&send_work, K_NO_WAIT);
}
K_WORK_DEFINE(init_work, run_init);

int main()
{
  k_work_submit(&init_work);
  return 0;
}