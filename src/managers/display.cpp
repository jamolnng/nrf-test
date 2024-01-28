#include "managers/display.hpp"

#include "ui/ui.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/display.h>

#include <algorithm>

#include <lvgl.h>

using namespace managers::display;

LOG_MODULE_REGISTER(nrf_test_display, CONFIG_NRF_TEST_LOG_LEVEL);

static void backlight_timer_expire(k_timer *timer);
K_TIMER_DEFINE(_brightness_timer, backlight_timer_expire, NULL);

Display &Display::instance()
{
  static const struct pwm_dt_spec display_bkl = PWM_DT_SPEC_GET_OR(DT_ALIAS(display_bkl), {});
  static const struct device *display_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_display));
  static const struct device *touch_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(cst816s));
  static Display display(display_dev, touch_dev, display_bkl);
  return display;
}

Display::Display(const device *display, const device *touch, const pwm_dt_spec backlight)
    : _display(display),
      _touch(touch),
      _backlight(backlight)
{
  _brightness_timer.user_data = (void *)&_backlight;
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
  k_work_cancel_delayable_sync(&_render_work, &_cancel_sync);
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
  brightness = std::min(brightness, uint8_t(32));
  auto npulses = uint8_t(_brightness - brightness) % 32;
  _brightness = brightness;
  if (npulses > 0)
  {
    pwm_set_pulse_dt(&_backlight, _backlight.period / 2);
    k_timer_start(&_brightness_timer, K_NSEC(_backlight.period * (npulses - 1)), K_FOREVER);
  }
}

uint8_t Display::get_brightness()
{
  return _brightness;
}

void backlight_timer_expire(k_timer *timer)
{
  auto *backlight = (pwm_dt_spec *)timer->user_data;
  pwm_set_pulse_dt(backlight, 0);
}

void Display::render(k_work *work)
{
  k_work_delayable *_render_work = CONTAINER_OF(work, k_work_delayable, work);
  k_work_schedule(_render_work, K_MSEC(lv_task_handler()));
}