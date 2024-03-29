#include "ble/bt.hpp"
#include "ble/bas.hpp"
#include "ble/auth.hpp"
#include "ble/cts.hpp"
#include "ble/gap.hpp"
#include "ble/nus.hpp"
#include "ble/utils.hpp"

#include "ble/services/gadgetbridge.hpp"
#include "managers/display.hpp"

#include "ui/ui.h"

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/reboot.h>

#include <array>
#include <ctime>
#include <random>

#include <nrfx_clock.h>

#include <lvgl.h>

static const struct pwm_dt_spec display_bkl = PWM_DT_SPEC_GET_OR(DT_ALIAS(display_bkl), {});

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
  bt::utils::current_time_print(current_time);
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

std::random_device dev;
std::mt19937 rng(dev());
std::uniform_int_distribution<std::mt19937::result_type> dist(0, 255);

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

  // constexpr int mb = 17;
  // static int bright = mb;
  // managers::display::set_brightness(bright);
  // bright -= 1;
  // if (bright <= 0)
  // {
  //   bright += mb;
  // }
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
  // LOG_DBG("Event: %d, %d, %d, %d", evt->sync, evt->code, evt->type, evt->value);
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
  if (!bt::connected() ||
      !bt::secure_connection() ||
      !bt::nus::can_send() ||
      bt::services::gadgetbridge::send_ver())
  {
    k_work_schedule(&send_work, K_MSEC(500));
  }
}
lv_obj_t *hello_world_label;
lv_obj_t *event_label;

void run_blink(k_work *item);
K_WORK_DELAYABLE_DEFINE(blink_work, run_blink);
void run_blink(k_work *item)
{
  std::array<char, 25> time_buf{0};
  auto now = std::time(nullptr);

  std::strftime(time_buf.data(), time_buf.size(), "%a %b %d", std::localtime(&now));
  lv_label_set_text(ui_daymonth, time_buf.data());

  std::strftime(time_buf.data(), time_buf.size(), "%T", std::localtime(&now));
  lv_label_set_text(ui_clocktime, time_buf.data());

  std::strftime(time_buf.data(), time_buf.size(), "%Y", std::localtime(&now));
  lv_label_set_text(ui_year, time_buf.data());

  static int blink_status = 0;
  dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
  dk_set_led(BT_STATUS_LED, bt::connected());
  k_work_schedule(&blink_work, K_MSEC(500));
}

void run_batt(k_work *item);
K_WORK_DELAYABLE_DEFINE(batt_work, run_batt);
void run_batt(k_work *item)
{
  if (bt::connected() &&
      bt::secure_connection() &&
      bt::nus::can_send())
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

void run_lvgl(k_work *item);
K_WORK_DELAYABLE_DEFINE(lvgl_work, run_lvgl);
void run_lvgl(k_work *item)
{
  k_work_schedule(&lvgl_work, K_MSEC(lv_task_handler()));
}

void passkey_display(unsigned int passkey)
{
  LOG_INF("Passkey %06u", passkey);
}
bt::auth::auth_cb _auth_callbacks = {
    .passkey_display = passkey_display,
};

void lvgl_screen_gesture_event_callback(lv_event_t *e)
{
  auto code = lv_event_get_code(e);
  if (code <= LV_EVENT_HIT_TEST)
    LOG_DBG("event %u", code);

  switch (code)
  {
  case LV_EVENT_PRESSED:
    lv_label_set_text(event_label, "PRESSED");
    break;
  case LV_EVENT_CLICKED:
    lv_label_set_text(event_label, "CLICKED");
    break;
  case LV_EVENT_LONG_PRESSED:
    lv_label_set_text(event_label, "LONG_PRESSED");
    break;
  case LV_EVENT_LONG_PRESSED_REPEAT:
    lv_label_set_text(event_label, "LONG_PRESSED_REPEAT");
    break;
  default:
    break;
  }
}

void run_init(k_work *item)
{
  dk_leds_init();
  bt::init();
  bt::auth::set_callback(&_auth_callbacks);
  bt::auth::set_pairable(false);
  bt::services::gadgetbridge::init();
  // drivers::display::gc9a01::init();

  ///////////////////////////////////////

  if (!device_is_ready(display_bkl.dev))
  {
    LOG_WRN("Display brightness control not supported");
  }

  // k_timer_init(&my_timer, my_expiry_function, NULL);
  pwm_set_pulse_dt(&display_bkl, 0);

  ///////////////////////////////////////

  INPUT_CALLBACK_DEFINE(NULL, on_input_subsys_callback);

  LOG_INF("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock / MHZ(1));

  k_work_schedule(&blink_work, K_NO_WAIT);
  k_work_schedule(&batt_work, K_NO_WAIT);
  k_work_schedule(&send_work, K_NO_WAIT);

  const struct device *display_dev;
  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev))
  {
    LOG_ERR("Device not ready, aborting test");
    return;
  }

  lv_indev_t *touch_indev = lv_indev_get_next(NULL);
  while (touch_indev)
  {
    if (lv_indev_get_type(touch_indev) == LV_INDEV_TYPE_POINTER)
    {
      // TODO First fix so not all presses everywhere are registered as clicks and cause vibration
      // Clicking anywehere with this below added right now will cause a vibration, which
      // is not what we want
      // touch_indev->driver->feedback_cb = click_feedback;
      break;
    }
    touch_indev = lv_indev_get_next(touch_indev);
  }

  ui_init();

  // lv_obj_add_event_cb(lv_scr_act(), lvgl_screen_gesture_event_callback, LV_EVENT_ALL, NULL);

  // hello_world_label = lv_label_create(lv_scr_act());
  // event_label = lv_label_create(lv_scr_act());
  // lv_label_set_text(hello_world_label, "Hello world!");
  // lv_label_set_text(event_label, "");
  // lv_obj_set_style_text_font(hello_world_label, &lv_font_montserrat_16, 0);
  // lv_obj_set_style_text_font(event_label, &lv_font_montserrat_12, 0);
  // lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);
  // lv_obj_align(event_label, LV_ALIGN_CENTER, 0, -25);
  // lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  // lv_obj_clear_flag(hello_world_label, LV_OBJ_FLAG_SCROLLABLE);
  // lv_obj_clear_flag(event_label, LV_OBJ_FLAG_SCROLLABLE);
  lv_task_handler();
  // display_blanking_off(display_dev);
  k_work_schedule(&lvgl_work, K_NO_WAIT);
}
K_WORK_DEFINE(init_work, run_init);

int main()
{
  LOG_INF("Git hash: %s", GIT_HASH);
  k_work_submit(&init_work);

  return 0;
}

int hfclk()
{
  return nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
}
SYS_INIT(hfclk, EARLY, 0);