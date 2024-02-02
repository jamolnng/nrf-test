#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/counter.h>

#include <cstdint>

namespace managers::display
{
  class Display
  {
    enum State
    {
      On,
      Sleep,
      // Off, // requires external regulator
    };

  public:
    Display(Display const &) = delete;
    void operator=(Display const &) = delete;

    static Display &instance();
    void init();
    void on();
    void sleep();
    // void off(); // requires external regulator

    void set_brightness(uint8_t brightness);
    uint8_t get_brightness();

  private:
    Display(const device *display, const device *touch, const device *counter, const pwm_dt_spec backlight);
    ~Display() = default;
    const device *_display;
    const device *_touch;
    const device *_counter;
    const pwm_dt_spec _backlight;
    uint8_t _brightness{32}, _last_brightness{32};
    State _state{Sleep};
    counter_alarm_cfg _brightness_alarm_start, _brightness_alarm_run, _brightness_alarm_stop;

    static void render(k_work *work);
    K_WORK_DELAYABLE_DEFINE(_render_work, render);
    k_work_sync _render_cancel_sync;

    static void do_set_brightness(k_work *work);
    K_WORK_DELAYABLE_DEFINE(_brightness_work, do_set_brightness);
    k_work_sync _brightness_cancel_sync;

    static void brightness_alarm_start_cb(const device *counter_dev,
                                          uint8_t chan_id, uint32_t ticks,
                                          void *user_data);
    static void brightness_alarm_run_cb(const device *counter_dev,
                                        uint8_t chan_id, uint32_t ticks,
                                        void *user_data);
    static void brightness_alarm_stop_cb(const device *counter_dev,
                                         uint8_t chan_id, uint32_t ticks,
                                         void *user_data);
  };
}