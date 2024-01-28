#include "managers/bluetooth.hpp"
#include "managers/devkit.hpp"
#include "managers/display.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_test, CONFIG_NRF_TEST_LOG_LEVEL);

using managers::bt::Bluetooth;
using managers::devkit::DevKit;
using managers::display::Display;

void run_init(k_work *item)
{
  DevKit::instance().init();
  Bluetooth::instance().init();
  Display::instance().init();
}
K_WORK_DEFINE(init_work, run_init);

int main()
{
  LOG_INF("Git hash: %s", GIT_HASH);
  LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock / MHZ(1));
  k_work_submit(&init_work);
  return 0;
}