#include "managers/devkit.hpp"

#include <dk_buttons_and_leds.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>

#include "managers/bluetooth.hpp"
#include "managers/display.hpp"

#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/reboot.h>

using namespace managers::devkit;

LOG_MODULE_REGISTER(nrf_test_dk, CONFIG_NRF_TEST_LOG_LEVEL);

void run_unregister(k_work *item);
K_WORK_DELAYABLE_DEFINE(unregister_work, run_unregister);
void run_unregister(k_work *item)
{
  bt_le_adv_stop();
  bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
  bt_disable();
  int err = bt_le_filter_accept_list_clear();
  k_sleep(K_MSEC(1000));
  sys_reboot(SYS_REBOOT_WARM);
}

void run_reset(k_work *item);
K_WORK_DELAYABLE_DEFINE(reset_work, run_reset);
void run_reset(k_work *item)
{
  bt_disable();
  k_sleep(K_MSEC(1000));
  sys_reboot(SYS_REBOOT_WARM);
}

void on_input_subsys_callback(struct input_event *evt)
{
  if (evt->value == 1)
  {
    switch (evt->code)
    {
    case INPUT_KEY_0: // button 1
      bt::auth::set_pairable(!bt::auth::pairable());
      break;
    case INPUT_KEY_1: // button 2
      static int b = 0;
      managers::display::Display::instance().set_brightness(b);
      b++;
      if (b == 32)
      {
        b = 0;
      }
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

DevKit &DevKit::instance()
{
  static DevKit dk;
  return dk;
}

DevKit::DevKit()
{
}

void DevKit::init()
{
  dk_leds_init();
  INPUT_CALLBACK_DEFINE(NULL, on_input_subsys_callback);
  k_work_schedule(&_blink_work, K_NO_WAIT);
}

void DevKit::blink(k_work *work)
{
  static constexpr auto RUN_STATUS_LED = DK_LED1;
  static constexpr auto BT_STATUS_LED = DK_LED2;

  static int blink_status = 0;
  dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);

  auto connected = managers::bt::Bluetooth::instance().connected();
  dk_set_led(BT_STATUS_LED, connected);

  k_work_delayable *_blink_work = k_work_delayable_from_work(work);
  k_work_schedule(_blink_work, K_MSEC(500));
}