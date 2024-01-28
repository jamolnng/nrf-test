#include "managers/devkit.hpp"

#include <dk_buttons_and_leds.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "managers/bluetooth.hpp"

using namespace managers::devkit;

LOG_MODULE_REGISTER(nrf_test_dk, CONFIG_NRF_TEST_LOG_LEVEL);

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