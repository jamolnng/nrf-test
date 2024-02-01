#include "managers/display.hpp"

#include "ui/ui.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/display.h>

#include <algorithm>
#include <array>
#include <chrono>

#include <lvgl.h>

using namespace managers::display;

LOG_MODULE_REGISTER(nrf_test_display, CONFIG_NRF_TEST_LOG_LEVEL);

Display &Display::instance()
{
  static const struct pwm_dt_spec display_bkl = PWM_DT_SPEC_GET_OR(DT_ALIAS(display_bkl), {});
  static const struct device *display_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_display));
  static const struct device *touch_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(cst816s));
  static const struct device *counter_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(timer1));
  static Display display(display_dev, touch_dev, counter_dev, display_bkl);
  return display;
}

void Display::brightness_alarm_start_cb(const device *counter_dev,
                                        uint8_t chan_id, uint32_t ticks,
                                        void *user_data)
{
  auto display = (Display *)user_data;
  pwm_set_pulse_dt(&display->_backlight, display->_backlight.period);
}

void Display::brightness_alarm_run_cb(const device *counter_dev,
                                      uint8_t chan_id, uint32_t ticks,
                                      void *user_data)
{
  auto display = (Display *)user_data;
  pwm_set_pulse_dt(&display->_backlight, display->_backlight.period / 2);
}

void Display::brightness_alarm_stop_cb(const device *counter_dev,
                                       uint8_t chan_id, uint32_t ticks,
                                       void *user_data)
{
  auto display = (Display *)user_data;
  pwm_set_pulse_dt(&display->_backlight, 0);
  counter_stop(counter_dev);
}

Display::Display(const device *display, const device *touch, const device *counter, const pwm_dt_spec backlight)
    : _display(display),
      _touch(touch),
      _counter(counter),
      _backlight(backlight)
{
  _brightness_alarm_start.flags = 0;
  _brightness_alarm_start.callback = &brightness_alarm_start_cb;
  _brightness_alarm_start.user_data = (void *)this;

  _brightness_alarm_run.flags = 0;
  _brightness_alarm_run.callback = &brightness_alarm_run_cb;
  _brightness_alarm_run.user_data = (void *)this;

  _brightness_alarm_stop.flags = 0;
  _brightness_alarm_stop.callback = &brightness_alarm_stop_cb;
  _brightness_alarm_stop.user_data = (void *)this;

  _brightness_alarm_start.ticks = counter_us_to_ticks(_counter, 0);
  _brightness_alarm_run.ticks = counter_us_to_ticks(_counter, 750);
}

void Display::init()
{
  if (!device_is_ready(_display))
  {
    LOG_ERR("Display device not ready");
  }
  if (!device_is_ready(_backlight.dev))
  {
    LOG_ERR("Backlight device not ready");
  }
  if (!device_is_ready(_touch))
  {
    LOG_ERR("Touch device not ready");
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

  pwm_set_pulse_dt(&_backlight, 0); // reset the backlight
  ui_init();
  on();
}

void Display::on()
{
  if (_state == Display::On)
    return;

  _state = Display::On;

  pm_device_action_run(_display, PM_DEVICE_ACTION_RESUME);
  pm_device_action_run(_touch, PM_DEVICE_ACTION_RESUME);

  set_brightness(_last_brightness);
  display_blanking_off(_display);
  k_work_schedule(&_render_work, K_MSEC(lv_task_handler()));
}

void Display::sleep()
{
  if (_state == Display::Sleep)
    return;

  _state = Display::Sleep;
  k_work_cancel_delayable_sync(&_render_work, &_render_cancel_sync);
  k_msleep(CONFIG_LV_DISP_DEF_REFR_PERIOD * 2);

  display_blanking_on(_display);
  pm_device_action_run(_display, PM_DEVICE_ACTION_SUSPEND);
  pm_device_action_run(_touch, PM_DEVICE_ACTION_SUSPEND);
  _last_brightness = _brightness;
  set_brightness(0);

  lv_obj_invalidate(lv_scr_act());
}

void Display::set_brightness(uint8_t brightness)
{
  _next_brightness = brightness;
  k_work_cancel_delayable_sync(&_brightness_work, &_brightness_cancel_sync);
  k_work_schedule(&_brightness_work, K_MSEC(20));
}

void Display::do_set_brightness(k_work *work)
{
  auto *display = CONTAINER_OF(work, Display, _brightness_work);

  uint8_t brightness = std::min(display->_next_brightness, uint8_t(32));
  // // auto npulses = uint8_t(display->_brightness - brightness) % 32;
  auto npulses = 32 - brightness;
  display->_brightness = brightness;

  // npulses = 3;

  // auto npulses = display->_next_brightness;
  // LOG_DBG("br: %d", npulses);

  // if (npulses > 0)
  {
    display->_brightness_alarm_stop.ticks = display->_brightness_alarm_run.ticks + counter_us_to_ticks(display->_counter, display->_backlight.period * (npulses + 1) / NSEC_PER_USEC);
    counter_set_channel_alarm(display->_counter, 0, &display->_brightness_alarm_start);
    counter_set_channel_alarm(display->_counter, 1, &display->_brightness_alarm_run);
    counter_set_channel_alarm(display->_counter, 2, &display->_brightness_alarm_stop);
    counter_start(display->_counter);
  }
}

uint8_t Display::get_brightness()
{
  return _brightness;
}

extern std::chrono::time_point<std::chrono::high_resolution_clock> stopwatch_start_time;
extern bool stopwatch_started;

void Display::render(k_work *work)
{
  std::array<char, 25> time_buf{0};
  auto now = std::time(nullptr);

  std::strftime(time_buf.data(), time_buf.size(), "%a %b %d", std::localtime(&now));
  lv_label_set_text(ui_daymonth, time_buf.data());

  std::strftime(time_buf.data(), time_buf.size(), "%T", std::localtime(&now));
  lv_label_set_text(ui_timehhmmss, time_buf.data());

  std::strftime(time_buf.data(), time_buf.size(), "%Y", std::localtime(&now));
  lv_label_set_text(ui_year, time_buf.data());

  if (stopwatch_started)
  {
    auto final_time = std::chrono::high_resolution_clock::now() - stopwatch_start_time;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(final_time);
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(final_time - minutes);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(final_time - minutes - seconds);
    std::array<char, 50> time_buf{0};
    snprintf(time_buf.data(), time_buf.size(), "%02lld:%02lld.%03lld",
             minutes.count(),
             seconds.count(),
             milliseconds.count());
    lv_label_set_text(ui_time, time_buf.data());
  }

  k_work_delayable *_render_work = CONTAINER_OF(work, k_work_delayable, work);
  lv_task_handler();
  k_work_schedule(_render_work, K_NO_WAIT);
}